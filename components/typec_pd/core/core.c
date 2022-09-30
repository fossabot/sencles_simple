/*******************************************************************************
 * @file     core.c
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
#include "core.h"
#include "TypeC.h"
#include "PDProtocol.h"
#include "PDPolicy.h"
#include "TypeC_Types.h"
#include "PD_Types.h"
#include "version.h"
/*
 * Call this function to initialize the core.
 */
void core_initialize(Port_t *port, uint8_t i2cAddr)
{
    PortInit(port, i2cAddr);
    core_enable_typec(port, true);
    core_set_state_unattached(port);
}

/*
 * Call this function to enable or disable the core Type-C state machine.
 */
void core_enable_typec(Port_t *port, bool enable)
{
    port->SMEnabled = enable;
}

/*
 * Call this function to enable or disable the core PD state machines.
 */
void core_enable_pd(Port_t *port, bool enable)
{
    port->USBPDEnabled = enable;
}

/*
 * Call this function to run the state machines.
 */
void core_state_machine(Port_t *port)
{
    uint8_t data;
    
    /* Check on HardReset timeout (shortcut for SenderResponse timeout) */
    if ((port->WaitingOnHR == true) &&
        TimerExpired(&port->PolicyStateTimer))
    {
        data = port->Registers.Control.byte[3] | 0x40;  /* Hard Reset bit */
        DeviceWrite(port->I2cAddr, regControl3, 1, &data);
    }

    /* Update the current port being used and process the port */
    /* The Protocol and Policy functions are called from within this call */
    StateMachineTypeC(port);
}

/*
 * Check for the next required timeout to support timer interrupt functionality
 */
uint32_t core_get_next_timeout(Port_t *port)
{
  uint32_t time = 0;
  uint32_t nexttime = 0xFFFFFFFF;
  uint8_t i;

  for (i = 0; i < FSC_NUM_TIMERS; ++i)
  {
    time = TimerRemaining(port->Timers[i]);
    if (time > 0 && time < nexttime) nexttime = time;
  }

  if (nexttime == 0xFFFFFFFF) nexttime = 0;

  return nexttime;
}

uint8_t core_get_rev_lower(void)
{
    return FSC_TYPEC_CORE_FW_REV_LOWER;
}

uint8_t core_get_rev_middle(void)
{
    return FSC_TYPEC_CORE_FW_REV_MIDDLE;
}

uint8_t core_get_rev_upper(void)
{
    return FSC_TYPEC_CORE_FW_REV_UPPER;
}

uint8_t core_get_cc_orientation(Port_t *port)
{
    return port->CCPin;
}

void core_send_hard_reset(Port_t *port)
{
#ifdef CONFIG_CONFIG_FSC_DEBUG
    SendUSBPDHardReset(port);
#endif
}

void core_set_state_unattached(Port_t *port)
{
    SetStateUnattached(port);
}

void core_reset_pd(Port_t *port)
{
    port->USBPDEnabled = true;
    USBPDEnable(port, true, port->sourceOrSink);
}

uint16_t core_get_advertised_current(Port_t *port)
{
    uint16_t current = 0;

    if (port->sourceOrSink == SINK)
    {
        if (port->PolicyHasContract)
        {
            /* If there is a PD contract - return contract current. */
            /* TODO - add PPS handling, etc. */
            current = port->USBPDContract.FVRDO.OpCurrent * 10;
        }
        else
        {
            /* Otherwise, return the TypeC advertised value... or... */
            /* Note for Default: This can be
             * 500mA for USB 2.0
             * 900mA for USB 3.1
             * Up to 1.5A for USB BC 1.2
             */
            switch (port->SinkCurrent)
            {
            case utccDefault:
                current = 500;
                break;
            case utcc1p5A:
                current = 1500;
                break;
            case utcc3p0A:
                current = 3000;
                break;
            case utccNone:
            default:
                current = 0;
                break;
            }
        }
    }
    return current;
}

void core_set_advertised_current(Port_t *port, uint8_t value)
{
    UpdateCurrentAdvert(port, value);
}

void core_set_drp(Port_t *port)
{
#ifdef CONFIG_FSC_HAVE_DRP
    port->PortConfig.PortType = USBTypeC_DRP;
    port->PortConfig.SnkPreferred = false;
    port->PortConfig.SrcPreferred = false;
    SetStateUnattached(port);
#endif /* CONFIG_FSC_HAVE_DRP */
}

void core_set_try_snk(Port_t *port)
{
#ifdef CONFIG_FSC_HAVE_DRP
    port->PortConfig.PortType = USBTypeC_DRP;
    port->PortConfig.SnkPreferred = true;
    port->PortConfig.SrcPreferred = false;
    SetStateUnattached(port);
#endif /* CONFIG_FSC_HAVE_DRP */
}

void core_set_try_src(Port_t *port)
{
#ifdef CONFIG_FSC_HAVE_DRP
    port->PortConfig.PortType = USBTypeC_DRP;
    port->PortConfig.SnkPreferred = false;
    port->PortConfig.SrcPreferred = true;
    SetStateUnattached(port);
#endif /* CONFIG_FSC_HAVE_DRP */
}

void core_set_source(Port_t *port)
{
#ifdef CONFIG_FSC_HAVE_SRC
    port->PortConfig.PortType = USBTypeC_Source;
    SetStateUnattached(port);
#endif /* CONFIG_FSC_HAVE_SRC */
}

void core_set_sink(Port_t *port)
{
#ifdef CONFIG_FSC_HAVE_SNK
    port->PortConfig.PortType = USBTypeC_Sink;
    SetStateUnattached(port);
#endif /* CONFIG_FSC_HAVE_SNK */
}

