#include <sys/cdefs.h>
// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
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

#include "esp_log.h"
#include "iot_sensor_hub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_main_task.h"
#include "lvgl_app.h"
#include "math.h"

#define SENSOR_PERIOD_MS 5000 

#define TAG "Sensors Monitor"

extern lv_obj_t * ui_humiArc;
extern lv_obj_t * ui_tempArc;
extern lv_obj_t * ui_tempLabelnum;
extern lv_obj_t * ui_humiLabelnum;
extern lv_obj_t * ui_btempLabelnum;


static void sensorEventHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    sensor_data_t *sensor_data = (sensor_data_t *)event_data;
    sensor_type_t sensor_type = (sensor_type_t)((sensor_data->sensor_id) >> 4 & SENSOR_ID_MASK);

    if (sensor_type >= SENSOR_TYPE_MAX) {
        ESP_LOGE(TAG, "sensor_id invalid, id=%d", sensor_data->sensor_id);
        return;
    }
    switch (id) {
        case SENSOR_STARTED:
            ESP_LOGI(TAG, "Timestamp = %llu - %s SENSOR_STARTED",
                     sensor_data->timestamp,
                     SENSOR_TYPE_STRING[sensor_type]);
            break;
        case SENSOR_STOPED:
            ESP_LOGI(TAG, "Timestamp = %llu - %s SENSOR_STOPED",
                     sensor_data->timestamp,
                     SENSOR_TYPE_STRING[sensor_type]);
            break;
        case SENSOR_TEMP_HUMI_DATA_READY:
            lv_label_set_text_fmt(ui_tempLabelnum, "%0.3f", sensor_data->humiture.temperature);
            lv_label_set_text_fmt(ui_humiLabelnum, "%0.3f", sensor_data->humiture.humidity);
            //sensor_data->humiture.temperature
            //sensor_data->humiture.humidity
            lv_arc_set_value(ui_tempArc, (int)sensor_data->humiture.temperature);
            lv_arc_set_value(ui_humiArc, (int)sensor_data->humiture.humidity);

            break;
        case SENSOR_ACCE_DATA_READY:
            ESP_LOGI(TAG, "Timestamp = %llu - SENSOR_ACCE_DATA_READY - "
                     "acce_x=%.2f, acce_y=%.2f, acce_z=%.2f",
                     sensor_data->timestamp,
                     sensor_data->acce.x, sensor_data->acce.y, sensor_data->acce.z);
            break;
        case SENSOR_GYRO_DATA_READY:
            ESP_LOGI(TAG, "Timestamp = %llu - SENSOR_GYRO_DATA_READY - "
                     "gyro_x=%.2f, gyro_y=%.2f, gyro_z=%.2f",
                     sensor_data->timestamp,
                     sensor_data->gyro.x, sensor_data->gyro.y, sensor_data->gyro.z);
            break;
        case SENSOR_LIGHT_DATA_READY:
            ESP_LOGI(TAG, "Timestamp = %llu - SENSOR_LIGHT_DATA_READY - "
                     "light=%.2f",
                     sensor_data->timestamp,
                     sensor_data->light);
            break;
        case SENSOR_RGBW_DATA_READY:
            ESP_LOGI(TAG, "Timestamp = %llu - SENSOR_RGBW_DATA_READY - "
                     "r=%.2f, g=%.2f, b=%.2f, w=%.2f",
                     sensor_data->timestamp,
                     sensor_data->rgbw.r, sensor_data->rgbw.r, sensor_data->rgbw.b, sensor_data->rgbw.w);
            break;
        case SENSOR_UV_DATA_READY:
            ESP_LOGI(TAG, "Timestamp = %llu - SENSOR_UV_DATA_READY - "
                     "uv=%.2f, uva=%.2f, uvb=%.2f",
                     sensor_data->timestamp,
                     sensor_data->uv.uv, sensor_data->uv.uva, sensor_data->uv.uvb);
            break;
        default:
            ESP_LOGI(TAG, "Timestamp = %llu - event id = %d", sensor_data->timestamp, id);
            break;
    }
    float temp_body = 1.07*(sensor_data->humiture.temperature) + 0.2*((sensor_data->humiture.humidity)/100.0)*6.105*exp((17.27*(sensor_data->humiture.temperature))/(237.7+(sensor_data->humiture.temperature))) - 2.7;
    lv_label_set_text_fmt(ui_btempLabelnum, "%0.3f", temp_body);
}

void sensor_task(void *args)
{
    (void) args;

    ESP_LOGI(TAG,"HELLO");
    /*create the i2c0 bus handle with a resource ID*/
    i2c_config_t i2c_conf;
    
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = GPIO_NUM_35;
    i2c_conf.scl_io_num = GPIO_NUM_36;
    i2c_conf.sda_pullup_en = true;
    i2c_conf.scl_pullup_en = true;
    i2c_conf.master.clk_speed = 40000;

    bus_handle_t i2c0_bus_handle = i2c_bus_create(I2C_NUM_0, &i2c_conf);
    if (i2c0_bus_handle == NULL) {
        goto error_loop;
    }

    /*register handler with NULL specific typeID, thus all events posted to sensor_loop will be handled*/
    ESP_ERROR_CHECK(iot_sensor_handler_register_with_type(NULL_ID, NULL_ID, sensorEventHandler, NULL));

    /*create sensors based on sensor scan result*/
    sensor_info_t* sensor_infos[10];
    sensor_handle_t sensor_handle[10] = {NULL};
    sensor_config_t sensor_config = {
        .bus = i2c0_bus_handle, /*which bus sensors will connect to*/
        .mode = MODE_POLLING, /*data acquire mode*/
        .min_delay = SENSOR_PERIOD_MS /*data acquire period*/
    };
    int num = iot_sensor_scan(i2c0_bus_handle, sensor_infos, 10); /*scan for valid sensors based on active i2c address*/
    for (size_t i = 0; i < num && i<10; i++) {

        if (ESP_OK != iot_sensor_create(sensor_infos[i]->sensor_id, &sensor_config, &sensor_handle[i])) { /*create a sensor with specific sensor_id and configurations*/
            goto error_loop;
        }

        iot_sensor_start(sensor_handle[i]); /*start a sensor, data ready events will be posted once data acquired successfully*/
        ESP_LOGI(TAG,"%s (%s) created",sensor_infos[i]->name, sensor_infos[i]->desc);
    }

    while (1) {
        vTaskDelay(2000/portTICK_RATE_MS);
    }

error_loop:
    ESP_LOGE(TAG, "ERROR");
    while (1) {
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}