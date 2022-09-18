/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "string.h"
#include "ir_gree_encoder.h"

static const char *TAG = "gree_encoder";

#define GETBITS8(data, offset, size) \
    (((data) & (((uint8_t)UINT8_MAX >> (8 - (size))) << (offset))) >> (offset))

// Constants
const uint8_t kKelvinatorAuto = 0; // (temp = 25C)
const uint8_t kKelvinatorCool = 1;
const uint8_t kKelvinatorDry = 2; // (temp = 25C, but not shown)
const uint8_t kKelvinatorFan = 3;
const uint8_t kKelvinatorHeat = 4;
const uint8_t kKelvinatorBasicFanMax = 3;
const uint8_t kKelvinatorFanAuto = 0;
const uint8_t kKelvinatorFanMin = 1;
const uint8_t kKelvinatorFanMax = 5;
const uint8_t kKelvinatorMinTemp = 16;  // 16C
const uint8_t kKelvinatorMaxTemp = 30;  // 30C
const uint8_t kKelvinatorAutoTemp = 25; // 25C

const uint8_t kKelvinatorSwingVOff = 0b0000;         // 0
const uint8_t kKelvinatorSwingVAuto = 0b0001;        // 1
const uint8_t kKelvinatorSwingVHighest = 0b0010;     // 2
const uint8_t kKelvinatorSwingVUpperMiddle = 0b0011; // 3
const uint8_t kKelvinatorSwingVMiddle = 0b0100;      // 4
const uint8_t kKelvinatorSwingVLowerMiddle = 0b0101; // 5
const uint8_t kKelvinatorSwingVLowest = 0b0110;      // 6
const uint8_t kKelvinatorSwingVLowAuto = 0b0111;     // 7
const uint8_t kKelvinatorSwingVMiddleAuto = 0b1001;  // 9
const uint8_t kKelvinatorSwingVHighAuto = 0b1011;    // 11

typedef struct
{
    rmt_encoder_t base;                    // the base "class", declares the standard encoder interface
    rmt_encoder_t *copy_encoder;           // use the copy_encoder to encode the leading and ending pulse
    rmt_encoder_t *bytes_encoder;          // use the bytes_encoder to encode the data
    rmt_symbol_word_t gree_leading_symbol; // GREE leading code with RMT representation
    rmt_symbol_word_t gree_connect_symbol; // GREE connect code with RMT representation
    rmt_symbol_word_t gree_lconnect_symbol;// GREE long connect code with RMT representation
    rmt_symbol_word_t gree_ending_symbol;  // GREE ending code with RMT representation

    rmt_symbol_word_t gree_low_level;
    rmt_symbol_word_t gree_high_level;
    int state;
} rmt_ir_gree_encoder_t;

static size_t rmt_encode_ir_gree(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;
    GreeProtocol_t *scan_code = (GreeProtocol_t *)primary_data;
    rmt_encoder_handle_t copy_encoder = gree_encoder->copy_encoder;
    rmt_encoder_handle_t bytes_encoder = gree_encoder->bytes_encoder;

    switch (gree_encoder->state)
    {
    case 0: // send leading code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_leading_symbol, sizeof(rmt_symbol_word_t), &session_state);
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
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[0], sizeof(uint8_t), &session_state);
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
    case 2: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[1], sizeof(uint8_t), &session_state);
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
    case 3: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[2], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 4; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 4: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[3], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 5; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 5: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_low_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 6;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
    case 6: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_high_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 7;
        }

        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 7: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_low_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 8;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 8: // send connect code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_connect_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 9;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 9: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[4], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 10; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 10: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[5], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 11; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 11: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[6], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 12; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 12: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[7], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 13; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 13: // send long connect code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_lconnect_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 14;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 14: // send SECOND leading code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_leading_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 15; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 15: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[8], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 16; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
    case 16: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[9], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 17; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
    case 17: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[10], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 18; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
    case 18: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[11], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 19; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 19: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_low_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 20;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 20: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_high_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 21;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 21: // send identifier 0b010
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_low_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 22;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 22: // send connect code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_connect_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 23;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through

    case 23: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[12], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 24; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }

    // fall-through
    case 24: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[13], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 25; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }

        // fall-through

    case 25: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[14], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 26; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }

        // fall-through

    case 26: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[15], sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 27; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }

        // fall-through

    case 27: // send end code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_ending_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 0;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
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
        .level0 = 1,
        .duration0 = 9000ULL * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 4500ULL * config->resolution / 1000000,
    };
    gree_encoder->gree_connect_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 620 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 20000ULL * config->resolution / 1000000,
    };
    gree_encoder->gree_lconnect_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 620 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 40000ULL * config->resolution / 1000000,
    };
    gree_encoder->gree_low_level = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 620 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 580 * config->resolution / 1000000,
    };
        gree_encoder->gree_high_level = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 620 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 1680 * config->resolution / 1000000,
    };
    gree_encoder->gree_ending_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 620 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 1000,
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 620 * config->resolution / 1000000, // T0H=610us
            .level1 = 0,
            .duration1 = 510 * config->resolution / 1000000, // T0L=580us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 620 * config->resolution / 1000000, // T1H=610us
            .level1 = 0,
            .duration1 = 1660 * config->resolution / 1000000, // T1L=1680us
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

/// Reset the internals of the object to a known good state.
void stateReset(GreeProtocol_t *greeproc)
{
    for (uint8_t i = 0; i < 16; i++)
        greeproc->raw[i] = 0x00;
    greeproc->raw[3] = 0x50;
    greeproc->raw[11] = 0x70;
}

/// Set the raw state of the object.
/// @param[in] new_code The raw state from the native IR message.
void setRaw(GreeProtocol_t *greeproc, uint8_t new_code[])
{
    memcpy(greeproc->raw, new_code, 16);
}

/// Calculate the checksum for a given block of state.
/// @param[in] block A pointer to a block to calc the checksum of.
/// @param[in] length Length of the block array to checksum.
/// @return The calculated checksum value.
/// @note Many Bothans died to bring us this information.
uint8_t calcBlockChecksum(uint8_t *block)
{
    uint8_t sum = 0b1010;
    // Sum the lower half of the first 4 bytes of this block.
    for (uint8_t i = 0; i < 4 ; i++, block++)
        sum += (*block & 0b1111);
    // then sum the upper half of the next 3 bytes.
    for (uint8_t i = 4; i < 8 - 1; i++, block++)
        sum += (*block >> 4);
    // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
    return sum & 0b1111;
}

/// Calculate the checksum for the internal state.
void checksum(GreeProtocol_t *greeproc)
{
    greeproc->GreeProtocol_bit_t.Sum1 = calcBlockChecksum(greeproc->raw);
    greeproc->GreeProtocol_bit_t.Sum2 = calcBlockChecksum(greeproc->raw + 8);
}

/// Fix up any odd conditions for the current state.
void procotol_fixup(GreeProtocol_t *greeproc)
{
    // X-Fan mode is only valid in COOL or DRY modes.
    if (greeproc->GreeProtocol_bit_t.Mode != kKelvinatorCool && greeproc->GreeProtocol_bit_t.Mode != kKelvinatorDry)
        setXFan(greeproc, false);
    // Duplicate to the 2nd command chunk.
    greeproc->raw[8] = greeproc->raw[0];
    greeproc->raw[9] = greeproc->raw[1];
    greeproc->raw[10] = greeproc->raw[2];
    checksum(greeproc); // Calculate the checksums
}

/// Get the raw state of the object, suitable to be sent with the appropriate
/// IRsend object method.
/// @return A PTR to the internal state.
uint8_t *getRaw(GreeProtocol_t *greeproc)
{
    procotol_fixup(greeproc); // Ensure correct settings before sending.
    return greeproc->raw;
}

/// Verify the checksum is valid for a given state.
/// @param[in] state The array to verify the checksum of.
/// @param[in] length The size of the state.
/// @return A boolean indicating if it is valid.
bool validChecksum(uint8_t state[])
{
    for (uint16_t offset = 0; offset + 7 < 16; offset += 8)
    {
        // Top 4 bits of the last byte in the block is the block's checksum.
        if (GETBITS8(state[offset + 7], 4, 4) !=
            calcBlockChecksum(state + offset))
            return false;
    }
    return true;
}

/// Set the internal state to have the desired power.
/// @param[in] on The desired power state.
void setPower(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.Power = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.Power = 0;
    }
}

/// Get the power setting from the internal state.
/// @return A boolean indicating if the power setting.
bool getPower(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Power;
}

/// Set the temperature setting.
/// @param[in] degrees The temperature in degrees celsius.
void setTemp(GreeProtocol_t *greeproc, uint8_t degrees)
{
    uint8_t temp = kKelvinatorMinTemp >= degrees ? kKelvinatorMinTemp : degrees;
    temp = kKelvinatorMaxTemp <= temp ? kKelvinatorMaxTemp : temp;
    greeproc->GreeProtocol_bit_t.Temp = temp - kKelvinatorMinTemp;
}

/// Get the current temperature setting.
/// @return Get current setting for temp. in degrees celsius.
uint8_t getTemp(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Temp + kKelvinatorMinTemp;
}

/// Set the speed of the fan.
/// @param[in] speed 0 is auto, 1-5 is the speed
void setFan(GreeProtocol_t *greeproc, uint8_t speed)
{
    uint8_t fan = kKelvinatorFanMax <= speed ? kKelvinatorFanMax : speed; // Bounds check

    // Only change things if we need to.
    if (fan != greeproc->GreeProtocol_bit_t.Fan)
    {
        // Set the basic fan values.
        greeproc->GreeProtocol_bit_t.BasicFan = kKelvinatorBasicFanMax <= fan ? kKelvinatorBasicFanMax : fan;
        // Set the advanced(?) fan value.
        greeproc->GreeProtocol_bit_t.Fan = fan;
        // Turbo mode is turned off if we change the fan settings.
        setTurbo(greeproc, false);
    }
}

/// Get the current fan speed setting.
/// @return The current fan speed.
uint8_t getFan(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Fan;
}

/// Get the current operation mode setting.
/// @return The current operation mode.
uint8_t getMode(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Mode;
}

/// Set the desired operation mode.
/// @param[in] mode The desired operation mode.
void setMode(GreeProtocol_t *greeproc, uint8_t mode)
{
    switch (mode)
    {
    case kKelvinatorAuto:
    case kKelvinatorDry:
        // When the remote is set to Auto or Dry, it defaults to 25C and doesn't
        // show it.
        setTemp(greeproc, kKelvinatorAutoTemp);
        // FALL-THRU
    case kKelvinatorHeat:
    case kKelvinatorCool:
    case kKelvinatorFan:
        greeproc->GreeProtocol_bit_t.Mode = mode;
        break;
    default:
        setTemp(greeproc, kKelvinatorAutoTemp);
        greeproc->GreeProtocol_bit_t.Mode = kKelvinatorAuto;
        break;
    }
}

/// Set the Vertical Swing mode of the A/C.
/// @param[in] automatic Do we use the automatic setting?
/// @param[in] position The position/mode to set the vanes to.
void setSwingVertical(GreeProtocol_t *greeproc, bool automatic, uint8_t position)
{
    greeproc->GreeProtocol_bit_t.SwingAuto = (automatic || greeproc->GreeProtocol_bit_t.SwingH);
    uint8_t new_position = position;
    if (!automatic)
    {
        switch (position)
        {
        case kKelvinatorSwingVHighest:
        case kKelvinatorSwingVUpperMiddle:
        case kKelvinatorSwingVMiddle:
        case kKelvinatorSwingVLowerMiddle:
        case kKelvinatorSwingVLowest:
            break;
        default:
            new_position = kKelvinatorSwingVOff;
        }
    }
    else
    {
        switch (position)
        {
        case kKelvinatorSwingVAuto:
        case kKelvinatorSwingVLowAuto:
        case kKelvinatorSwingVMiddleAuto:
        case kKelvinatorSwingVHighAuto:
            break;
        default:
            new_position = kKelvinatorSwingVAuto;
        }
    }
    greeproc->GreeProtocol_bit_t.SwingV = new_position;
}

/// Get the Vertical Swing Automatic mode setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool getSwingVerticalAuto(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.SwingV & 0b0001;
}

/// Get the Vertical Swing position setting of the A/C.
/// @return The native position/mode.
uint8_t getSwingVerticalPosition(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.SwingV;
}

/// Control the current Quiet setting.
/// @param[in] on The desired setting.
void setQuiet(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.Quiet = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.Quiet = 0;
    }
}

/// Is the Quiet setting on?
/// @return The current value.
bool getQuiet(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Quiet;
}

/// Control the current Ion Filter setting.
/// @param[in] on The desired setting.
void setIonFilter(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.IonFilter = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.IonFilter = 0;
    }
}

/// Is the Ion Filter setting on?
/// @return The current value.
bool getIonFilter(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.IonFilter;
}

/// Control the current Light setting.
/// i.e. The LED display on the A/C unit that shows the basic settings.
/// @param[in] on The desired setting.
void setLight(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.Light = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.Light = 0;
    }
}

/// Is the Light (Display) setting on?
/// @return The current value.
bool getLight(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Light;
}

/// Control the current XFan setting.
/// This setting will cause the unit blow air after power off to dry out the
/// A/C device.
/// @note XFan mode is only valid in Cool or Dry mode.
/// @param[in] on The desired setting.
void setXFan(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.XFan = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.XFan = 0;
    }
}

/// Is the XFan setting on?
/// @return The current value.
bool getXFan(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.XFan;
}

/// Control the current Turbo setting.
/// @note Turbo mode is turned off if the fan speed is changed.
/// @param[in] on The desired setting.
void setTurbo(GreeProtocol_t *greeproc, bool state)
{
    if (state)
    {
        greeproc->GreeProtocol_bit_t.Turbo = 1;
    }
    else
    {
        greeproc->GreeProtocol_bit_t.Turbo = 0;
    }
}

/// Is the Turbo setting on?
/// @return The current value.
bool getTurbo(GreeProtocol_t *greeproc)
{
    return greeproc->GreeProtocol_bit_t.Turbo;
}
