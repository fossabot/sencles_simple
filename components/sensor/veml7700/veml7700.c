// Copyright 2022 JeongYeham 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "veml7700.h"

const char *VEML7700_TAG = "VEML7700";

/**
 * @brief Proper resolution multipliers mapped to gain-integration time combination.
 *
 * @note Source: Official Vishay VEML7700 Application Note, rev. 20-Sep-2019
 *
 * @link https://www.vishay.com/docs/84323/designingveml7700.pdf
 */
const float resolution_map[VEML7700_IT_OPTIONS_COUNT][VEML7700_GAIN_OPTIONS_COUNT] = {
	{0.0036, 0.0072, 0.0288, 0.0576},
	{0.0072, 0.0144, 0.0576, 0.1152},
	{0.0144, 0.0288, 0.1152, 0.2304},
	{0.0288, 0.0576, 0.2304, 0.4608},
	{0.0576, 0.1152, 0.4608, 0.9216},
	{9.1152, 0.2304, 0.9216, 1.8432}};
/**
 * @brief Maximum luminocity mapped to gain-integration time combination.
 *
 * @note Source: Official Vishay VEML7700 Application Note, rev. 20-Sep-2019
 *
 * @link https://www.vishay.com/docs/84323/designingveml7700.pdf
 */
const uint32_t maximums_map[VEML7700_IT_OPTIONS_COUNT][VEML7700_GAIN_OPTIONS_COUNT] = {
	{236, 472, 1887, 3775},
	{472, 944, 3775, 7550},
	{944, 1887, 7550, 15099},
	{1887, 3775, 15099, 30199},
	{3775, 7550, 30199, 60398},
	{7550, 15099, 60398, 120796}};

/**
 * @brief List of all possible values for configuring sensor gain.
 *
 */
const uint8_t gain_values[VEML7700_GAIN_OPTIONS_COUNT] = {
	VEML7700_GAIN_2,
	VEML7700_GAIN_1,
	VEML7700_GAIN_1_8,
	VEML7700_GAIN_1_4};

/**
 * @brief List of all possible values for configuring sensor integration time.
 *
 */
const uint8_t integration_time_values[VEML7700_IT_OPTIONS_COUNT] = {
	VEML7700_IT_800MS,
	VEML7700_IT_400MS,
	VEML7700_IT_200MS,
	VEML7700_IT_100MS,
	VEML7700_IT_50MS,
	VEML7700_IT_25MS};

veml7700_handle_t veml7700_create(i2c_bus_handle_t bus, uint8_t dev_addr)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)calloc(1, sizeof(veml7700_dev_t));
	sens->optimal_signal = xEventGroupCreate();
	xEventGroupClearBits(sens->optimal_signal, 0xff);
	sens->i2c_dev = i2c_bus_device_create(bus, dev_addr, i2c_bus_get_current_clk_speed(bus));
	if (sens->i2c_dev == NULL)
	{
		free(sens);
		return NULL;
	}
	sens->dev_addr = dev_addr;
	return (veml7700_handle_t)sens;
}

esp_err_t veml7700_delete(veml7700_handle_t *sensor)
{
	if (*sensor == NULL)
	{
		return ESP_OK;
	}
	veml7700_dev_t *sens = (veml7700_dev_t *)(*sensor);
	i2c_bus_device_delete(&sens->i2c_dev);
	free(sens);
	*sensor = NULL;
	return ESP_OK;
}

esp_err_t veml7700_optimize_configuration(veml7700_handle_t sensor, float *lux)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	int gain_index = veml7700_get_gain_index(sens->config.gain);
	int it_index = veml7700_get_it_index(sens->config.integration_time);
	if (ceil(*lux) >= maximums_map[it_index][gain_index])
	{ // Decrease resolution
		// Make sure we haven't reached the upper luminocity limit
		if (sens->config.maximum_lux == veml7700_get_maximum_lux())
		{
			ESP_LOGI(VEML7700_TAG, "Already configured for maximum luminocity.");
			return ESP_OK;
		}
		for (size_t i = it_index; i < 5; i++)
		{
			if (i == it_index)
			{
				for (size_t j = gain_index; j < 3; j++)
				{
					if (sens->config.integration_time / resolution_map[i][j] > 100)
					{
						sens->config.integration_time = integration_time_values[i];
						sens->config.gain = gain_values[j];
						break;
					}
				}
			}
			else
			{
				for (size_t j = 0; j < 3; j++)
				{
					if (sens->config.integration_time / resolution_map[i][j] > 100)
					{
						sens->config.integration_time = integration_time_values[i];
						sens->config.gain = gain_values[j];
						break;
					}
				}
			}
		}
		veml7700_send_config(sensor);
	}
	else if (*lux < veml7700_get_lower_maximum_lux(sensor, lux))
	{ // Increase resolution
		// Make sure this isn't the smallest maximum
		if (sens->config.maximum_lux == veml7700_get_lowest_maximum_lux())
		{
			ESP_LOGI(VEML7700_TAG, "Already configured with maximum resolution.");
			return ESP_FAIL;
		}
		for (size_t i = it_index + 1; i > 0; i--)
		{
			if (i == it_index)
			{
				for (size_t j = gain_index + 1; j > 0; j--)
				{
					if (sens->config.integration_time / resolution_map[i][j] > 100)
					{
						sens->config.integration_time = integration_time_values[i];
						sens->config.gain = gain_values[j];
						break;
					}
				}
			}
			else
			{
				for (size_t j = 4; j > 0; j--)
				{
					if (sens->config.integration_time / resolution_map[i][j] > 100)
					{
						sens->config.integration_time = integration_time_values[i];
						sens->config.gain = gain_values[j];
						break;
					}
				}
			}
		}
		veml7700_send_config(sensor);
	}
	else
	{
		ESP_LOGI(VEML7700_TAG, "Configuration already optimal.");
		return ESP_OK;
	}

	ESP_LOGD(VEML7700_TAG, "Configuration optimized.");
	return ESP_OK;
}

uint32_t veml7700_get_current_maximum_lux(veml7700_handle_t sensor)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	int gain_index = veml7700_get_gain_index(sens->config.gain);
	int it_index = veml7700_get_it_index(sens->config.integration_time);

	return maximums_map[it_index][gain_index];
}

uint32_t veml7700_get_lower_maximum_lux(veml7700_handle_t sensor, float *lux)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	int gain_index = veml7700_get_gain_index(sens->config.gain);
	int it_index = veml7700_get_it_index(sens->config.integration_time);

	// Find the next smallest 'maximum' value in the mapped maximum luminocities
	if ((gain_index > 0) && (it_index > 0))
	{
		if (maximums_map[it_index][gain_index - 1] >= maximums_map[it_index - 1][gain_index])
		{
			return maximums_map[it_index][gain_index - 1];
		}
		else
		{
			return maximums_map[it_index - 1][gain_index];
		}
	}
	else if ((gain_index > 0) && (it_index == 0))
	{
		return maximums_map[it_index][gain_index - 1];
	}
	else
	{
		return maximums_map[it_index - 1][gain_index];
	}
}

uint32_t veml7700_get_lowest_maximum_lux()
{
	return maximums_map[0][0];
}

uint32_t veml7700_get_maximum_lux()
{
	return maximums_map[VEML7700_IT_OPTIONS_COUNT - 1][VEML7700_GAIN_OPTIONS_COUNT - 1];
}

int veml7700_get_gain_index(uint8_t gain)
{
	return indexOf(gain, gain_values, VEML7700_GAIN_OPTIONS_COUNT);
}

int veml7700_get_it_index(uint8_t integration_time)
{
	return indexOf(integration_time, integration_time_values, VEML7700_IT_OPTIONS_COUNT);
}

uint8_t indexOf(uint8_t elm, const uint8_t *ar, uint8_t len)
{
	while (len--)
	{
		if (ar[len] == elm)
		{
			return len;
		}
	}
	return -1;
}

esp_err_t veml7700_send_config(veml7700_handle_t sensor)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	uint16_t config_data = ((sens->config.gain << 11) |
							(sens->config.integration_time << 6) |
							(sens->config.persistance << 4) |
							(sens->config.interrupt_enable << 1) |
							(sens->config.shutdown << 0));

	// Set the resolution on the configuration struct
	sens->config.resolution = veml7700_get_resolution(sensor);
	// Set the current maximum value on the configuration struct
	sens->config.maximum_lux = veml7700_get_current_maximum_lux(sensor);

	uint8_t send_config_data[2];
	send_config_data[0] = (uint8_t)config_data & 0x0f;
	send_config_data[1] = config_data >> 8 & 0x0f;
	return i2c_bus_write_bytes(sens->i2c_dev, VEML7700_ALS_CONFIG, 2, &send_config_data[0]);
}

esp_err_t veml7700_set_config(veml7700_handle_t sensor, veml7700_config_t *configuration)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	sens->config = *configuration;
	return veml7700_send_config(sensor);
}

esp_err_t veml7700_read_als_lux(veml7700_handle_t sensor, float *lux)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	esp_err_t i2c_result;
	uint8_t reg_data[2];

	i2c_result = i2c_bus_read_bytes(sens->i2c_dev, VEML7700_ALS_DATA, 2, &reg_data[0]);
	if (i2c_result != 0)
	{
		ESP_LOGW(VEML7700_TAG, "veml7700_i2c_read() returned %d", i2c_result);
		return i2c_result;
	}

	*lux = ((((uint16_t)reg_data[1]) << 8) | reg_data[0]) * sens->config.resolution;

	return ESP_OK;
}

esp_err_t veml7700_read_als_lux_auto(veml7700_handle_t sensor, float *lux)
{

	veml7700_read_als_lux(sensor, lux);
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);

	// Calculate and automatically reconfigure the optimal sensor configuration
	esp_err_t optimize = veml7700_optimize_configuration(sensor, lux);
	ESP_LOGI(VEML7700_TAG, "Configured maximum luminocity: %ld", sens->config.maximum_lux);
	ESP_LOGI(VEML7700_TAG, "Configured resolution: %0.4f", sens->config.resolution);
	ESP_LOGI(VEML7700_TAG, "Configured gain: %d", sens->config.gain);
	ESP_LOGI(VEML7700_TAG, "Configured itime: %d", sens->config.integration_time);
	if (optimize == ESP_OK)
	{
		// Read again
		return veml7700_read_als_lux(sensor, lux);
	}

	return ESP_OK;
}

esp_err_t veml7700_read_white_lux(veml7700_handle_t sensor, float *lux)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	esp_err_t i2c_result;
	uint8_t reg_data[2];

	i2c_result = i2c_bus_read_bytes(sens->i2c_dev, VEML7700_WHITE_DATA, 2, &reg_data[0]);
	if (i2c_result != 0)
	{
		ESP_LOGW(VEML7700_TAG, "veml7700_i2c_read() returned %d", i2c_result);
		return ESP_FAIL;
	}

	*lux = ((((uint16_t)reg_data[1]) << 8) | reg_data[0]) * sens->config.resolution;

	return ESP_OK;
}

esp_err_t veml7700_read_white_lux_auto(veml7700_handle_t sensor, float *lux)
{

	veml7700_read_white_lux(sensor, lux);
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);

	// Calculate and automatically reconfigure the optimal sensor configuration
	esp_err_t optimize = veml7700_optimize_configuration(sensor, lux);
	ESP_LOGI(VEML7700_TAG, "Configured maximum luminocity: %ld", sens->config.maximum_lux);
	ESP_LOGI(VEML7700_TAG, "Configured resolution: %0.4f", sens->config.resolution);
	ESP_LOGI(VEML7700_TAG, "Configured gain: %d", sens->config.gain);
	ESP_LOGI(VEML7700_TAG, "Configured itime: %d", sens->config.integration_time);
	if (optimize == ESP_OK)
	{
		// Read again
		return veml7700_read_white_lux(sensor, lux);
	}

	return ESP_OK;
}

float veml7700_get_resolution(veml7700_handle_t sensor)
{
	veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
	int gain_index = veml7700_get_gain_index(sens->config.gain);
	int it_index = veml7700_get_it_index(sens->config.integration_time);

	return resolution_map[it_index][gain_index];
}

#ifdef CONFIG_SENSOR_LIGHT_INCLUDED_VEML7700

veml7700_handle_t veml7700 = NULL;
bool is_init = false;

esp_err_t light_sensor_veml7700_init(i2c_bus_handle_t i2c_bus)
{
	if (is_init || !i2c_bus)
	{
		return ESP_FAIL;
	}
	veml7700 = veml7700_create(i2c_bus, VEML7700_I2C_ADDRESS);
	if (!veml7700)
	{
		return ESP_FAIL;
	}
	veml7700_config_t veml7700_info;
	memset(&veml7700_info, 0, sizeof(veml7700_info));
	veml7700_info.integration_time = VEML7700_IT_100MS;
	veml7700_info.gain = VEML7700_GAIN_1_8;
	veml7700_info.interrupt_enable = false;
	veml7700_info.persistance = VEML7700_PERS_1;
	veml7700_info.shutdown = VEML7700_POWERSAVE_MODE1;
	esp_err_t ret = veml7700_set_config(veml7700, &veml7700_info);
	if (ret != ESP_OK)
	{
		return ESP_FAIL;
	}
	is_init = true;
	return ESP_OK;
}

esp_err_t light_sensor_veml7700_deinit(void)
{
	if (!is_init)
	{
		return ESP_FAIL;
	}
	esp_err_t ret = veml7700_delete(&veml7700);
	if (ret != ESP_OK)
	{
		return ESP_FAIL;
	}
	is_init = false;
	return ESP_OK;
}

esp_err_t light_sensor_veml7700_test(void)
{
	if (!is_init)
	{
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t light_sensor_veml7700_acquire_light(float *light, float *white)
{
	esp_err_t ret1, ret2;
	if (!is_init)
	{
		return ESP_FAIL;
	}
	ret1 = veml7700_read_als_lux_auto(veml7700, light);
	ret2 = veml7700_read_white_lux_auto(veml7700, white);
	if (ret1 || ret2 == 0)
		return ESP_OK;
	return ESP_FAIL;
}

#endif