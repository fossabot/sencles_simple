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

#ifndef _MAIN_GROUP_CONFIG_H_
#define _MAIN_GROUP_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

    extern EventGroupHandle_t all_event;

#define BIT0_WIFI_READY BIT0
#define BIT1_SC_OVER BIT1
#define BIT2_NTP_READY BIT2
#define BIT3_ALIYUN_CONNECTED BIT3
#define BIT4_ENV_TOO_HOT BIT4
#define BIT5_GREE_AC_ON BIT5

#ifdef __cplusplus
}
#endif

#endif
