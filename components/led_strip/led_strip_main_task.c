// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "main.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "led_strip_main_task.h"

#define RMT_LED_STRIP_RESOLUTION_HZ CONFIG_STRIP_RESOLUTION_HZ // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM CONFIG_RMT_TX_GPIO

static const char *TAG = "led_strip";
static uint8_t led_strip_pixels[3];
esp_err_t color_breathe(rmt_channel_handle_t tx_channel, rmt_encoder_handle_t encoder, led_strip_color_t led_strip_color, uint8_t min_lighten, uint8_t max_lighten, uint8_t light_step, uint8_t chase_speed_ms, uint8_t loop_num, uint32_t *time_minus, rmt_transmit_config_t *config);

void led_task(void *arg)
{
    (void)arg;

    // uint32_t red = 0;
    // uint32_t green = 0;
    // uint32_t blue = 0;

    rmt_channel_handle_t led_chan = NULL;
    rmt_encoder_handle_t led_encoder = NULL;

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 2, // set the number of transactions that can be pending in the background
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
    while (1)
    {
        EventBits_t uxBits_hot = xEventGroupWaitBits(all_event, BIT4_ENV_TOO_HOT, pdFALSE, pdTRUE, (TickType_t)0);
        uint32_t time = 0;

        if (uxBits_hot & (BIT4_ENV_TOO_HOT))
        {
            ESP_ERROR_CHECK(color_breathe(led_chan, led_encoder, red, 0, 2, 1, 50, 1, &time, &tx_config));
            vTaskDelay(pdMS_TO_TICKS(1000 - time));
        }
        else
        {

            ESP_ERROR_CHECK(color_breathe(led_chan, led_encoder, green, 0, 5, 1, 100, 2, &time, &tx_config));
            vTaskDelay(pdMS_TO_TICKS(3000 - time));
        }
    }
}

esp_err_t color_breathe(rmt_channel_handle_t tx_channel, rmt_encoder_handle_t encoder, led_strip_color_t led_strip_color, uint8_t min_lighten, uint8_t max_lighten, uint8_t light_step, uint8_t chase_speed_ms, uint8_t loop_num, uint32_t *time_minus, rmt_transmit_config_t *config)
{
    switch (led_strip_color)
    {
    case red:
    {
        uint8_t red = 0;
        uint8_t loop_num_count = 0;
        uint32_t start_time = esp_log_timestamp();
        uint32_t end_time = 0;
    up_red:
        for (red = min_lighten; red <= max_lighten; red += light_step)
        {
            led_strip_pixels[0] = 0;   // green
            led_strip_pixels[1] = red; // red
            led_strip_pixels[2] = 0;   // blue

            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (red == max_lighten)
            {
                goto down_red;
            }
        }
    down_red:
        for (red = max_lighten; red >= min_lighten; red -= light_step)
        {
            led_strip_pixels[0] = 0;   // green
            led_strip_pixels[1] = red; // red
            led_strip_pixels[2] = 0;   // blue

            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (red == min_lighten)
            {
                loop_num_count++;
                if (loop_num == loop_num_count)
                {
                    end_time = esp_log_timestamp();
                    *time_minus = end_time - start_time;
                    break;
                }
                goto up_red;
            }
        }
    }
    break;
    case green:
    {
        uint8_t green = 0;
        uint8_t loop_num_count = 0;
        uint32_t start_time = esp_log_timestamp();
        uint32_t end_time = 0;
    up_green:
        for (green = min_lighten; green <= max_lighten; green += light_step)
        {
            led_strip_pixels[0] = green; // green
            led_strip_pixels[1] = 0;     // red
            led_strip_pixels[2] = 0;     // blue
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (green == max_lighten)
            {
                goto down_green;
            }
        }
    down_green:
        for (green = max_lighten; green >= min_lighten; green -= light_step)
        {
            led_strip_pixels[0] = green; // green
            led_strip_pixels[1] = 0;     // red
            led_strip_pixels[2] = 0;     // blue

            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (green == min_lighten)
            {
                loop_num_count++;
                if (loop_num == loop_num_count)
                {
                    end_time = esp_log_timestamp();
                    *time_minus = end_time - start_time;
                    break;
                }
                goto up_green;
            }
        }
    }
    break;
    case blue:
    {
        uint8_t blue = 0;
        uint8_t loop_num_count = 0;
        uint32_t start_time = esp_log_timestamp();
        uint32_t end_time = 0;
    up_blue:
        for (blue = min_lighten; blue <= max_lighten; blue += light_step)
        {
            led_strip_pixels[0] = 0;    // green
            led_strip_pixels[1] = 0;    // red
            led_strip_pixels[2] = blue; // blue
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (blue == max_lighten)
            {
                goto down_blue;
            }
        }
    down_blue:
        for (blue = max_lighten; blue >= min_lighten; blue -= light_step)
        {
            led_strip_pixels[0] = 0;    // green
            led_strip_pixels[1] = 0;    // red
            led_strip_pixels[2] = blue; // blue
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
            if (blue == min_lighten)
            {
                loop_num_count++;
                if (loop_num == loop_num_count)
                {
                    end_time = esp_log_timestamp();
                    *time_minus = end_time - start_time;
                    break;
                }
                goto up_blue;
            }
        }
    }
    break;
    default:
        break;
    }
    return ESP_OK;
}