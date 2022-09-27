/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
        uint32_t data_pack[4];
        uint8_t raw[16]; ///< The state in IR code form.
        struct GreeProtocol_bit
        {
            // Byte 0   610,580,610,580,610,580,610,580,610,580,610,580,610,1680,610,580,
            uint8_t Mode : 3;
            uint8_t Power : 1;
            uint8_t BasicFan : 2;
            uint8_t SwingAuto : 1;
            uint8_t : 1; // Sleep Modes 1 & 3 (1 = On, 0 = Off)
            // Byte 1    610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,
            uint8_t Temp : 4; // Degrees C.
            uint8_t : 4;
            // Byte 2    610,580,610,580,610,580,610,580,610,580,610,1680,610,580,610,580,
            uint8_t : 4;
            uint8_t Turbo : 1;
            uint8_t Light : 1;
            uint8_t IonFilter : 1;
            uint8_t XFan : 1;
            // Byte 3    610,580,610,580,610,580,610,580,610,1680,610,580,610,1680,610,580,
            uint8_t : 4;
            uint8_t : 2; // (possibly timer related) (Typically 0b01)
            uint8_t : 2; // End of command block (B01)
            // (B010 marker and a gap of 20ms)  610,580,610,1680,610,580,610,20000,
            // Byte 4    610,1680,610,580,610,580,610,580,610,1680,610,580,610,580,610,580,
            uint8_t SwingV : 4;
            uint8_t SwingH : 1;
            uint8_t : 3;
            // Byte 5~6  610,580,610,580,610,580,610,580,610,580,610,1680,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,
            uint8_t pad0[2]; // Timer related. Typically 0 except when timer in use.
            // Byte 7    610,580,610,580,610,580,610,580,610,1680,610,580,610,1680,610,1680,
            uint8_t : 4;      // (Used in Timer mode)
            uint8_t Sum1 : 4; // checksum of the previous bytes (0-6)
            // (gap of 40ms)   610,40000,
            // (header mark and space)

            // 9000,4500,

            // Byte 8~10  610,580,610,580,610,580,610,580,610,580,610,580,610,1680,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,1680,610,580,610,580,
            uint8_t pad1[3]; // Repeat of byte 0~2
            // Byte 11    610,580,610,580,610,580,610,580,610,1680,610,1680,610,1680,610,580,
            uint8_t : 4;
            uint8_t : 2; // (possibly timer related) (Typically 0b11)
            uint8_t : 2; // End of command block (B01)
            // (B010 marker and a gap of 20ms)  610,580,610,1680,610,580,610,20000,
            // Byte 12    610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,
            uint8_t : 1; // Sleep mode 2 (1 = On, 0=Off)
            uint8_t : 6; // (Used in Sleep Mode 3, Typically 0b000000)
            uint8_t Quiet : 1;
            // Byte 13    610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,
            uint8_t : 8; // (Sleep Mode 3 related, Typically 0x00)
            // Byte 14    610,580,610,580,610,580,610,580,610,580,610,580,610,580,610,580,
            uint8_t : 4; // (Sleep Mode 3 related, Typically 0b0000)
            uint8_t Fan : 3;
            // Byte 15    610,580,610,580,610,580,610,580,610,1680,610,1680,610,580,610,580,    610]
            uint8_t : 4;
            uint8_t Sum2 : 4; // checksum of the previous bytes (8-14)
        } GreeProtocol_bit_t;
    } GreeProtocol_t;

    typedef enum
    {
        opmode_Auto = 0,
        opmode_Cool = 1,
        opmode_Dry = 2,
        opmode_Fan = 3,
        opmode_Heat = 4,
    } opmode_t;

    /// Common A/C settings for Fan Speeds.
    typedef enum
    {
        fanspeed_Auto = 0,
        fanspeed_Min = 1,
        fanspeed_Low = 2,
        fanspeed_Medium = 3,
        fanspeed_High = 4,
        fanspeed_Max = 5,
    } fanspeed_t;

    /// Common A/C settings for Vertical Swing.
    typedef enum
    {
        swingv_Off = 0,
        swingv_Auto = 1,
        swingv_Highest = 2,
        swingv_Upper_Middle = 3,
        swingv_Middle = 4,
        swingv_Lower_Middle = 5,
        swingv_Lowest = 6,
        swingv_Low_Auto = 7,
        swingv_Middle_Auto = 9,
        swingv_High_Auto = 11,
    } swingv_t;

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

    esp_err_t stateReset(GreeProtocol_t *greeproc);
    esp_err_t setPower(GreeProtocol_t *greeproc, bool state);
    bool getPower(GreeProtocol_t *greeproc);
    esp_err_t setTemp(GreeProtocol_t *greeproc, uint8_t degrees);
    uint8_t getTemp(GreeProtocol_t *greeproc);
    esp_err_t setFan(GreeProtocol_t *greeproc, fanspeed_t speed);
    fanspeed_t getFan(GreeProtocol_t *greeproc);
    esp_err_t setMode(GreeProtocol_t *greeproc, opmode_t mode);
    uint8_t getMode(GreeProtocol_t *greeproc);
    esp_err_t setSwingVertical(GreeProtocol_t *greeproc, bool automatic, swingv_t position);
    bool getSwingVerticalAuto(GreeProtocol_t *greeproc);
    uint8_t getSwingVerticalPosition(GreeProtocol_t *greeproc);
    esp_err_t setSwingHorizontal(GreeProtocol_t *greeproc, bool state);
    bool getSwingHorizontal(GreeProtocol_t *greeproc);
    esp_err_t setQuiet(GreeProtocol_t *greeproc, bool state);
    bool getQuiet(GreeProtocol_t *greeproc);
    esp_err_t setIonFilter(GreeProtocol_t *greeproc, bool state);
    bool getIonFilter(GreeProtocol_t *greeproc);
    esp_err_t setLight(GreeProtocol_t *greeproc, bool state);
    bool getLight(GreeProtocol_t *greeproc);
    esp_err_t setXFan(GreeProtocol_t *greeproc, bool state);
    bool getXFan(GreeProtocol_t *greeproc);
    esp_err_t setTurbo(GreeProtocol_t *greeproc, bool state);
    bool getTurbo(GreeProtocol_t *greeproc);
    esp_err_t procotol_fixup(GreeProtocol_t *greeproc);
esp_err_t getRaw(GreeProtocol_t *greeproc, uint8_t *raw);
    esp_err_t setRaw(GreeProtocol_t *greeproc, uint8_t new_code[]);


#ifdef __cplusplus
}
#endif
