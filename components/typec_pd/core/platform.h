/*******************************************************************************
 * @file     platform.h
 * @author   USB PD Firmware Team
 *
 * Copyright 2018 ON Semiconductor. All rights reserved.
 *
 * This software and/or documentation is licensed by ON Semiconductor under
 * limited terms and conditions. The terms and conditions pertaining to the
 * software and/or documentation are available at
 * http://www.onsemi.com/site/pdf/ONSEMI_T&C.pdf
 * ("ON Semiconductor Standard Terms and Conditions of Sale, Section 8 Software").
 *
 * DO NOT USE THIS SOFTWARE AND/OR DOCUMENTATION UNLESS YOU HAVE CAREFULLY
 * READ AND YOU AGREE TO THE LIMITED TERMS AND CONDITIONS. BY USING THIS
 * SOFTWARE AND/OR DOCUMENTATION, YOU AGREE TO THE LIMITED TERMS AND CONDITIONS.
 ******************************************************************************/
#ifndef _FSC_PLATFORM_H_
#define _FSC_PLATFORM_H_

#include "stdio.h"
#include "stdbool.h"
#include "sdkconfig.h"


#ifndef NUM_PORTS
#define NUM_PORTS           1   /* Number of ports in this system */
#endif /* NUM_PORTS */

/**
 * VBus switch levels
 */
typedef enum
{
    VBUS_LVL_5V,
    VBUS_LVL_HV,
    VBUS_LVL_ALL
} VBUS_LVL;


/**
 * Current port connection state
 */
typedef enum
{
    SINK = 0,
    SOURCE
} SourceOrSink;

/**
 * Events that platform uses to notify modules listening to the events.
 * The subscriber to event signal can subscribe to individual events or
 * a event in group.
 */
typedef enum
{
    CC1_ORIENT         =       0x00000001,
    CC2_ORIENT         =       0x00000002,
    CC_NO_ORIENT       =       0x00000004,
    CC_ORIENT_ALL      =       CC1_ORIENT | CC2_ORIENT | CC_NO_ORIENT,
    PD_NEW_CONTRACT    =       0x00000008,
    PD_NO_CONTRACT     =       0x00000010,
    PD_CONTRACT_ALL    =       PD_NEW_CONTRACT | PD_NO_CONTRACT,
    PD_STATE_CHANGED   =       0x00000020,
    ACC_UNSUPPORTED    =       0x00000040,
    DATA_ROLE          =       0x00000080,
    BIST_DISABLED      =       0x00000100,
    BIST_ENABLED       =       0x00000200,
    BIST_ALL           =       BIST_ENABLED | BIST_DISABLED,
    ALERT_EVENT        =       0x00000400,
    EVENT_ALL          =       0xFFFFFFFF,
} Events_t;

/**
 * @brief Set or return the current vbus voltage level as implemented by
 *        the platform (i.e. supply control, gpio switches, etc.)
 *
 * @param port ID for multiple port controls
 * @param level enumeration
 * @param enable TRUE = ON
 * @param disableOthers Disable other sources in make-before-break fashion
 * @return None or state of vbus.
 */
void platform_set_vbus_lvl_enable(uint8_t port, VBUS_LVL level, bool enable,
                                  bool disableOthers);

/**
 * @brief Check if the VBUS voltage is enabled
 * @param level VBUS level to check
 * @return TRUE if enabled
 */
bool platform_get_vbus_lvl_enable(uint8_t port, VBUS_LVL level);

/**
 * @brief Set or return programmable supply (PPS) voltage and current limit.
 *
 * @param port ID for multiple port controls
 * @param mv Voltage in millivolts
 * @return None or Value in mv/ma.
 */
void platform_set_pps_voltage(uint8_t port, uint32_t mv);

/**
 * @brief The function gets the current VBUS level supplied by PPS supply
 *
 * If VBUS is not enabled by the PPS supply the return type is undefined.
 *
 * @param port ID for multiple port controls
 * @return VBUS level supplied by PPS in milivolt resolution
 */
uint16_t platform_get_pps_voltage(uint8_t port);

/**
 * @brief Set the maximum current that can be supplied by PPS source
 * @param port ID for multiple port controls
 * @param ma Current in milliamps
 * @return None
 */
void platform_set_pps_current(uint8_t port, uint32_t ma);

/**
 * @brief Get the maximum current that the PPS supply is configured to provide
 *
 * If the PPS supply is not currently supplying current the return value is
 * undefined.
 *
 * @param port ID for multiple port controls
 * @return Current in milliamps
 */
uint16_t platform_get_pps_current(uint8_t port);

/**
 * @brief Enable/Disable VBus discharge path
 *
 * @param port ID for multiple port controls
 * @param enable TRUE = discharge path ON.
 * @return None
 */
void platform_set_vbus_discharge(uint8_t port, bool enable);

/**
 * @brief Enable/Disable VConn path
 *
 * Optional for platforms with separate VConn switch
 *
 * @param port ID for multiple port controls
 * @param enable TRUE = VConn path ON.
 * @return None
 */
void platform_set_vconn(uint8_t port, bool enable);

/**
 * @brief The current state of the device interrupt pin
 *
 * @param port ID for multiple port controls
 * @return TRUE if interrupt condition present.  Note: pin is active low.
 */
bool platform_get_device_irq_state(uint8_t port);

/**
 * @brief Write a char buffer to the I2C peripheral.
 *
 * Assumes a single I2C bus.  If multiple buses are used, map based on
 * I2C address in the platform code.
 *
 * @param SlaveAddress - Slave device bus address
 * @param RegAddrLength - Register Address Byte Length
 * @param DataLength - Length of data to transmit
 * @param PacketSize - Maximum size of each transmitted packet
 * @param IncSize - Number of bytes to send before incrementing addr
 * @param RegisterAddress - Internal register address
 * @param Data - Buffer of char data to transmit
 * @return TRUE - success, FALSE otherwise
 */
bool platform_i2c_write(uint8_t SlaveAddress,
                            uint8_t RegAddrLength,
                            uint8_t DataLength,
                            uint8_t PacketSize,
                            uint8_t IncSize,
                            uint32_t RegisterAddress,
                            uint8_t* Data);

/**
 * @brief Read char data from the I2C peripheral.
 *
 * Assumes a single I2C bus.  If multiple buses are used, map based on
 * I2C address in the platform code.
 *
 * @param SlaveAddress - Slave device bus address
 * @param RegAddrLength - Register Address Byte Length
 * @param DataLength - Length of data to attempt to read
 * @param PacketSize - Maximum size of each received packet
 * @param IncSize - Number of bytes to recv before incrementing addr
 * @param RegisterAddress - Internal register address
 * @param Data - Buffer for received char data
 * @return TRUE - success, FALSE otherwise
 */
bool platform_i2c_read( uint8_t SlaveAddress,
                            uint8_t RegAddrLength,
                            uint8_t DataLength,
                            uint8_t PacketSize,
                            uint8_t IncSize,
                            uint32_t RegisterAddress,
                            uint8_t* Data);

/**
 * @brief Perform a blocking delay.
 *
 * @param delayCount - Number of 10us delays to wait
 * @return None
 */
void platform_delay_10us(uint32_t delayCount);

/**
 * @brief Perform a blocking delay.
 *
 * @param delayCount - Number of us delays to wait
 * @return None
 */
void platform_delay(uint32_t uSec);
/**
 * @brief Return a system timestamp for use with core timers.
 *
 * @param None
 * @return System time value in units of (milliseconds / )
 */
uint32_t platform_get_system_time(void);

/**
 * @brief Return a system timestamp for use with logging functions
 *
 * @param None
 * @return Packed timestamp - format: Upper 16: seconds, Lower 16: 0.1ms.
 */
uint32_t platform_get_log_time(void);


#ifdef CONFIG_FSC_HAVE_DP
/******************************************************************************
 * Function:        platform_dp_enable_pins
 * Input:           enable - If false put dp pins to safe state and config is
 *                           don't care. When true configure the pins with valid
 *                           config.
 *                  config - 32-bit port partner config. Same as type in
 *                  DisplayPortConfig_t in display_port_types.h.
 * Return:          TRUE - pin config succeeded, FALSE - pin config failed
 * Description:     enable/disable display port pins. If enable is true, check
 *                  the configuration bits[1:0] and the pin assignment
 *                  bits[15:8] to decide the appropriate configuration.
 ******************************************************************************/
bool platform_dp_enable_pins(bool enable, uint32_t config);

/******************************************************************************
 * Function:        platform_dp_status_update
 * Input:           status - 32-bit status value. Same as DisplayPortStatus_t
 *                  in display_port_types.h
 * Return:          None
 * Description:     Called when new status is available from port partner
 ******************************************************************************/
void platform_dp_status_update(uint32_t status);
#endif

#endif /* _FSC_PLATFORM_H_ */

