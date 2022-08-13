// Copyright 2022-2023 Jeong Yeham
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
#include "esp_timer.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lvgl_app.h"

extern lv_obj_t * ui_timeLabel;
extern lv_obj_t * ui_count;


extern EventGroupHandle_t all_event;

static void count_dtimer_callback(void* arg);

static const char* TAG = "dtimer";

time_t curtime;
static time_t examtime = 1671843600;

void dtimer_task(void* arg)
{
    (void) arg;

    const esp_timer_create_args_t count_dtimer_args = {
            .callback = &count_dtimer_callback,
            .name = "count_dtimer"
    };
    esp_timer_handle_t count_dtimer;
    ESP_ERROR_CHECK(esp_timer_create(&count_dtimer_args, &count_dtimer));




    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(count_dtimer, 1000000));
    ESP_LOGI(TAG, "Started  timer, time since boot: %lld us", esp_timer_get_time());


    while (1)
    {

    }
    

    /* impossible run here */
    ESP_ERROR_CHECK(esp_timer_stop(count_dtimer));   
    ESP_ERROR_CHECK(esp_timer_delete(count_dtimer));
    ESP_LOGE(TAG, "dtimer ERROR!!! Stopped and deleted timers");
}

static void count_dtimer_callback(void* arg)
{
    (void) arg;
    time(&curtime);
    uint32_t difft = (uint32_t)difftime(examtime, curtime);
    //lv_label_set_text_fmt(ui_timeLabel, "%s", ctime(&curtime));
    //lv_label_set_text_fmt(ui_count, "%f", difft);
}

