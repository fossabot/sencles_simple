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
     * @brief IR scan code representation
     */
    typedef union GreeProtocol
    {
        uint8_t raw[16]; ///< The state in IR code form.
        struct GreeProtocol_bit
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
            uint8_t : 2; // (possibly timer related) (Typically 0b01)
            uint8_t : 2; // End of command block (B01)
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
        } GreeProtocol_bit_t;
    } GreeProtocol_t;

    enum opmode_t
    {
        opmode_Off = -1,
        opmode_Auto = 0,
        opmode_Cool = 1,
        opmode_Heat = 2,
        opmode_Dry = 3,
        opmode_Fan = 4,
    };

    /// Common A/C settings for Fan Speeds.
    enum fanspeed_t
    {
        fanspeed_Auto = 0,
        fanspeed_Min = 1,
        fanspeed_Low = 2,
        fanspeed_Medium = 3,
        fanspeed_High = 4,
        fanspeed_Max = 5,
    };

    /// Common A/C settings for Vertical Swing.
    enum swingv_t
    {
        swingv_Off = -1,
        swingv_Auto = 0,
        swingv_Highest = 1,
        swingv_High = 2,
        swingv_Middle = 3,
        swingv_Low = 4,
        swingv_Lowest = 5,
    };


    /// Structure to hold a common A/C state.
    struct state_t
    {
        bool power = false;
        opmode_t mode = opmode_t::kOff;
        float degrees = 25;
        bool celsius = true;
        fanspeed_t fanspeed = fanspeed_t::kAuto;
        swingv_t swingv = swingv_t::kOff;
        swingh_t swingh = swingh_t::kOff;
        bool quiet = false;
        bool turbo = false;
        bool econo = false;
        bool light = false;
        bool filter = false;
        bool clean = false;
        bool beep = false;
        int16_t sleep = -1; // `-1` means off.
        int16_t clock = -1; // `-1` means not set.
    };

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

    GreeProtocol_t _;

    /**
     * @brief Type of IR NEC encoder configuration
     */
    typedef struct
    {
        uint32_t resolution; /*!< Encoder resolution, in Hz */
    } ir_gree_encoder_config_t;

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
    esp_err_t rmt_new_ir_gree_encoder(const ir_gree_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);
    void ir_gree_transceiver_main_task(void *arg);

#ifdef __cplusplus
}
#endif
