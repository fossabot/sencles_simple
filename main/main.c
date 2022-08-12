

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/param.h>
#include "smart_config.h"
#include "gc9a01.h"
#include "sensor_main_task.h"

const char *TAG = "main";

void app_main(void)
{
    //xTaskCreatePinnedToCore(initialise_wifi_task, "initialise_wifi", 4096, NULL, 0, NULL, 1);
    //xTaskCreatePinnedToCore(gui_task, "gui", 4096*2, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(sensor_task, "sensor_hub", 4096, NULL, 0, NULL, 0);
    return;
}




