/* RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "main_group_config.h"

static const char *TAG = "LED_STRIP";

led_strip_t *strip ;

#define RMT_TX_CHANNEL RMT_CHANNEL_0


void led_task(void* arg)
{
    (void) arg;
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 0));

    ESP_LOGI(TAG, "LED Start");
    while (true) 
    {
        EventBits_t uxBits_hot = xEventGroupWaitBits(all_event, BIT4_ENV_TOO_HOT, pdFALSE, pdTRUE, (TickType_t)0);
        uint32_t time = 0;

        if (uxBits_hot & (BIT4_ENV_TOO_HOT))
        {
            color_breathe(strip, red, 0, 2, 1, 40, 2, &time);
            vTaskDelay(pdMS_TO_TICKS(1000 - time));
        }else{

            color_breathe(strip, green, 0, 2, 1, 100, 1, &time);
            vTaskDelay(pdMS_TO_TICKS(3000 - time));
        }
    }
}

esp_err_t color_breathe(led_strip_t *strip, led_strip_color_t led_strip_color, uint8_t min_lighten, uint8_t max_lighten, uint8_t light_step, uint8_t chase_speed_ms ,uint8_t loop_num, uint32_t *time_minus)
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
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, red, 0, 0));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (red == max_lighten)
                {
                    goto down_red;
                }
            }
            down_red:   
            for ( red = max_lighten; red >= min_lighten; red -= light_step)
            {
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, red, 0, 0));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (red == min_lighten)
                {
                    loop_num_count++;
                    if(loop_num == loop_num_count)
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
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 0, green, 0));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (green == max_lighten)
                {
                    goto down_green;
                }
            }
            down_green:   
            for ( green = max_lighten; green >= min_lighten; green -= light_step)
            {
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 0, green, 0));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (green == min_lighten)
                {
                    loop_num_count++;
                    if(loop_num == loop_num_count)
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
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 0, 0, blue));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (blue == max_lighten)
                {
                    goto down_blue;
                }   
            }
            down_blue:   
            for (blue = max_lighten; blue >= min_lighten; blue -= light_step)
            {
                ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 0, 0, blue));
                ESP_ERROR_CHECK(strip->refresh(strip, 0));
                vTaskDelay(pdMS_TO_TICKS(chase_speed_ms));
                if (blue == min_lighten)
                {
                    loop_num_count++;
                    if(loop_num == loop_num_count)
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