// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _SHT4x_H_
#define _SHT4x_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2c.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "math.h"

typedef enum {

    SOFT_RESET_CMD = 0x94,    /*!< Command to soft reset*/
    READ_SERIAL_NUMBER = 0x89,        /*!< Command to read serial number
                                            [2 * 8-bit data; 8-bit CRC; 2 * 8-bit data; 8-bit CRC]*/

    /* Data Acquisition */
    SHT4x_MEASURE_HIGH_PRECISION = 0xFD,     /*!< Command to measure T & RH with high precision (high repeatability)
                                                    [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/
    
    SHT4x_MEASURE_MEDIUM_PRECISION = 0xF6,   /*!< Command to measure T & RH with medium precision (medium repeatability) 
                                                    [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/

    SHT4x_MEASURE_LOW_PRECISION = 0xE0,     /*!< Command to measure T & RH with lowest precision (low repeatability)
                                                    [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/
    
    /* Data Acquisition with heater condition */
    SHT4x_MEASURE_HIGH_PRECISION_200mW_1s = 0x39,   /*!< Command to activate heater with 200mW for 1s, including a high precision measurement just before deactivation
                                                            [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/
    SHT4x_MEASURE_HIGH_PRECISION_200mW_01s = 0x32,   /*!< Command to activate heater with 200mW for 0.1s, including a high precision measurement just before deactivation
                                                              [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/


    SHT4x_MEASURE_HIGH_PRECISION_110mW_1s = 0x2F,   /*!< Command to activate heater with 110mW for 1s, including a high precision measurement just before deactivation
                                                            [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/  
    SHT4x_MEASURE_HIGH_PRECISION_110mW_01s = 0x24,  /*!< Command to activate heater with 110mW for 0.1s, including a high precision measurement just before deactivation
                                                             [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/


    SHT4x_MEASURE_HIGH_PRECISION_20mW_1s = 0x1E,   /*!< Command to activate heater with 20mW for 1s, including a high precision measurement just before deactivation
                                                            [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/
    SHT4x_MEASURE_HIGH_PRECISION_20mW_01s = 0x15,  /*!< Command to activate heater with 20mW for 0.1s, including a high precision measurement just before deactivation
                                                            [2 * 8-bit T-data; 8-bit CRC; 2 * 8-bit RH-data; 8-bit CRC]*/
} sht4x_cmd_measure_t;

typedef enum {
    SHT4x_ADDR_PIN = 0x44, /*!< set address PIN   */
} sht4x_set_address_t;


typedef void *sht4x_handle_t;

/**
 * @brief Create sht4x handle_t
 *
 * @param bus sensorice object handle of sht4x
 * @param dev_addr sensorice address
 *
 * @return
 *     - sht4x handle_t
 */
sht4x_handle_t sht4x_create(i2c_bus_handle_t bus, uint8_t dev_addr);

/**
 * @brief Delete sht4x handle_t
 *
 * @param sensor point to sensorice object handle of sht4x
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t sht4x_delete(sht4x_handle_t *sensor);

/**
 * @brief Get temperature and humidity just once
 *
 * @param sensor object handle of shd4x
 * @param Tem_val temperature data
 * @param Hum_val humidity data
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t sht4x_get_single_shot(sht4x_handle_t sensor, sht4x_cmd_measure_t mode, float *Tem_val, float *Hum_val);

/**
 * @brief Soft reset for sht4x
 *
 * @param sensor object handle of sht4x
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t sht4x_soft_reset(sht4x_handle_t sensor);
esp_err_t humiture_sht4x_test(void);



/***implements of humiture hal interface****/
#ifdef CONFIG_SENSOR_HUMITURE_INCLUDED_SHT4X

/**
 * @brief initialize sht4x with default configurations
 *
 * @param i2c_bus i2c bus handle the sensor will attached to
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t humiture_sht4x_init(i2c_bus_handle_t handle);

/**
 * @brief de-initialize sht4x
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t humiture_sht4x_deinit(void);

/**
 * @brief acquire relative humidity result one time.
 *
 * @param h point to result data (unit:percentage)
 * @param t point to result data (unit:dce)
 * @return esp_err_t
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t humiture_sht4x_acquire_humiture(float *h, float *t);

#endif

#ifdef __cplusplus
}
#endif

#endif
