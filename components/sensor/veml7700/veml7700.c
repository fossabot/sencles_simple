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
 * @brief List of all possible values for configuring sensor gain.
 *
 */
const uint8_t gain_values[VEML7700_GAIN_OPTIONS_COUNT] = {VEML7700_GAIN_1_8, VEML7700_GAIN_1_4,
                                                          VEML7700_GAIN_1, VEML7700_GAIN_2};

/**
 * @brief List of all possible values for configuring sensor integration time.
 *
 */
const uint8_t integration_time_values[VEML7700_IT_OPTIONS_COUNT] = {VEML7700_IT_25MS, VEML7700_IT_50MS,
                                                                    VEML7700_IT_100MS, VEML7700_IT_200MS,
                                                                    VEML7700_IT_400MS, VEML7700_IT_800MS};

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
  uint16_t raw;
  veml7700_config_t veml7700_info = {0};
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);

  int gain_index = 0, it_index = 2;
  bool useCorrection = false;

  veml7700_info.gain = gain_values[gain_index];
  sens->config.gain = veml7700_info.gain;
  veml7700_send_config(sensor, &veml7700_info, VEML7700_SEND_GAIN);

  veml7700_info.integration_time = integration_time_values[it_index];
  sens->config.integration_time = veml7700_info.integration_time;
  veml7700_send_config(sensor, &veml7700_info, VEML7700_SEND_ITIME);

  veml7700_read_als_lux(sensor, &raw);
  if (raw <= 100)
  {
    // increase first gain and then integration time as needed
    // compute lux using simple linear formula
    while ((raw <= 100) && !((gain_index == 3) && (it_index == 5)))
    {
      if (gain_index < 3)
      {
        veml7700_info.gain = gain_values[++gain_index];
        sens->config.gain = veml7700_info.gain;
        veml7700_send_config(sensor, &veml7700_info, VEML7700_SEND_GAIN);
      }
      else if (it_index < 5)
      {
        veml7700_info.integration_time = integration_time_values[++it_index];
        sens->config.integration_time = veml7700_info.integration_time;
        veml7700_send_config(sensor, &veml7700_info, VEML7700_SEND_ITIME);
      }
      veml7700_read_als_lux(sensor, &raw);
    }
  }
  else
  {
    // decrease integration time as needed
    // compute lux using non-linear correction
    useCorrection = true;
    while ((raw > 10000) && (it_index > 0))
    {
      veml7700_info.integration_time = integration_time_values[--it_index];
      veml7700_send_config(sensor, &veml7700_info, VEML7700_SEND_ITIME);
      veml7700_read_als_lux(sensor, &raw);
    }
  }
  *lux = computeLux(sensor, raw, useCorrection);
  return ESP_OK;
}

esp_err_t veml7700_send_config(veml7700_handle_t sensor, veml7700_config_t *configuration, veml7700_send_config_t way)
{
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
  switch (way)
  {
  case VEML7700_SEND_ALL:
    sens->config = *configuration;
    uint16_t config_data = ((sens->config.gain << 11) |
                            (sens->config.integration_time << 6) |
                            (sens->config.persistance << 4) |
                            (sens->config.interrupt_enable << 1) |
                            (sens->config.shutdown << 0));

    uint8_t send_config_data[2];
    send_config_data[0] = (uint8_t)config_data & 0x0f;
    send_config_data[1] = config_data >> 8 & 0x0f;
    return i2c_bus_write_bytes(sens->i2c_dev, VEML7700_ALS_CONFIG, 2, &send_config_data[0]);
    break;
  case VEML7700_SEND_GAIN:
    sens->config.gain = configuration->gain;
    ESP_LOGI(VEML7700_TAG, "write gain %d", sens->config.gain);
    return i2c_bus_write_bits(sens->i2c_dev, VEML7700_ALS_CONFIG, 3, 2, sens->config.gain);
    break;
  case VEML7700_SEND_ITIME:
    sens->config.integration_time = configuration->integration_time;
    ESP_LOGI(VEML7700_TAG, "write time %d", sens->config.integration_time);
    return i2c_bus_write_bits(sens->i2c_dev, VEML7700_ALS_CONFIG, 6, 4, sens->config.integration_time);
    break;

  default:
    ESP_LOGI(VEML7700_TAG, "Do not know which way to send!");
    return ESP_FAIL;
    break;
  }
}

esp_err_t veml7700_read_als_lux(veml7700_handle_t sensor, uint16_t *raw)
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

  *raw = ((((uint16_t)reg_data[1]) << 8) | reg_data[0]);
  if (i2c_result == !ESP_OK)
    return ESP_FAIL;
  return ESP_OK;
}

esp_err_t veml7700_read_als_lux_auto(veml7700_handle_t sensor, float *lux)
{
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);

  // Calculate and automatically reconfigure the optimal sensor configuration
  veml7700_optimize_configuration(sensor, lux);
  ESP_LOGI(VEML7700_TAG, "Configured gain: %d", sens->config.gain);
  ESP_LOGI(VEML7700_TAG, "Configured itime: %d", sens->config.integration_time);
  return ESP_OK;
}

esp_err_t veml7700_read_white_lux(veml7700_handle_t sensor, float *raw)
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

  *raw = ((((uint16_t)reg_data[1]) << 8) | reg_data[0]);
  if (i2c_result == !ESP_OK)
    return ESP_FAIL;
  return ESP_OK;
}

esp_err_t veml7700_read_white_lux_auto(veml7700_handle_t sensor, float *lux)
{

  veml7700_read_white_lux(sensor, lux);
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);

  // Calculate and automatically reconfigure the optimal sensor configuration
  veml7700_optimize_configuration(sensor, lux);
  ESP_LOGI(VEML7700_TAG, "Configured gain: %d", sens->config.gain);
  ESP_LOGI(VEML7700_TAG, "Configured itime: %d", sens->config.integration_time);
  return ESP_OK;
}

inline int getIntegrationTimeValue(veml7700_handle_t sensor)
{
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
  switch (sens->config.integration_time)
  {
  case VEML7700_IT_25MS:
    return 25;
  case VEML7700_IT_50MS:
    return 50;
  case VEML7700_IT_100MS:
    return 100;
  case VEML7700_IT_200MS:
    return 200;
  case VEML7700_IT_400MS:
    return 400;
  case VEML7700_IT_800MS:
    return 800;
  default:
    return -1;
  }
}
inline float getGainValue(veml7700_handle_t sensor)
{
  veml7700_dev_t *sens = (veml7700_dev_t *)(sensor);
  switch (sens->config.gain)
  {
  case VEML7700_GAIN_1_8:
    return 0.125;
  case VEML7700_GAIN_1_4:
    return 0.25;
  case VEML7700_GAIN_1:
    return 1;
  case VEML7700_GAIN_2:
    return 2;
  default:
    return -1;
  }
}
inline float get_resolution(veml7700_handle_t sensor)
{
  return 0.0036 * (800 / getIntegrationTimeValue(sensor)) * (2 / getGainValue(sensor));
}

inline float computeLux(veml7700_handle_t sensor, uint16_t rawALS, bool corrected)
{
  float lux = get_resolution(sensor) * rawALS;
  ESP_LOGI(VEML7700_TAG, "resolution : %f, raw is %d", get_resolution(sensor), rawALS);
  if (corrected)
    lux = (((6.0135e-13 * lux - 9.3924e-9) * lux + 8.1488e-5) * lux + 1.0023) * lux;
  return lux;
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
  veml7700_info.interrupt_enable = false;
  veml7700_info.persistance = VEML7700_PERS_1;
  veml7700_info.shutdown = VEML7700_POWERSAVE_MODE1;
  esp_err_t ret = veml7700_send_config(veml7700, &veml7700_info, VEML7700_SEND_ALL);
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