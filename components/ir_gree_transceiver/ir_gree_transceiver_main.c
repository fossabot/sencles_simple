/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "ir_gree_encoder.h"
#include "ir_gree_transceiver_main.h"
#include "sensor_hub_main_task.h"
#include "main.h"

//extern EventGroupHandle_t all_event;

//QueueHandle_t xQueueACData;

#define IR_RESOLUTION_HZ 1000000 // 1MHz resolution, 1 tick = 1us
#define IR_TX_GPIO_NUM 47

static const char *TAG = "GREE_IR";

void ir_gree_transceiver_main_task(void *pvParameters)
{
    all_signals_t *signal = (all_signals_t *)pvParameters;

    ESP_LOGI(TAG, "create GREE RMT TX channel");
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = IR_RESOLUTION_HZ,
        .mem_block_symbols = 128, // amount of RMT symbols that the channel can store at a time
        .trans_queue_depth = 2,   // number of transactions that allowed to pending in the background
        .gpio_num = IR_TX_GPIO_NUM,
    };
    rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

    ESP_LOGI(TAG, "modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.50,
        .frequency_hz = 38000, // 38KHz
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    // this won't send GREE frames in a loop
    rmt_transmit_config_t transmit_config = {
        .loop_count = 0, // no loop
    };

    ESP_LOGI(TAG, "install IR GREE encoder");
    ir_gree_encoder_config_t gree_encoder_cfg = {
        .resolution = IR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t gree_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_ir_gree_encoder(&gree_encoder_cfg, &gree_encoder));

    ESP_LOGI(TAG, "enable RMT TX channels");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    GreeProtocol_t code = {{0}};
    stateReset(&code);

    while (1)
    {
        EventBits_t uxBits_hot = xEventGroupWaitBits(signal->all_event, BIT4_ENV_TOO_HOT, pdFALSE, pdTRUE, (TickType_t)0);
        EventBits_t uxBits_ac = xEventGroupWaitBits(signal->all_event, BIT5_GREE_AC_ON, pdFALSE, pdTRUE, (TickType_t)0);
        if ((uxBits_hot & (BIT4_ENV_TOO_HOT)) ^ (uxBits_ac & (BIT5_GREE_AC_ON)))
        {

            ESP_LOGI(TAG, "XOR DONE");
            if (uxBits_hot & (BIT4_ENV_TOO_HOT) && !(uxBits_ac & (BIT5_GREE_AC_ON)))
            {
                setPower(&code, true);
                setFan(&code, 0);
                setMode(&code, 1);
                setTemp(&code, 25);
                setSwingVertical(&code, true, 0);
                setLight(&code, true);
                procotol_fixup(&code);
                xQueueSend(signal->xQueueACData, &code, (TickType_t)0);
                ESP_ERROR_CHECK(rmt_transmit(tx_channel, gree_encoder, &code, sizeof(code), &transmit_config));
/*                 ESP_LOGI(TAG,"Power is % \n ",);
                getPower(GreeProtocol_t *greeproc)
                getTemp(GreeProtocol_t *greeproc)
                getFan(GreeProtocol_t *greeproc)
                getMode(GreeProtocol_t *greeproc) */
                xEventGroupSetBits(signal->all_event, BIT5_GREE_AC_ON);
            }
            if(!(uxBits_hot & (BIT4_ENV_TOO_HOT)) && uxBits_ac & (BIT5_GREE_AC_ON))
            {
                setTemp(&code, 27);
                procotol_fixup(&code);
                xQueueSend(signal->xQueueACData, &code, (TickType_t)0);
                ESP_ERROR_CHECK(rmt_transmit(tx_channel, gree_encoder, &code, sizeof(code), &transmit_config));
                xEventGroupClearBits(signal->all_event, BIT5_GREE_AC_ON);
            }
            vTaskDelay(pdMS_TO_TICKS(10*60000));
        }
        else
        {
            ESP_LOGI(TAG, "XOR PASS");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
