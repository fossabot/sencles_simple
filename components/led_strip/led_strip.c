/* RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "main_group_config.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "led_strip.h"

#define RMT_LED_STRIP_RESOLUTION_HZ CONFIG_STRIP_RESOLUTION_HZ // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      CONFIG_RMT_TX_GPIO


static const char *TAG = "led_strip";




void led_task(void* arg)
{
    (void) arg;

    uint8_t red = 0;
    uint8_t green = 0;
    //uint8_t blue = 0;
    rmt_channel_handle_t led_chan = NULL;
    rmt_encoder_handle_t led_encoder = NULL;


   ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    ESP_LOGI(TAG, "Start LED");


    while (true) 
    {
        //EventBits_t uxBits_hot = xEventGroupWaitBits(all_event, BIT4_ENV_TOO_HOT, pdFALSE, pdTRUE, (TickType_t)0);
        uint32_t time = 0;

        //if (uxBits_hot & (BIT4_ENV_TOO_HOT))
        //{
            color_breathe(led_chan, led_encoder, red, 0, 120, 5, 10, 1, &time, &tx_config);
            vTaskDelay(pdMS_TO_TICKS(10000 - time));
        //}else{

            //color_breathe(led_chan, led_encoder, green, 0, 20, 1, 1, 1, &time, &tx_config);
            //vTaskDelay(pdMS_TO_TICKS(30000 - time));
        //}
    }
}

esp_err_t color_breathe(rmt_channel_handle_t tx_channel, rmt_encoder_handle_t encoder, led_strip_color_t led_strip_color, uint8_t min_lighten, uint8_t max_lighten, uint8_t light_step, uint8_t chase_speed_ms ,uint8_t loop_num, uint32_t *time_minus, const rmt_transmit_config_t *config)
{
    uint8_t led_strip_pixels[((max_lighten + 1 - min_lighten) / light_step) * 3 * 2 * loop_num];

            uint8_t red = 0;
            uint8_t loop_num_count = 0;
            uint32_t start_time = esp_log_timestamp();
            uint32_t end_time = 0;

            for (uint8_t i = 0; i < (max_lighten + 1 - min_lighten); i+= 3)
            {   
                for (red = min_lighten; red <= max_lighten; red += light_step)
                {

                if (red == max_lighten)
                {
                    break;
                }
            led_strip_pixels[i + 0] = 0;
            led_strip_pixels[i + 1] = 0;
            led_strip_pixels[i + 2] = red;
                
            }

            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
   
    return ESP_OK;
}