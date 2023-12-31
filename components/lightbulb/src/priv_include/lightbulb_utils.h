// Copyright 2020-2022 Espressif Systems (Shanghai) PTE LTD
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

#ifndef MAX
#define MAX(a, b)                                   (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)                                   (((a) < (b)) ? (a) : (b))
#endif

#if CONFIG_LIGHTBULB_CHECK_DEFAULT_LEVEL_3

#define LIGHTBULB_CHECK(a, str, action, ...)                                                \
    if (unlikely(!(a))) {                                                                   \
        ESP_LOGE(TAG, "%s:%d (%s): " str, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        action;                                                                             \
    }

#elif CONFIG_LIGHTBULB_CHECK_DEFAULT_LEVEL_2

#define LIGHTBULB_CHECK(a, str, action, ...)                                  \
    if (unlikely(!(a))) {                                                     \
        ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        action;                                                               \
    }

#else

#define LIGHTBULB_CHECK(a, str, action, ...)                                \
    if (unlikely(!(a))) {                                                   \
        ESP_LOGE(TAG, str, ##__VA_ARGS__);                                  \
        action;                                                             \
    }

#endif
