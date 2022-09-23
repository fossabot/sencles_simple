// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
// Copyright 2022 JeongYeham
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
extern "C"
{
#endif

#include "esp_err.h"
#include "driver/rmt_tx.h"

    typedef enum _led_strip_color_type
    {
        red,
        green,
        blue,
    } led_strip_color_t;

    void led_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
