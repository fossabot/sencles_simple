#include "platform.h"

void platform_set_vbus_lvl_enable(uint8_t port,
                                  VBUS_LVL level,
                                  bool enable,
                                  bool disableOthers)
{
    unsigned int i;

    /* Additional VBUS levels can be added here as needed. */
    switch (level) {
    case VBUS_LVL_5V:
        /* Enable/Disable the 5V Source */

        break;
    default:
        /* Otherwise, do nothing. */
        break;
    }

    /* Turn off other levels, if requested */
    if (disableOthers || ((level == VBUS_LVL_ALL) && (enable == false)))
    {
        i = 0;

        do {
            /* Skip the current level */
            if( i == level ) continue;

            /* Turn off the other level(s) */
            platform_set_vbus_lvl_enable( i, level, false, false);
        } while (++i < VBUS_LVL_ALL);
    }
}

bool platform_get_vbus_lvl_enable(uint8_t port, VBUS_LVL level)
{
    /* Additional VBUS levels can be added here as needed. */
    switch (level) {
    case VBUS_LVL_5V:
        /* Return the state of the 5V VBUS Source. */

        break;
    default:
        /* Otherwise, return FALSE. */
        break;
    }

    return false;
}

void platform_set_vbus_discharge(uint8_t port, bool enable)
{
    /* Enable/Disable the discharge path */
    /* TODO - Implement as needed on platforms that support this feature. */
}

bool platform_get_device_irq_state(uint8_t port)
{
    /* Return the state of the device interrupt signal. */
    return false;
}

bool platform_i2c_write(uint8_t SlaveAddress,
                            uint8_t RegAddrLength,
                            uint8_t DataLength,
                            uint8_t PacketSize,
                            uint8_t IncSize,
                            uint32_t RegisterAddress,
                            uint8_t* Data)
{
    /* Write some data! */

    /*
     * Return TRUE if successful,
     * Return FALSE otherwise.
     */

    return false;
}

bool platform_i2c_read( uint8_t SlaveAddress,
                            uint8_t RegAddrLength,
                            uint8_t DataLength,
                            uint8_t PacketSize,
                            uint8_t IncSize,
                            uint32_t RegisterAddress,
                            uint8_t* Data)
{
    /* Read some data! */

    /*
     * Return TRUE if successful,
     * Return FALSE otherwise.
     */

    return false;
}

/*****************************************************************************
* Function:        platform_delay_10us
* Input:           delayCount - Number of 10us delays to wait
* Return:          None
* Description:     Perform a software delay in intervals of 10us.
******************************************************************************/
void platform_delay_10us(uint32_t delayCount)
{
    /*fusb_Delay10us(delayCount); */
}

void platform_set_pps_voltage(uint8_t port, uint32_t mv)
{
}

uint16_t platform_get_pps_voltage(uint8_t port)
{
    return 0;
}

void platform_set_pps_current(uint8_t port, uint32_t ma)
{
}

uint16_t platform_get_pps_current(uint8_t port)
{
    return 0;
}

uint32_t platform_get_system_time(void)
{
    return 0;
}

