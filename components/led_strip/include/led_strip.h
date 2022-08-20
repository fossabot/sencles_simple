// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "driver/rmt_tx.h"

typedef struct 
{
    uint8_t red  ;
    uint8_t green;
    uint8_t blue ;
} led_strip_color_t;


void led_task(void* arg);
esp_err_t color_breathe(rmt_channel_handle_t tx_channel, rmt_encoder_handle_t encoder, led_strip_color_t led_strip_color, uint8_t min_lighten, uint8_t max_lighten, uint8_t light_step, uint8_t chase_speed_ms, uint8_t loop_num, uint32_t *time_minus, const rmt_transmit_config_t *config);

#ifdef __cplusplus
}
#endif
