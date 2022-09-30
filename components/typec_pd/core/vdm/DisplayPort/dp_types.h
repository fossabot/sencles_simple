/*******************************************************************************
 * @file     dp_types.h
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
#ifdef CONFIG_FSC_HAVE_DP

#ifndef __DISPLAYPORT_TYPES_H__
#define __DISPLAYPORT_TYPES_H__

#include "platform.h"

/* DisplayPort-specific commands */
typedef enum {
  DP_COMMAND_ATTENTION = 0x06,
  DP_COMMAND_STATUS = 0x10,                            /* Status Update */
  DP_COMMAND_CONFIG = 0x11                             /* Configure */
} DisplayPortCommand_t;

typedef enum
{
  DP_Plug = 0,                                         /* DP present in plug */
  DP_Receptacle = 1,                                   /* DP present is receptacle */
} DisplayPortReceptacle_t;

typedef union {
  uint32_t word;
  uint8_t byte[4];
  struct {
    uint32_t UfpDCapable :1;                           /* 0 - reserved, 01b - UFP_D, 10b - DFP, 11b - Both */
    uint32_t DfpDCapable :1;
    uint32_t Transport :4;
    uint32_t ReceptacleIndication:1;                   /* 1 = Receptacle, 0 = Plug */
    uint32_t USB2p0NotUsed:1;                          /* 1 = USB r2.0 signaling not required, 0 = _may_ be required */
    uint32_t DFP_DPinAssignments:8;
    uint32_t UFP_DPinAssignments:8;
    uint32_t Rsvd0:8;
  };
} DisplayPortCaps_t;

/* DisplayPort DFP_D/UFP_D Connected */
typedef enum {
  DP_MODE_DISABLED = 0,                              /* Neither DFP_D nor UFP_D is connected, or adaptor is disabled */
  DP_MODE_DFP_D = 1,                                 /* DFP_D is connected */
  DP_MODE_UFP_D = 2,                                 /* UFP_D is connected */
  DP_MODE_BOTH = DP_MODE_DFP_D | DP_MODE_UFP_D       /* Both DFP_D and UFP_D are connected */
} DisplayPortConn_t;

typedef union {
  uint32_t word;
  uint8_t byte[4];
  struct {
    DisplayPortConn_t Connection:2;                            /* Connected-to information */
    uint32_t           PowerLow:1;                              /* Adaptor has detected low power and disabled DisplayPort support */
    uint32_t           Enabled:1;                               /* Adaptor DisplayPort functionality is enabled and operational */
    uint32_t           MultiFunctionPreferred:1;                /* Multi-function preference */
    uint32_t           UsbConfigRequest:1;                      /* 0 = Maintain current configuration, 1 = request switch to USB Configuration (if in DP Config) */
    uint32_t           ExitDpModeRequest:1;                     /* 0 = Maintain current mode, 1 = Request exit from DisplayPort Mode (if in DP Mode) */
    uint32_t           HpdState:1;                              /* 0 = HPD_Low, 1 = HPD_High */
    uint32_t           IrqHpd:1;                                /* 0 = No IRQ_HPD since last status message, 1 = IRQ_HPD */
    uint32_t           Rsvd0:23;
  };
} DisplayPortStatus_t;

/* Select configuration */
typedef enum {
  DP_CONF_USB = 0,                                  /* Set configuration for USB */
  DP_CONF_DFP_D = 1,                                /* Set configuration for UFP_U as DFP_D */
  DP_CONF_UFP_D = 2,                                /* Set configuration for UFP_U as UFP_D */
  DP_CONF_RSVD = 3
} DisplayPortMode_t;

/* Signaling for Transport of DisplayPort Protocol */
typedef enum {
  DP_CONF_SIG_UNSPECIFIED = 0,                     /* Signaling unspecified (only for USB Configuration) */
  DP_CONF_SIG_DP_V1P3 = 1,                         /* Select DP v1.3 signaling rates and electrical settings */
  DP_CONF_SIG_GEN2 = 2                             /* Select Gen 2 signaling rates and electrical specifications */
} DpConfSignaling_t;

typedef enum {
  DP_PIN_ASSIGN_NS = 0, /* Not selected */
  DP_PIN_ASSIGN_C = 4,
  DP_PIN_ASSIGN_D = 8,
  DP_PIN_ASSIGN_E = 16,
} DisplayPortPinAssign_t;

typedef union {
  uint32_t word;
  uint8_t byte[4];
  struct {
    DisplayPortMode_t       Mode:2;                                 /* UFP_D, DFP_D */
    DpConfSignaling_t       SignalConfig:4;                         /* Signaling configuration */
    uint32_t                 Rsvd0:2;
    DisplayPortPinAssign_t  PinAssign:8;                            /* Configure UFP_U with DFP_D Pin Assigment */
    uint32_t                 Rsvd1:8;
  };
} DisplayPortConfig_t;

typedef struct
{
  bool DpAutoModeEntryEnabled;                    /* Automatically enter mode if DP_SID found */
  bool DpEnabled;                                 /* DP is enabled */
  bool DpConfigured;                              /* Currently acting as DP UFP_D or DFP_D */
  bool DpCapMatched;                              /* Port's capability matches partner's capability */
  uint32_t  DpModeEntered;                              /* Mode index of display port mode entered */

  /*  DisplayPort Status/Config objects */
  DisplayPortCaps_t DpCap;                            /* Port DP capability */
  DisplayPortStatus_t DpStatus;                       /* Port DP status sent during Discv. Mode or attention */

  /*  Port Partner Status/Config objects */
  DisplayPortCaps_t   DpPpCap;                        /* Port partner capability rx from Modes */
  DisplayPortConfig_t DpPpConfig;                     /* Port partner configuration to select, valid when DpCapMatched == TRUE */
  DisplayPortStatus_t DpPpStatus;                     /* Previous status of port partner */

} DisplayPortData_t;

#endif  /* __DISPLAYPORT_TYPES_H__ */

#endif /* CONFIG_FSC_HAVE_DP */
