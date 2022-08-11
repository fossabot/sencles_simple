#pragma once

#include <stdint.h>
#include "lvgl/lvgl.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void UI_APP(void);
