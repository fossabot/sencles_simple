// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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
#ifndef _VEML7700_H_
#define _VEML7700_H_

#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define VEML7700_I2C_ADDRESS (0x10)
#define VEML7700_I2C_ERR_RES (-1)

// CMD
#define VEML7700_ALS_CONFIG 0x00        /*!< Light configuration register */
#define VEML7700_ALS_THREHOLD_HIGH 0x01 /*!< Light high threshold for irq */
#define VEML7700_ALS_THREHOLD_LOW 0x02  /*!< Light low threshold for irq */
#define VEML7700_ALS_POWER_SAVE 0x03    /*!< Power save register */
#define VEML7700_ALS_DATA 0x04          /*!< The light data output */
#define VEML7700_WHITE_DATA 0x05        /*!< The white light data output */
#define VEML7700_INTERRUPTSTATUS 0x06   /*!< What IRQ (if any) */

#define VEML7700_INTERRUPT_HIGH 0x4000 /*!< Interrupt status for high threshold */
#define VEML7700_INTERRUPT_LOW 0x8000  /*!< Interrupt status for low threshold */

#define VEML7700_GAIN_OPTIONS_COUNT 4 /*!< Possible gain values count */
#define VEML7700_IT_OPTIONS_COUNT 6   /*!< Possible integration time values count */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum
{
    VEML7700_GAIN_2 = 1,   /*!< ALS gain 2x */
    VEML7700_GAIN_1 = 0,   /*!< ALS gain 1x */
    VEML7700_GAIN_1_8 = 2, /*!< ALS gain 1/8x */
    VEML7700_GAIN_1_4 = 3, /*!< ALS gain 1/4x */
} veml7700_gain_t;

typedef enum
{
    VEML7700_IT_25MS = 12, /*!< Command to set integration time 25ms*/
    VEML7700_IT_50MS = 8,  /*!< Command to set integration time 50ms*/
    VEML7700_IT_100MS = 0, /*!< Command to set integration time 100ms*/
    VEML7700_IT_200MS = 1, /*!< Command to set integration time 2000ms*/
    VEML7700_IT_400MS = 2, /*!< Command to set integration time 4000ms*/
    VEML7700_IT_800MS = 3, /*!< Command to set integration time 8000ms*/
} veml7700_integration_time_t;


typedef enum 
{
    VEML7700_POWERSAVE_MODE1 = 0, /*!< Power saving mode 1 */
    VEML7700_POWERSAVE_MODE2 = 1, /*!< Power saving mode 2 */
    VEML7700_POWERSAVE_MODE3 = 2, /*!< Power saving mode 3 */
    VEML7700_POWERSAVE_MODE4 = 3, /*!< Power saving mode 4 */
} veml7700_power_save_t;

typedef enum 
{
    VEML7700_PERS_1 = 0, /*!< ALS irq persisance 1 sample */
    VEML7700_PERS_2 = 1, /*!< ALS irq persisance 2 samples */
    VEML7700_PERS_4 = 2, /*!< ALS irq persisance 4 samples */
    VEML7700_PERS_8 = 3, /*!< ALS irq persisance 8 samples */
} veml7700_irq_pre_t;

/**
 * @brief  VEML7700 Init structure definition.
 */
typedef struct
{
    veml7700_gain_t gain;      /*!< Sensor gain configuration */
    veml7700_integration_time_t integration_time; /*!< Sensor integration time configuration */
    veml7700_irq_pre_t persistance;      /*!< Last result persistance on-sensor configuration */
    uint16_t interrupt_enable; /*!< Enable/disable interrupts */
    uint16_t shutdown;         /*!< Shutdown command configuration */
    float resolution;          /*!< Current resolution and multiplier */
    uint32_t maximum_lux;      /*!< Current maximum lux limit */
} veml7700_config_t;

typedef struct
{
    uint16_t raw_light;
    uint16_t raw_white;
} veml7700_raw_data_t;

typedef struct
{
    i2c_bus_device_handle_t i2c_dev;
    uint8_t dev_addr;
    veml7700_config_t config;
    veml7700_raw_data_t raw_data;
    EventGroupHandle_t optimal_signal;
} veml7700_dev_t;

typedef void *veml7700_handle_t;


#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief   create veml7700 sensor device
     * @param   bus device object handle
     * @param   dev_addr device I2C address
     * @return
     *     - NULL if fail to create
     *     - Others handle of veml7700 device
     */
    veml7700_handle_t veml7700_create(i2c_bus_handle_t bus, uint8_t dev_addr);

    /**
     * @brief   delete veml7700 device
     * @param   sensor Point to veml7700 operate handle
     * @return
     *     - ESP_OK Success
     *     - ESP_FAIL Fail
     */
    esp_err_t veml7700_delete(veml7700_handle_t *sensor);
    esp_err_t veml7700_optimize_configuration(veml7700_handle_t sensor, float *lux);
    uint32_t veml7700_get_current_maximum_lux();
    uint32_t veml7700_get_lower_maximum_lux(veml7700_handle_t sensor, float *lux);
    uint32_t veml7700_get_lowest_maximum_lux();
    uint32_t veml7700_get_maximum_lux();
    int veml7700_get_gain_index(uint8_t gain);
    int veml7700_get_it_index(uint8_t integration_time);
    uint8_t indexOf(uint8_t elm, const uint8_t *ar, uint8_t len);
    void decrease_resolution(veml7700_handle_t sensor);
    void increase_resolution(veml7700_handle_t sensor);
    esp_err_t veml7700_send_config(veml7700_handle_t sensor);
    float veml7700_get_resolution(veml7700_handle_t sensor);

/**implements of light sensor hal interface**/
#ifdef CONFIG_SENSOR_LIGHT_INCLUDED_VEML7700
    /**
     * @brief initialize veml6040 with default configurations
     *
     * @param i2c_bus i2c bus handle the sensor will attached to
     * @return
     *     - ESP_OK Success
     *     - ESP_FAIL Fail
     */
    esp_err_t light_sensor_veml7700_init(i2c_bus_handle_t handle);

    /**
     * @brief de-initialize veml6040
     *
     * @return
     *     - ESP_OK Success
     *     - ESP_FAIL Fail
     */
    esp_err_t light_sensor_veml7700_deinit(void);

    /**
     * @brief test if veml6040 is active
     *
     * @return
     *     - ESP_OK Success
     *     - ESP_FAIL Fail
     */
    esp_err_t light_sensor_veml7700_test(void);

    /**
     * @brief Acquire light sensor ultra violet result one time.
     * light Ultraviolet includes UVA UVB and UV
     * @return
     *     - ESP_OK Success
     *     - ESP_FAIL Fail
     */
    esp_err_t light_sensor_veml7700_acquire_light(float *light, float *white);

#endif

#ifdef __cplusplus
}
#endif

#endif
