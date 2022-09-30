/*******************************************************************************
 * @file     vdm.h
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
#ifndef __VDM_MANAGER_H__
#define __VDM_MANAGER_H__

#include "vendor_info.h"
#include "platform.h"
#include "fsc_vdm_defs.h"
#include "vdm_callbacks_defs.h"
#include "PD_Types.h"
#include "Port.h"

#ifdef CONFIG_FSC_HAVE_VDM

#define SVID_DEFAULT SVID1_SOP
#define MODE_DEFAULT 0x0001
#define SVID_AUTO_ENTRY 0x1057
#define MODE_AUTO_ENTRY 0x0001

#define NUM_VDM_MODES 6
#define MAX_NUM_SVIDS_PER_SOP 30
#define MAX_SVIDS_PER_MESSAGE 12
#define MIN_DISC_ID_RESP_SIZE 3

/* Millisecond values ticked by 1ms timer. */
#define tVDMSenderResponse 27 
#define tVDMWaitModeEntry  45 
#define tVDMWaitModeExit   45 

/*
 * Initialization functions.
 */
int32_t initializeVdm(Port_t *port);

/*
 * Functions to go through PD VDM flow.
 */
/* Initiations from DPM */
int32_t requestDiscoverIdentity(Port_t *port, SopType sop);
int32_t requestDiscoverSvids(Port_t *port, SopType sop);
int32_t requestDiscoverModes(Port_t *port, SopType sop, uint16_t svid);
int32_t requestSendAttention(Port_t *port, SopType sop, uint16_t svid,
                             uint8_t mode);
int32_t requestEnterMode(Port_t *port, SopType sop, uint16_t svid,
                         uint32_t mode_index);
int32_t requestExitMode(Port_t *port, SopType sop, uint16_t svid,
                        uint32_t mode_index);
int32_t requestExitAllModes(void);

/* receiving end */
int32_t processVdmMessage(Port_t *port, SopType sop, uint32_t* arr_in,
                          uint32_t length_in);
int32_t processDiscoverIdentity(Port_t *port, SopType sop, uint32_t* arr_in,
                                uint32_t length_in);
int32_t processDiscoverSvids(Port_t *port, SopType sop, uint32_t* arr_in,
                             uint32_t length_in);
int32_t processDiscoverModes(Port_t *port, SopType sop, uint32_t* arr_in,
                             uint32_t length_in);
int32_t processEnterMode(Port_t *port, SopType sop, uint32_t* arr_in,
                         uint32_t length_in);
int32_t processExitMode(Port_t *port, SopType sop, uint32_t* arr_in,
                        uint32_t length_in);
int32_t processAttention(Port_t *port, SopType sop, uint32_t* arr_in,
                         uint32_t length_in);
int32_t processSvidSpecific(Port_t *port, SopType sop, uint32_t* arr_in,
                            uint32_t length_in);

/* Private function */
bool evalResponseToSopVdm(Port_t *port, doDataObject_t vdm_hdr);
bool evalResponseToCblVdm(Port_t *port, doDataObject_t vdm_hdr);
void sendVdmMessageWithTimeout(Port_t *port, SopType sop, uint32_t* arr,
                               uint32_t length, int32_t n_pe);
void vdmMessageTimeout(Port_t *port);
void startVdmTimer(Port_t *port, int32_t n_pe);
void sendVdmMessageFailed(Port_t *port);
void resetPolicyState(Port_t *port);

void sendVdmMessage(Port_t *port, SopType sop, uint32_t * arr, uint32_t length,
                    PolicyState_t next_ps);

bool evaluateModeEntry (Port_t *port, uint32_t mode_in);

#endif /* CONFIG_FSC_HAVE_VDM */
#endif /* __VDM_MANAGER_H__ */
