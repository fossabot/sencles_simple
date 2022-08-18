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

#define BIT0_WIFI_READY        BIT0
#define BIT1_SC_OVER           BIT1
#define BIT2_NTP_READY         BIT2
#define BIT3_ALIYUN_CONNECTED  BIT3 
#define BIT4_ENV_TOO_HOT       BIT4








#ifdef __cplusplus
}
#endif

#endif
