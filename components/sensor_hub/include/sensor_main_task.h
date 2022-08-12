#ifndef _SENSORS_MAIM_TASK_H_
#define _SENSORS_MAIN_TASK_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif



void sensor_task(void *args);


#ifdef __cplusplus
}
#endif

#endif