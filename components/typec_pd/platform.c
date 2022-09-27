#include "platform.h"

void platform_set_vbus_lvl_enable(FSC_U8 port,
                                  VBUS_LVL level,
                                  FSC_BOOL enable,
                                  FSC_BOOL disableOthers)
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
    if (disableOthers || ((level == VBUS_LVL_ALL) && (enable == FALSE)))
    {
        i = 0;

        do {
            /* Skip the current level */
            if( i == level ) continue;

            /* Turn off the other level(s) */
            platform_set_vbus_lvl_enable( i, FALSE, FALSE );
        } while (++i < VBUS_LVL_ALL);
    }
}

FSC_BOOL platform_get_vbus_lvl_enable(FSC_U8 port, VBUS_LVL level)
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

    return FALSE;
}

void platform_set_vbus_discharge(FSC_U8 port, FSC_BOOL enable)
{
    /* Enable/Disable the discharge path */
    /* TODO - Implement as needed on platforms that support this feature. */
}

FSC_BOOL platform_get_device_irq_state(FSC_U8 port)
{
    /* Return the state of the device interrupt signal. */
    return FALSE;
}

FSC_BOOL platform_i2c_write(FSC_U8 SlaveAddress,
                            FSC_U8 RegAddrLength,
                            FSC_U8 DataLength,
                            FSC_U8 PacketSize,
                            FSC_U8 IncSize,
                            FSC_U32 RegisterAddress,
                            FSC_U8* Data)
{
    /* Write some data! */

    /*
     * Return TRUE if successful,
     * Return FALSE otherwise.
     */

    return FALSE;
}

FSC_BOOL platform_i2c_read( FSC_U8 SlaveAddress,
                            FSC_U8 RegAddrLength,
                            FSC_U8 DataLength,
                            FSC_U8 PacketSize,
                            FSC_U8 IncSize,
                            FSC_U32 RegisterAddress,
                            FSC_U8* Data)
{
    /* Read some data! */

    /*
     * Return TRUE if successful,
     * Return FALSE otherwise.
     */

    return FALSE;
}

/*****************************************************************************
* Function:        platform_delay_10us
* Input:           delayCount - Number of 10us delays to wait
* Return:          None
* Description:     Perform a software delay in intervals of 10us.
******************************************************************************/
void platform_delay_10us(FSC_U32 delayCount)
{
    /*fusb_Delay10us(delayCount); */
}

void platform_set_pps_voltage(FSC_U32 mv)
{
}

FSC_U16 platform_get_pps_voltage(void)
{
    return 0;
}

void platform_set_pps_current(FSC_U32 ma)
{
}

FSC_U16 platform_get_pps_current(void)
{
    return 0;
}

FSC_U32 platform_get_system_time(void)
{
    return 0;
}

