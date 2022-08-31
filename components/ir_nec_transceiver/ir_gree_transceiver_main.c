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

#define EXAMPLE_IR_RESOLUTION_HZ     1000000 // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_TX_GPIO_NUM       18
#define EXAMPLE_IR_RX_GPIO_NUM       19
#define EXAMPLE_IR_GREE_DECODE_MARGIN 200     // Tolerance for parsing RMT symbols into bit stream

/**
 * @brief GREE timing spec
 */
#define GREE_LEADING_CODE_DURATION_0  9000
#define GREE_LEADING_CODE_DURATION_1  4500
#define GREE_PAYLOAD_ZERO_DURATION_0  560
#define GREE_PAYLOAD_ZERO_DURATION_1  560
#define GREE_PAYLOAD_ONE_DURATION_0   560
#define GREE_PAYLOAD_ONE_DURATION_1   1690
#define GREE_REPEAT_CODE_DURATION_0   9000
#define GREE_REPEAT_CODE_DURATION_1   2250

static const char *TAG = "example";

/**
 * @brief Saving GREE decode results
 */
static uint16_t s_gree_code_address;
static uint16_t s_gree_code_command;

/**
 * @brief Check whether a duration is within expected range
 */
static inline bool gree_check_in_range(uint32_t signal_duration, uint32_t spec_duration)
{
    return (signal_duration < (spec_duration + EXAMPLE_IR_GREE_DECODE_MARGIN)) &&
           (signal_duration > (spec_duration - EXAMPLE_IR_GREE_DECODE_MARGIN));
}

/**
 * @brief Check whether a RMT symbol represents GREE logic zero
 */
static bool gree_parse_logic0(rmt_symbol_word_t *rmt_gree_symbols)
{
    return gree_check_in_range(rmt_gree_symbols->duration0, GREE_PAYLOAD_ZERO_DURATION_0) &&
           gree_check_in_range(rmt_gree_symbols->duration1, GREE_PAYLOAD_ZERO_DURATION_1);
}

/**
 * @brief Check whether a RMT symbol represents GREE logic one
 */
static bool gree_parse_logic1(rmt_symbol_word_t *rmt_gree_symbols)
{
    return gree_check_in_range(rmt_gree_symbols->duration0, GREE_PAYLOAD_ONE_DURATION_0) &&
           gree_check_in_range(rmt_gree_symbols->duration1, GREE_PAYLOAD_ONE_DURATION_1);
}

/**
 * @brief Decode RMT symbols into GREE address and command
 */
static bool gree_parse_frame(rmt_symbol_word_t *rmt_gree_symbols)
{
    rmt_symbol_word_t *cur = rmt_gree_symbols;
    uint16_t address = 0;
    uint16_t command = 0;
    bool valid_leading_code = gree_check_in_range(cur->duration0, GREE_LEADING_CODE_DURATION_0) &&
                              gree_check_in_range(cur->duration1, GREE_LEADING_CODE_DURATION_1);
    if (!valid_leading_code) {
        return false;
    }
    cur++;
    for (int i = 0; i < 16; i++) {
        if (gree_parse_logic1(cur)) {
            address |= 1 << i;
        } else if (gree_parse_logic0(cur)) {
            address &= ~(1 << i);
        } else {
            return false;
        }
        cur++;
    }
    for (int i = 0; i < 16; i++) {
        if (gree_parse_logic1(cur)) {
            command |= 1 << i;
        } else if (gree_parse_logic0(cur)) {
            command &= ~(1 << i);
        } else {
            return false;
        }
        cur++;
    }
    // save address and command
    s_gree_code_address = address;
    s_gree_code_command = command;
    return true;
}

/**
 * @brief Check whether the RMT symbols represent GREE repeat code
 */
static bool gree_parse_frame_repeat(rmt_symbol_word_t *rmt_gree_symbols)
{
    return gree_check_in_range(rmt_gree_symbols->duration0, GREE_REPEAT_CODE_DURATION_0) &&
           gree_check_in_range(rmt_gree_symbols->duration1, GREE_REPEAT_CODE_DURATION_1);
}

/**
 * @brief Decode RMT symbols into GREE scan code and print the result
 */
static void example_parse_gree_frame(rmt_symbol_word_t *rmt_gree_symbols, size_t symbol_num)
{
    printf("GREE frame start---\r\n");
    for (size_t i = 0; i < symbol_num; i++) {
        printf("{%d:%d},{%d:%d}\r\n", rmt_gree_symbols[i].level0, rmt_gree_symbols[i].duration0,
               rmt_gree_symbols[i].level1, rmt_gree_symbols[i].duration1);
    }
    printf("---GREE frame end: ");
    // decode RMT symbols
    switch (symbol_num) {
    case 34: // GREE normal frame
        if (gree_parse_frame(rmt_gree_symbols)) {
            printf("Address=%04X, Command=%04X\r\n\r\n", s_gree_code_address, s_gree_code_command);
        }
        break;
    case 2: // GREE repeat frame
        if (gree_parse_frame_repeat(rmt_gree_symbols)) {
            printf("Address=%04X, Command=%04X, repeat\r\n\r\n", s_gree_code_address, s_gree_code_command);
        }
        break;
    default:
        printf("Unknown GREE frame\r\n\r\n");
        break;
    }
}

static bool example_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void app_main(void)
{
    ESP_LOGI(TAG, "create RMT TX channel");
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
        .mem_block_symbols = 64, // amount of RMT symbols that the channel can store at a time
        .trans_queue_depth = 4,  // number of transactions that allowed to pending in the background, this example won't queue multiple transactions, so queue depth > 1 is sufficient
        .gpio_num = EXAMPLE_IR_TX_GPIO_NUM,
    };
    rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

    ESP_LOGI(TAG, "modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000, // 38KHz
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    // this example won't send GREE frames in a loop
    rmt_transmit_config_t transmit_config = {
        .loop_count = 0, // no loop
    };

    ESP_LOGI(TAG, "install IR GREE encoder");
    ir_gree_encoder_config_t gree_encoder_cfg = {
        .resolution = EXAMPLE_IR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t gree_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_ir_gree_encoder(&gree_encoder_cfg, &gree_encoder));

    ESP_LOGI(TAG, "enable RMT TX and RX channels");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    // save the received RMT symbols
    rmt_symbol_word_t raw_symbols[64]; // 64 symbols should be sufficient for a standard GREE frame
    rmt_rx_done_event_data_t rx_data;
    // ready to receive
    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));
    while (1) {
        // wait for RX done signal
        if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdPASS) {
            // parse the receive symbols and print the result
            example_parse_gree_frame(rx_data.received_symbols, rx_data.num_symbols);
            // start receive again
            ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));
        } else {
            // timeout, transmit predefined IR GREE packets
            const ir_gree_scan_code_t scan_code = {
                .address = 0x0440,
                .command = 0x3003,
            };
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, gree_encoder, &scan_code, sizeof(scan_code), &transmit_config));
        }
    }
}
