// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
// Copyright 2021-2022 Jeong Yeham
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
#include "driver/i2c.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sht4x.h"
#include "esp_timer.h"

typedef struct {
    i2c_bus_device_handle_t i2c_dev;
    uint8_t dev_addr;
} sht4x_sensor_t;

sht4x_handle_t sht4x_create(i2c_bus_handle_t bus, uint8_t dev_addr)
{
    sht4x_sensor_t *sens = (sht4x_sensor_t *) calloc(1, sizeof(sht4x_sensor_t));
    sens->i2c_dev = i2c_bus_device_create(bus, dev_addr, i2c_bus_get_current_clk_speed(bus));
    if (sens->i2c_dev == NULL) {
        free(sens);
        return NULL;
    }
    sens->dev_addr = dev_addr;
    return (sht4x_handle_t) sens;
}

esp_err_t sht4x_delete(sht4x_handle_t *sensor)
{
    if (*sensor == NULL) {
        return ESP_OK;
    }

    sht4x_sensor_t *sens = (sht4x_sensor_t *)(*sensor);
    i2c_bus_device_delete(&sens->i2c_dev);
    free(sens);
    *sensor = NULL;
    return ESP_OK;
}

static esp_err_t sht4x_write_cmd(sht4x_handle_t sensor, sht4x_cmd_measure_t sht4x_cmd)
{
    sht4x_sensor_t *sens = (sht4x_sensor_t *) sensor;
    uint8_t cmd_buffer;
    cmd_buffer = sht4x_cmd;
    esp_err_t ret = i2c_bus_write_bytes(sens->i2c_dev, NULL_I2C_MEM_ADDR, 1, cmd_buffer);
    return ret;
}

static esp_err_t sht4x_get_data(sht4x_handle_t sensor, uint8_t data_len, uint8_t *data_arr)
{
    sht4x_sensor_t *sens = (sht4x_sensor_t *) sensor;
    esp_err_t ret = i2c_bus_read_bytes(sens->i2c_dev, NULL_I2C_MEM_ADDR, data_len, data_arr);
    return ret;
}

static uint8_t CheckCrc8(uint8_t *const message, uint8_t initial_value)
{
    uint8_t  crc;
    int  i = 0, j = 0;
    crc = initial_value;

    for (j = 0; j < 2; j++) {
        crc ^= message[j];
        for (i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x41;         /*!< 0x31 is Polynomial for 8-bit CRC checksum*/
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

esp_err_t sht3x_measure_period(sht4x_measure_mode mode, uint16_t *min_delay_us)
{
    //set value
    switch (mode) {
        case SHT4x_WITHOUT_HEATER_HIGH_PRE:
            *min_delay_us = 2000;
            break;
        case SHT4x_WITHOUT_HEATER_MEDIUM_PRE:
            *min_delay_us = 1000;
            break;
        case 2:
            *min_delay_us = 500;
            break;
        case 3:
            *min_delay_us = 250;
            break;
        case 4:
            *min_delay_us = 100;
            break;
        case 5:
            *min_delay_us = 100;
            break;        
        case 6:
            *min_delay_us = 100;
            break;        
        case 7:
            *min_delay_us = 100;
            break;        
        case 8:
            *min_delay_us = 100;
            break;
        case 9:
            *min_delay_us = 100;
            break;
        default:
            break;
    }
    return ESP_OK;
}


esp_err_t sht3x_get_single_shot(sht4x_handle_t sensor, sht4x_measure_mode mode, float *Tem_val, float *Hum_val)
{
    uint8_t buff[6];
    uint16_t tem, hum;
    static float Temperature = 0;
    static float Humidity = 0;
    static int64_t last_shot_time = 0;
    int64_t current_time = esp_timer_get_time();
    int64_t min_delay_us = 0;
    sht4x_measure_period(false, &min_delay_us);

    if ((current_time - last_shot_time) < min_delay_us) {
        *Tem_val = Temperature;
        *Hum_val = Humidity;
        return ESP_OK;
    }

    esp_err_t ret = sht4x_get_data(sensor, 6, buff);

    /* check crc */
    if (ret != ESP_OK || CheckCrc8(buff, 0xFF) != buff[2] || CheckCrc8(&buff[3], 0xFF) != buff[5]) {
        return ESP_FAIL;
    }

    last_shot_time = current_time;
    tem = (((uint16_t)buff[0] << 8) | buff[1]);
    Temperature = (175.0 * (float)tem / 65535.0 - 45.0) ;  /*!< T = -45 + 175 * tem / (2^16-1), this temperature conversion formula is for Celsius °C */
    //Temperature= (315.0*(float)tem/65535.0-49.0) ;     /*!< T = -45 + 175 * tem / (2^16-1), this temperature conversion formula is for Fahrenheit °F */
    hum = (((uint16_t)buff[3] << 8) | buff[4]);
    Humidity = (100.0 * (float)hum / 65535.0);            /*!< RH = hum*100 / (2^16-1) */

    if ((Temperature >= -20) && (Temperature <= 125) && (Humidity >= 0) && (Humidity <= 100)) {
        *Tem_val = Temperature;
        *Hum_val = Humidity;
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t sht4x_soft_reset(sht4x_handle_t sensor)
{
    esp_err_t ret = sht4x_write_cmd(sensor, SOFT_RESET_CMD);
    return ret;
}



#ifdef CONFIG_SENSOR_HUMITURE_INCLUDED_SHT4X

static sht4x_handle_t sht4x = NULL;
static bool is_init = false;

esp_err_t humiture_sht4x_init(i2c_bus_handle_t i2c_bus)
{
    if (is_init || !i2c_bus) {
        return ESP_FAIL;
    }

    sht4x = sht4x_create(i2c_bus, SHT4x_ADDR_PIN);

    if (!sht4x) {
        return ESP_FAIL;
    }

    esp_err_t ret = sht4x_set_measure_mode(sht4x, SHT4x_PER_4_MEDIUM); /**medium accuracy/repeatability with 250ms period (1000ms/4)**/

    if (ret != ESP_OK) {
        return ESP_FAIL;
    }

    is_init = true;
    return ESP_OK;
}

esp_err_t humiture_sht4x_deinit(void)
{
    if (!is_init) {
        return ESP_FAIL;
    }

    esp_err_t ret = sht4x_delete(&sht4x);

    if (ret != ESP_OK) {
        return ESP_FAIL;
    }

    is_init = false;
    return ESP_OK;
}

esp_err_t humiture_sht4x_test(void)
{
    if (!is_init) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t humiture_sht4x_acquire_humidity(float *h)
{
    if (!is_init) {
        return ESP_FAIL;
    }

    float temperature = 0;
    float humidity = 0;
    esp_err_t ret = sht4x_get_single_shot(sht4x, &temperature, &humidity);

    if (ret == ESP_OK) {
        *h = humidity;
        return ESP_OK;
    }

    *h = 0;
    return ESP_FAIL;
}

esp_err_t humiture_sht4x_acquire_temperature(float *t)
{
    if (!is_init) {
        return ESP_FAIL;
    }

    float temperature = 0;
    float humidity = 0;
    esp_err_t ret = sht4x_get_single_shot(sht4x, &temperature, &humidity);

    if (ret == ESP_OK) {
        *t = temperature;
        return ESP_OK;
    }

    *t = 0;
    return ESP_FAIL;
}

#endif