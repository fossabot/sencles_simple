/*******************************************************************************
 * @file     vdm_callbacks.h
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
/*
 * Using this file to emulate the callbacks that a real DPM would set up for the VDM block.
 * Setting default values etc in here too.
 */


#ifndef __DPM_EMULATION_H__
#define __DPM_EMULATION_H__

#include "platform.h"
#include "vdm_types.h"
#include "PD_Types.h"

#ifdef CONFIG_FSC_HAVE_VDM

ModesInfo vdmRequestModesInfo(Port_t *port, uint16_t svid);

Identity vdmRequestIdentityInfo(Port_t *port);
SvidInfo vdmRequestSvidInfo(Port_t *port);
bool vdmModeEntryRequest(Port_t *port, uint16_t svid, uint32_t mode_index);
bool vdmModeExitRequest(Port_t *port, uint16_t svid, uint32_t mode_index);
bool vdmEnterModeResult(Port_t *port, bool success, uint16_t svid, uint32_t mode_index);

void vdmExitModeResult(Port_t *port, bool success, uint16_t svid, uint32_t mode_index);
void vdmInformIdentity(Port_t *port, bool success, SopType sop, Identity id);
void vdmInformSvids(Port_t *port, bool success, SopType sop, SvidInfo svid_info);
void vdmInformModes(Port_t *port, bool success, SopType sop, ModesInfo modes_info);
void vdmInformAttention(Port_t *port, uint16_t svid, uint8_t mode_index);
void vdmInitDpm(Port_t *port);

#endif /* CONFIG_FSC_HAVE_VDM */

#endif /* __DPM_EMULATION_H__ */

