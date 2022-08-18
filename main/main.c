

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include <sys/param.h>
#include "smart_config.h"
#include "gc9a01.h"
#include "sensor_main_task.h"
#include "link_main.h"
#include "esp_sntp.h"
#include "time.h"
#include "led_strip.h"
#include "main_group_config.h"


const char *TAG = "main";
EventGroupHandle_t all_event;

void app_main(void)
{
    all_event = xEventGroupCreate();
    uint16_t i = 0;


    xTaskCreatePinnedToCore(initialise_wifi_task, "initialise_wifi", 4096, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(gui_task, "gui", 4096*2, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(sensor_task, "sensor_hub", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(led_task, "led_strip", 4096, NULL, 3, NULL, 1);

    while (1)
    {
        EventBits_t uxBits_wifi = xEventGroupWaitBits(all_event, BIT0_WIFI_READY, pdFALSE, pdTRUE, (TickType_t)0);

        if (uxBits_wifi & (BIT0_WIFI_READY))
        {
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
            //xTaskCreatePinnedToCore(link_main, "aliyun", 4096, NULL, 0, NULL, 0);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        i++;
        ESP_LOGI(TAG, "Waiting for network! %ds are avilable!!", 15-i);
        if (i == 15)
        {
            esp_wifi_stop();
            ESP_LOGI(TAG, "NO Network!!! Turning OFF WIFI!!!");
            break;
        }
    }
    return ;
}




