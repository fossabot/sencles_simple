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
#include "esp_wifi.h"
#include <sys/param.h>
#include "esp_sntp.h"
#include "time.h"
///////////////////////////////////////////////////////////////
#include "wifi_smart_config_main_task.h"
#include "lvgl_hw_gc9a01.h"
#include "sensor_hub_main_task.h"
#include "LinkSDK_main_task.h"
#include "led_strip_main_task.h"
#include "ir_gree_transceiver_main.h"
#include "main.h"

const char *TAG = "main";
EventGroupHandle_t all_event;

void app_main(void)
{
    all_event = xEventGroupCreate();
    //uint16_t i = 0;

    //xTaskCreatePinnedToCore(initialise_wifi_task, "initialise_wifi", 4096, NULL, 0, NULL, 1);
    //xTaskCreatePinnedToCore(gui_task, "gui", 4096 * 2, NULL, 2, NULL, 0);
    //xTaskCreatePinnedToCore(sensor_task, "sensor_hub", 4096, NULL, 0, NULL, 0);
    //xTaskCreatePinnedToCore(led_task, "led_strip", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ir_gree_transceiver_main_task, "gree_ir", 4096, NULL, 3, NULL, 1);

    /*while (1)
    {
        EventBits_t uxBits_wifi = xEventGroupWaitBits(all_event, BIT0_WIFI_READY, pdFALSE, pdTRUE, (TickType_t)0);

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
            xEventGroupSetBits(all_event, BIT2_NTP_READY);
            ESP_LOGI(TAG, "SNTP Finished! Ready to launch aliyun!");
            // xTaskCreatePinnedToCore(link_main, "aliyun", 4096, NULL, 0, NULL, 0);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        i++;
        ESP_LOGI(TAG, "Waiting for network! %ds are avilable!!", 15 - i);
        if (i == 15)
        {
            esp_wifi_stop();
            ESP_LOGI(TAG, "NO Network!!! Turning OFF WIFI!!!");
            break;
        }
    }*/
}
