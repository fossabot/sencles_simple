/*******************************************************************************
 * @file     TypeC.h
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
#ifndef _FSC_TYPEC_H_
#define	_FSC_TYPEC_H_

#include "platform.h"
#include "Port.h"

/* Type C Timing Parameters */
#define tAMETimeout     900   /* Alternate Mode Entry Time */
#define tCCDebounce     120  
#define tPDDebounce     15   
#define tDRPTry         90   
#define tDRPTryWait     600 
#define tTryTimeout     825 
#define tErrorRecovery  30    /* Delay in Error Recov State */

#define tDeviceToggle   3     /* CC pin measure toggle */
#define tTOG2           30    /* DRP Toggle timing */
#define tIllegalCable   500   /* Reconnect loop reset time */
#define tOrientedDebug  100   /* DebugAcc Orient Delay */
#define tLoopReset      100  
#define tAttachWaitPoll 20    /* Periodic poll in AW.Src */
#define tAttachWaitAdv  20    /* Switch from Default to correct
                                                  advertisement in AW.Src */

/* Attach-Detach loop count - Halt after N loops */
#define MAX_CABLE_LOOP  20

void StateMachineTypeC(Port_t *port);
void StateMachineDisabled(Port_t *port);
void StateMachineErrorRecovery(Port_t *port);
void StateMachineUnattached(Port_t *port);

#ifdef CONFIG_FSC_HAVE_SNK
void StateMachineAttachWaitSink(Port_t *port);
void StateMachineAttachedSink(Port_t *port);
void StateMachineTryWaitSink(Port_t *port);
void StateMachineDebugAccessorySink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SNK */

#if (defined(CONFIG_FSC_HAVE_DRP) || \
     (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)))
void StateMachineTrySink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_DRP || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */

#ifdef CONFIG_FSC_HAVE_SRC
void StateMachineUnattachedSourceOnly(Port_t *port);
void StateMachineAttachWaitSource(Port_t *port);
void StateMachineTryWaitSource(Port_t *port);
#ifdef CONFIG_FSC_HAVE_DRP
void StateMachineUnattachedSource(Port_t *port);    /* AW.Snk -> Unattached */
#endif /* CONFIG_FSC_HAVE_DRP */
void StateMachineAttachedSource(Port_t *port);
void StateMachineTrySource(Port_t *port);
void StateMachineDebugAccessorySource(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SRC */

#ifdef CONFIG_FSC_HAVE_ACCMODE
void StateMachineAttachWaitAccessory(Port_t *port);
void StateMachineAudioAccessory(Port_t *port);
void StateMachinePoweredAccessory(Port_t *port);
void StateMachineUnsupportedAccessory(Port_t *port);
void SetStateAudioAccessory(Port_t *port);
#endif /* CONFIG_FSC_HAVE_ACCMODE */

void SetStateErrorRecovery(Port_t *port);
void SetStateUnattached(Port_t *port);

#ifdef CONFIG_FSC_HAVE_SNK
void SetStateAttachWaitSink(Port_t *port);
void SetStateAttachedSink(Port_t *port);
void SetStateDebugAccessorySink(Port_t *port);
#ifdef CONFIG_FSC_HAVE_DRP
void RoleSwapToAttachedSink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_DRP */
void SetStateTryWaitSink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SNK */

#if (defined(CONFIG_FSC_HAVE_DRP) || \
     (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)))
void SetStateTrySink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_DRP || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */

#ifdef CONFIG_FSC_HAVE_SRC
void SetStateUnattachedSourceOnly(Port_t *port);
void SetStateAttachWaitSource(Port_t *port);
void SetStateAttachedSource(Port_t *port);
void SetStateDebugAccessorySource(Port_t *port);
#ifdef CONFIG_FSC_HAVE_DRP
void RoleSwapToAttachedSource(Port_t *port);
#endif /* CONFIG_FSC_HAVE_DRP */
void SetStateTrySource(Port_t *port);
void SetStateTryWaitSource(Port_t *port);
#ifdef CONFIG_FSC_HAVE_DRP
void SetStateUnattachedSource(Port_t *port);
#endif /* CONFIG_FSC_HAVE_DRP */
#endif /* CONFIG_FSC_HAVE_SRC */

#if (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE))
void SetStateAttachWaitAccessory(Port_t *port);
void SetStateUnsupportedAccessory(Port_t *port);
void SetStatePoweredAccessory(Port_t *port);
#endif /* (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)) */

void SetStateIllegalCable(Port_t *port);
void StateMachineIllegalCable(Port_t *port);

void updateSourceCurrent(Port_t *port);
void updateSourceMDACHigh(Port_t *port);
void updateSourceMDACLow(Port_t *port);

void ToggleMeasure(Port_t *port);

CCTermType DecodeCCTermination(Port_t *port);
#if defined(CONFIG_FSC_HAVE_SRC) || \
    (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE))
CCTermType DecodeCCTerminationSource(Port_t *port);
bool IsCCPinRa(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SRC || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */
#ifdef CONFIG_FSC_HAVE_SNK
CCTermType DecodeCCTerminationSink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SNK */

void UpdateSinkCurrent(Port_t *port, CCTermType term);
bool VbusVSafe0V (Port_t *port);

#ifdef CONFIG_FSC_HAVE_SNK
bool VbusUnder5V(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SNK */

bool isVSafe5V(Port_t *port);
bool isVBUSOverVoltage(Port_t *port, uint16_t vbus_mv);

void resetDebounceVariables(Port_t *port);
void setDebounceVariables(Port_t *port, CCTermType term);
void setDebounceVariables(Port_t *port, CCTermType term);
void debounceCC(Port_t *port);

#if defined(CONFIG_FSC_HAVE_SRC) || \
    (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE))
void setStateSource(Port_t *port, bool vconn);
void DetectCCPinSource(Port_t *port);
void updateVCONNSource(Port_t *port);
void updateVCONNSource(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SRC || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */

#ifdef CONFIG_FSC_HAVE_SNK
void setStateSink(Port_t *port);
void DetectCCPinSink(Port_t *port);
void updateVCONNSource(Port_t *port);
void updateVCONNSink(Port_t *port);
#endif /* CONFIG_FSC_HAVE_SNK */

void clearState(Port_t *port);

void UpdateCurrentAdvert(Port_t *port, USBTypeCCurrent Current);

#ifdef CONFIG_CONFIG_FSC_DEBUG
void SetStateDisabled(Port_t *port);

/* Returns local registers as data array */
bool GetLocalRegisters(Port_t *port, uint8_t * data, int32_t size);

void setAlternateModes(uint8_t mode);
uint8_t getAlternateModes(void);
#endif /* CONFIG_CONFIG_FSC_DEBUG */

#endif/* _FSC_TYPEC_H_ */

