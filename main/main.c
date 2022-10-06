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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include <sys/param.h>
#include "esp_sntp.h"
#include "time.h"
///////////////////////////////////////////////////////////////
#include "wifi_smart_config_main_task.h"
#include "lvgl_hw_main_task.h"
#include "sensor_hub_main_task.h"
#include "LinkSDK_main_task.h"
#include "led_strip_main_task.h"
#include "ir_gree_transceiver_main.h"
#include "ir_gree_encoder.h"
#include "typec_pd_main_task.h"
#include "main.h"
#include "driver/temperature_sensor.h"

const char *TAG = "main";
all_signals_t signal_de;

TaskHandle_t wifi_task_handle;
TaskHandle_t sensor_task_handle;
TaskHandle_t gui_task_handle;
TaskHandle_t led_task_handle;
TaskHandle_t gree_task_handle;
TaskHandle_t aliyun_task_handle;
TaskHandle_t typec_pd_task_handle;

void app_main(void)
{
    signal_de.all_event = xEventGroupCreate();
    signal_de.xQueueSenData = xQueueCreate(1, sizeof(sensor_data_t));
    signal_de.xQueueACData = xQueueCreate(1, sizeof(GreeProtocol_t));

    all_signals_t *signal = &signal_de;

    xEventGroupClearBits(signal->all_event, 0xff);

    xTaskCreatePinnedToCore(initialise_wifi_task, "initialise_wifi", 4096, signal, 0, &wifi_task_handle, 0);
    xTaskCreatePinnedToCore(sensor_task, "sensor_hub", 4096, signal, 0, &sensor_task_handle, 0);
    xTaskCreatePinnedToCore(gui_task, "gui", 4096 * 2, signal, 2, &gui_task_handle, 1);
    xTaskCreatePinnedToCore(led_task, "led_strip", 4096, signal, 3, &led_task_handle, 1);
    //xTaskCreatePinnedToCore(ir_gree_transceiver_main_task, "gree_ir", 4096, signal, 3, &gree_task_handle, 0);
    //xTaskCreatePinnedToCore(typec_pd_main_task, "typec_pd", 4096, signal, 3, &typec_pd_task_handle, 0);

    while (1)
    {
        EventBits_t uxBits_wifi = xEventGroupWaitBits(signal->all_event, BIT0_WIFI_READY, pdFALSE, pdTRUE, (TickType_t)0);

        if (uxBits_wifi & (BIT0_WIFI_READY))
        {
            ESP_LOGI(TAG, "Network found, prepare to connect SNTP and aliyun");
            setenv("TZ", "EST-8", 1);
            tzset();
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
            sntp_setservername(0, "ntp1.aliyun.com");
            sntp_setservername(1, "ntp2.aliyun.com");
            sntp_setservername(2, "ntp3.aliyun.com");
            sntp_init();
            xEventGroupSetBits(signal->all_event, BIT2_NTP_READY);
            ESP_LOGI(TAG, "SNTP Finished! Ready to launch aliyun!");
            // xTaskCreatePinnedToCore(link_main, "aliyun", 4096, signal, 0, &aliyun_task_handle, 1);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    }
}
