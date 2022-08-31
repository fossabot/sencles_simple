/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "ir_gree_encoder.h"

static const char *TAG = "gree_encoder";

typedef struct
{
    rmt_encoder_t base;                    // the base "class", declares the standard encoder interface
    rmt_encoder_t *copy_encoder;           // use the copy_encoder to encode the leading and ending pulse
    rmt_encoder_t *bytes_encoder;          // use the bytes_encoder to encode the address and command data
    rmt_symbol_word_t gree_leading_symbol;  // GREE leading code with RMT representation
    rmt_symbol_word_t gree_connect_symbol;  // GREE ending code with RMT representation
    rmt_symbol_word_t gree_lconnect_symbol; 
    rmt_symbol_word_t gree_gap_symbol; 
    int state;
} rmt_ir_gree_encoder_t;

static size_t rmt_encode_ir_gree(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;
    ir_gree_scan_code_t *scan_code = (ir_gree_scan_code_t *)primary_data;
    rmt_encoder_handle_t copy_encoder = gree_encoder->copy_encoder;
    rmt_encoder_handle_t bytes_encoder = gree_encoder->bytes_encoder;
    switch (gree_encoder->state)
    {
    case 0: // send leading code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_leading_symbol,
                                                sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 1; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 1: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->code, 4 * sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 2; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 2: // send identifier 0b010
        encoded_symbols += bytes_encoder->encode(copy_encoder, channel, &scan_code->command, sizeof(uint16_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 3; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 3: // send ending code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_ending_symbol,
                                                sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 0; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_ir_gree_encoder(rmt_encoder_t *encoder)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_del_encoder(gree_encoder->copy_encoder);
    rmt_del_encoder(gree_encoder->bytes_encoder);
    free(gree_encoder);
    return ESP_OK;
}

static esp_err_t rmt_ir_gree_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_encoder_reset(gree_encoder->copy_encoder);
    rmt_encoder_reset(gree_encoder->bytes_encoder);
    gree_encoder->state = 0;
    return ESP_OK;
}

esp_err_t rmt_new_ir_gree_encoder(const ir_gree_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_ir_gree_encoder_t *gree_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gree_encoder = calloc(1, sizeof(rmt_ir_gree_encoder_t));
    ESP_GOTO_ON_FALSE(gree_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for ir gree encoder");
    gree_encoder->base.encode = rmt_encode_ir_gree;
    gree_encoder->base.del = rmt_del_ir_gree_encoder;
    gree_encoder->base.reset = rmt_ir_gree_encoder_reset;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &gree_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    // construct the leading code and ending code with RMT symbol format
    gree_encoder->gree_leading_symbol = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = 9000ULL * config->resolution / 1000000,
        .level1 = 1,
        .duration1 = 4500ULL * config->resolution / 1000000,
    };
    gree_encoder->gree_connect_symbol = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = 680 * config->resolution / 1000000,
        .level1 = 1,
        .duration1 = 19975 * config->resolution / 1000000,
    };
    gree_encoder->gree_lconnect_symbol = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = 680 * config->resolution / 1000000,
        .level1 = 1,
        .duration1 = 19975 * 2 * config->resolution / 1000000,
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 510 * config->resolution / 1000000, // T0H=510us
            .level1 = 0,
            .duration1 = 680 * config->resolution / 1000000, // T0L=680us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 1530 * config->resolution / 1000000, // T1H=1530us
            .level1 = 0,
            .duration1 =  680 * config->resolution / 1000000, // T1L=680us
        },
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &gree_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");

    *ret_encoder = &gree_encoder->base;
    return ESP_OK;
err:
    if (gree_encoder)
    {
        if (gree_encoder->bytes_encoder)
        {
            rmt_del_encoder(gree_encoder->bytes_encoder);
        }
        if (gree_encoder->copy_encoder)
        {
            rmt_del_encoder(gree_encoder->copy_encoder);
        }
        free(gree_encoder);
    }
    return ret;
}
