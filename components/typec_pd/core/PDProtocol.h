/*******************************************************************************
 * @file     PDProtocol.h
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
#ifndef _PDPROTOCOL_H_
#define	_PDPROTOCOL_H_

/////////////////////////////////////////////////////////////////////////////
//                              Required headers
/////////////////////////////////////////////////////////////////////////////
#include "platform.h"
#include "Port.h"

#ifdef CONFIG_CONFIG_FSC_DEBUG
#include "Log.h"
#endif /* CONFIG_CONFIG_FSC_DEBUG */

/* USB PD Protocol Layer Routines */
void USBPDProtocol(Port_t *port);
void ProtocolSendCableReset(Port_t *port);
void ProtocolIdle(Port_t *port);
void ProtocolResetWait(Port_t *port);
void ProtocolRxWait(void);
void ProtocolGetRxPacket(Port_t *port, bool HeaderReceived);
void ProtocolTransmitMessage(Port_t *port);
void ProtocolSendingMessage(Port_t *port);
void ProtocolWaitForPHYResponse(void);
void ProtocolVerifyGoodCRC(Port_t *port);
void ProtocolSendGoodCRC(Port_t *port, SopType sop);
void ProtocolLoadSOP(Port_t *port, SopType sop);
void ProtocolLoadEOP(Port_t *port);
void ProtocolSendHardReset(Port_t *port);
void ProtocolFlushRxFIFO(Port_t *port);
void ProtocolFlushTxFIFO(Port_t *port);
void ResetProtocolLayer(Port_t *port, bool ResetPDLogic);

#ifdef CONFIG_CONFIG_FSC_DEBUG
/* Logging and debug functions */
bool StoreUSBPDToken(Port_t *port, bool transmitter,
                         USBPD_BufferTokens_t token);
bool StoreUSBPDMessage(Port_t *port, sopMainHeader_t Header,
                           doDataObject_t* DataObject,
                           bool transmitter, uint8_t SOPType);
uint8_t GetNextUSBPDMessageSize(Port_t *port);
uint8_t GetUSBPDBufferNumBytes(Port_t *port);
bool ClaimBufferSpace(Port_t *port, int32_t intReqSize);
uint8_t ReadUSBPDBuffer(Port_t *port, uint8_t* pData, uint8_t bytesAvail);

void SendUSBPDHardReset(Port_t *port);

#endif /* CONFIG_CONFIG_FSC_DEBUG */

#endif	/* _PDPROTOCOL_H_ */

