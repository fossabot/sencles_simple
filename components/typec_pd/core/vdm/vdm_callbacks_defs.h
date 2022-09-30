/*******************************************************************************
 * @file     vdm_callbacks_defs.h
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
#ifndef __FSC_VDM_CALLBACKS_DEFS_H__
#define __FSC_VDM_CALLBACKS_DEFS_H__
/*
 * This file defines types for callbacks that the VDM block will use.
 * The intention is for the user to define functions that return data
 * that VDM messages require, ex whether or not to enter a mode.
 */
#ifdef CONFIG_FSC_HAVE_VDM

#include "vdm_types.h"
#include "PD_Types.h"

typedef struct Port Port_t;

typedef Identity (*RequestIdentityInfo)(Port_t *port);

typedef SvidInfo (*RequestSvidInfo)(Port_t *port);

typedef ModesInfo (*RequestModesInfo)(Port_t *port, uint16_t);

typedef bool (*ModeEntryRequest)(Port_t *port, uint16_t svid, uint32_t mode_index);

typedef bool (*ModeExitRequest)(Port_t *port, uint16_t svid, uint32_t mode_index);

typedef bool (*EnterModeResult)(Port_t *port, bool success, uint16_t svid, uint32_t mode_index);

typedef void (*ExitModeResult)(Port_t *port, bool success, uint16_t svid, uint32_t mode_index);

typedef void (*InformIdentity)(Port_t *port, bool success, SopType sop, Identity id);

typedef void (*InformSvids)(Port_t *port, bool success, SopType sop, SvidInfo svid_info);

typedef void (*InformModes)(Port_t *port, bool success, SopType sop, ModesInfo modes_info);

typedef void (*InformAttention)(Port_t *port, uint16_t svid, uint8_t mode_index);

/*
 * VDM Manager object, so I can have multiple instances intercommunicating using the same functions!
 */
typedef struct
{
    /* callbacks! */
    RequestIdentityInfo req_id_info;
    RequestSvidInfo req_svid_info;
    RequestModesInfo req_modes_info;
    ModeEntryRequest req_mode_entry;
    ModeExitRequest req_mode_exit;
    EnterModeResult enter_mode_result;
    ExitModeResult exit_mode_result;
    InformIdentity inform_id;
    InformSvids inform_svids;
    InformModes inform_modes;
    InformAttention inform_attention;
} VdmManager;

#endif /* CONFIG_FSC_HAVE_VDM */

#endif /* __FSC_VDM_CALLBACKS_DEFS_H__ */

