/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "ir_gree_encoder.h"

#define IR_RESOLUTION_HZ 1000000 // 1MHz resolution, 1 tick = 1us
#define IR_TX_GPIO_NUM 14

static const char *TAG = "GREE_IR";

void ir_gree_transceiver_main_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "create GREE RMT TX channel");
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = IR_RESOLUTION_HZ,
        .mem_block_symbols = 64, // amount of RMT symbols that the channel can store at a time
        .trans_queue_depth = 2,  // number of transactions that allowed to pending in the background
        .gpio_num = IR_TX_GPIO_NUM,
    };
    rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

    ESP_LOGI(TAG, "modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.5,
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

    while (1)
    {
        for (uint16_t i = 0; i < 100; i++)
        {
            GreeProtocol_t scan_code = {{0x10 + i, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, gree_encoder, &scan_code, sizeof(scan_code), &transmit_config));
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "transmiting ok %d", i);
        }
    }
}
