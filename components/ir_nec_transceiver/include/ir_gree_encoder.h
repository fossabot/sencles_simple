/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief IR NEC scan code representation
     */
    typedef struct
    {
        // Byte 0
        uint8_t Mode : 3;
        uint8_t Power : 1;
        uint8_t BasicFan : 2;
        uint8_t SwingAuto : 1;
        uint8_t : 1; // Sleep Modes 1 & 3 (1 = On, 0 = Off)
        // Byte 1
        uint8_t Temp : 4; // Degrees C.
        uint8_t : 4;
        // Byte 2
        uint8_t : 4;
        uint8_t Turbo : 1;
        uint8_t Light : 1;
        uint8_t IonFilter : 1;
        uint8_t XFan : 1;
        // Byte 3
        uint8_t : 4;
        uint8_t 0b01 : 2; // (possibly timer related) (Typically 0b01)
        uint8_t 0b01 : 2; // End of command block (B01)
        // (B010 marker and a gap of 20ms)
        // Byte 4
        uint8_t SwingV : 4;
        uint8_t SwingH : 1;
        uint8_t : 3;
        // Byte 5~6
        uint8_t pad0[2]; // Timer related. Typically 0 except when timer in use.
        // Byte 7
        uint8_t : 4;      // (Used in Timer mode)
        uint8_t Sum1 : 4; // checksum of the previous bytes (0-6)
        // (gap of 40ms)
        // (header mark and space)
        // Byte 8~10
        uint8_t pad1[3]; // Repeat of byte 0~2
        // Byte 11
        uint8_t : 4;
        uint8_t : 2; // (possibly timer related) (Typically 0b11)
        uint8_t : 2; // End of command block (B01)
        // (B010 marker and a gap of 20ms)
        // Byte 12
        uint8_t : 1; // Sleep mode 2 (1 = On, 0=Off)
        uint8_t : 6; // (Used in Sleep Mode 3, Typically 0b000000)
        uint8_t Quiet : 1;
        // Byte 13
        uint8_t : 8; // (Sleep Mode 3 related, Typically 0x00)
        // Byte 14
        uint8_t : 4; // (Sleep Mode 3 related, Typically 0b0000)
        uint8_t Fan : 3;
        // Byte 15
        uint8_t : 4;
        uint8_t Sum2 : 4; // checksum of the previous bytes (8-14)
    } ir_gree_scan_code_t;

    /**
     * @brief Type of IR NEC encoder configuration
     */
    typedef struct
    {
        uint32_t resolution; /*!< Encoder resolution, in Hz */
    } ir_nec_encoder_config_t;

    /**
     * @brief Create RMT encoder for encoding IR NEC frame into RMT symbols
     *
     * @param[in] config Encoder configuration
     * @param[out] ret_encoder Returned encoder handle
     * @return
     *      - ESP_ERR_INVALID_ARG for any invalid arguments
     *      - ESP_ERR_NO_MEM out of memory when creating IR NEC encoder
     *      - ESP_OK if creating encoder successfully
     */
    esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif
