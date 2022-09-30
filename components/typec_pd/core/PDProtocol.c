/*******************************************************************************
 * @file     PDProtocol.c
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
#include "platform.h"
#include "fusb30X.h"
#include "TypeC.h"
#include "PDPolicy.h"
#include "PDProtocol.h"
#include "PD_Types.h"

#ifdef CONFIG_FSC_HAVE_VDM
#include "vdm/vdm_callbacks.h"
#include "vdm/vdm_callbacks_defs.h"
#include "vdm/vdm.h"
#include "vdm/vdm_types.h"
#include "vdm/bitfield_translators.h"
#endif /* CONFIG_FSC_HAVE_VDM */

/* USB PD Protocol Layer Routines */
void USBPDProtocol(Port_t *port)
{
    if (port->Registers.Status.I_HARDRST ||
        port->Registers.Status.I_HARDSENT)
    {
        ResetProtocolLayer(port, true);
        if (port->PolicyIsSource)
        {
            TimerStart(&port->PolicyStateTimer, tPSHardReset);
            SetPEState(port, peSourceTransitionDefault);
        }
        else
        {
          SetPEState(port, peSinkTransitionDefault);
        }

#ifdef CONFIG_CONFIG_FSC_DEBUG
        if (port->Registers.Status.I_HARDSENT)
            StoreUSBPDToken(port, false, pdtHardResetTxd);
        else
            StoreUSBPDToken(port, false, pdtHardResetRxd);
#endif /* CONFIG_CONFIG_FSC_DEBUG */
    }
    else
    {
        switch (port->ProtocolState)
        {
        case PRLReset:
            /* Sending a hard reset. */
            ProtocolSendHardReset(port);
            port->PDTxStatus = txWait;
            port->ProtocolState = PRLResetWait;
            break;
        case PRLResetWait:
            /* Wait on hard reset signaling */
            ProtocolResetWait(port);
            break;
        case PRLIdle:
            /* Wait on Tx/Rx */
            ProtocolIdle(port);
            break;
        case PRLTxSendingMessage:
            /* Wait on Tx to finish */
            ProtocolSendingMessage(port);
            break;
        case PRLTxVerifyGoodCRC:
            /* Verify returned GoodCRC */
            ProtocolVerifyGoodCRC(port);
            break;
        case PRLDisabled:
            break;
        default:
            break;
        }
    }
}

void ProtocolSendCableReset(Port_t *port)
{
    port->ProtocolTxBytes = 0;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = RESET1;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = RESET1;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC3_TOKEN;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = TXOFF;
    DeviceWrite(port->I2cAddr, regFIFO, port->ProtocolTxBytes,
                &port->ProtocolTxBuffer[0]);

    port->Registers.Control.TX_START = 1;
    DeviceWrite(port->I2cAddr, regControl0, 1, &port->Registers.Control.byte[0]);
    port->Registers.Control.TX_START = 0;
    port->MessageIDCounter[port->ProtocolMsgTxSop] = 0;
    port->MessageID[port->ProtocolMsgTxSop] = 0xFF;
#ifdef CONFIG_CONFIG_FSC_DEBUG
    StoreUSBPDToken(port, true, pdtCableReset);
#endif /* CONFIG_CONFIG_FSC_DEBUG */
}

void ProtocolIdle(Port_t *port)
{
    if (port->PDTxStatus == txReset)
    {
        /* Need to send a reset? */
        port->ProtocolState = PRLReset;
    }
#ifdef FSC_GSCE_FIX
    else if (port->Registers.Status.I_CRC_CHK || port->ProtocolMsgRxPending)
    {
        /* Using the CRC CHK interrupt as part of the GSCE workaround. */
        if (port->DoTxFlush)
        {
            ProtocolFlushTxFIFO(port);
            port->DoTxFlush = false;
        }

        if (!port->ProtocolMsgRx)
        {
            /* Only read in a new msg if there isn't already one waiting
             * to be processed by the PE.
             */
            ProtocolGetRxPacket(port, false);
        }
        else
        {
            /* Otherwise make a note to handle it later. */
            port->ProtocolMsgRxPending = true;
        }

        port->Registers.Status.I_CRC_CHK = 0;
    }
#else
    else if (port->Registers.Status.I_GCRCSENT || port->ProtocolMsgRxPending)
    {
        /* Received a message and sent a GoodCRC in response? */
        if (port->DoTxFlush)
        {
            ProtocolFlushTxFIFO(port);
            port->DoTxFlush = false;
        }

        if (!port->ProtocolMsgRx)
        {
            /* Only read in a new msg if there isn't already one waiting
             * to be processed by the PE.
             */
            ProtocolGetRxPacket(port, false);
        }
        else
        {
            /* Otherwise make a note to handle it later. */
            port->ProtocolMsgRxPending = true;
        }

        port->Registers.Status.I_GCRCSENT = 0;
    }
#endif /* FSC_GSCE_FIX */
    else if (port->PDTxStatus == txSend)
    {
        /* Have a message to send? */
        if (port->ProtocolMsgRx || port->ProtocolMsgRxPending)
        {
            port->PDTxStatus = txAbort;
        }
        else
        {
            ProtocolTransmitMessage(port);
        }
    }
#ifdef CONFIG_FSC_HAVE_EXT_MSG
    else if (port->ExtTxOrRx != NoXfer && port->ExtWaitTxRx == false)
    {
        port->PDTxStatus = txSend;
    }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
}

void ProtocolResetWait(Port_t *port)
{
    if (port->Registers.Status.I_HARDSENT)
    {
        port->ProtocolState = PRLIdle;
        port->PDTxStatus = txSuccess;
    }
}

void ProtocolGetRxPacket(Port_t *port, bool HeaderReceived)
{
#ifdef CONFIG_CONFIG_FSC_DEBUG
    sopMainHeader_t logHeader;
#endif /* CONFIG_CONFIG_FSC_DEBUG */
    uint32_t i = 0, j = 0;
    uint8_t data[4];

    port->ProtocolMsgRxPending = false;

    TimerDisable(&port->ProtocolTimer);

    /* Update to make sure GetRxPacket can see a valid RxEmpty value */
    DeviceRead(port->I2cAddr, regStatus1, 1,
               &port->Registers.Status.byte[5]);

    if (port->Registers.Status.RX_EMPTY == 1)
    {
        /* Nothing to see here... */
        return;
    }

    if (HeaderReceived == false)
    {
        /* Read the Rx token and two header bytes */
        DeviceRead(port->I2cAddr, regFIFO, 3, &data[0]);
        port->PolicyRxHeader.byte[0] = data[1];
        port->PolicyRxHeader.byte[1] = data[2];

        /* Figure out what SOP* the data came in on */
        port->ProtocolMsgRxSop = TokenToSopType(data[0]);
    }
    else
    {
        /* PolicyRxHeader, ProtocolMsgRxSop already set */
    }

#ifdef FSC_GSCE_FIX
    /* This is the received GetSourceCapsExtended workaround */
    /* The FUSB302B doesn't send an automatic GoodCRC for this message,
     * so this fix tries to send a fast manual GoodCRC.  Depending on how fast
     * this code is run, it might be a bit late or it might occur after
     * a retry of the GSCE message.
     */
    if (port->PolicyRxHeader.NumDataObjects == 0 &&
        port->PolicyRxHeader.MessageType == CMTGetSourceCapExt &&
        port->SendFastGoodCRC == false)
    {
        /* Pre-load manual GoodCRC */
        port->FastGoodCRCBuffer[5] |= port->PolicyIsDFP << 5;
        port->FastGoodCRCBuffer[6] |= port->PolicyIsSource;
        port->FastGoodCRCBuffer[6] |= port->PolicyRxHeader.MessageID << 1;
        DeviceWrite(port->I2cAddr, regFIFO, 10, port->FastGoodCRCBuffer);

        port->SendFastGoodCRC = true;
        return;
    }
    else if(port->SendFastGoodCRC == false)
    {
        /* For normal usage, wait on the GCRCSENT interrupt.  Should
         * be no longer than 700us later
         */
        /* I_GCRCSENT can be missed (rare), so this is a simple delay
         * instead of a re-read of the interrupt register.
         */
        platform_delay_10us(70); /* 700us */
    }
    port->SendFastGoodCRC = false;
#endif /* FSC_GSCE_FIX */

#ifdef CONFIG_CONFIG_FSC_DEBUG
    /* Log the auto goodcrc */
    logHeader.word = 0;
    logHeader.NumDataObjects = 0;
    logHeader.MessageType = CMTGoodCRC;
    logHeader.PortDataRole = port->PolicyIsDFP;
    logHeader.PortPowerRole = port->PolicyIsSource;
    logHeader.SpecRevision = DPM_SpecRev(port, port->ProtocolMsgRxSop);
    logHeader.MessageID = port->PolicyRxHeader.MessageID;
#endif /* CONFIG_CONFIG_FSC_DEBUG */

    if (port->ProtocolMsgRxSop == SOP_TYPE_ERROR)
    {
        /* SOP result can be corrupted in rare cases, or possibly if
         * the FIFO reads get out of sync.
         * TODO - Flush?
         */
        return;
    }

    if ((port->PolicyRxHeader.NumDataObjects == 0) &&
        (port->PolicyRxHeader.MessageType == CMTSoftReset))
    {
        /* For a soft reset, reset ID's, etc. */
        port->MessageIDCounter[port->ProtocolMsgRxSop] = 0;
        port->MessageID[port->ProtocolMsgRxSop] = 0xFF;
        port->ProtocolMsgRx = true;
#ifdef CONFIG_CONFIG_FSC_DEBUG
        port->SourceCapsUpdated = true;
#endif /* CONFIG_CONFIG_FSC_DEBUG */
    }
    else if (port->PolicyRxHeader.MessageID !=
             port->MessageID[port->ProtocolMsgRxSop])
    {
        port->MessageID[port->ProtocolMsgRxSop] =
            port->PolicyRxHeader.MessageID;
        port->ProtocolMsgRx = true;
    }

    if ((port->PolicyRxHeader.NumDataObjects == 0) &&
        (port->PolicyRxHeader.MessageType == CMTGoodCRC))
    {
        /* Rare cases may result in the next GoodCRC being processed before
         * the expected current message.  Handle and continue on to next msg.
         */

        /* Read out the 4 CRC bytes to move the address to the next packet */
        DeviceRead(port->I2cAddr, regFIFO, 4, data);

        port->ProtocolState = PRLIdle;
        port->PDTxStatus = txSuccess;
        port->ProtocolMsgRx = false;

        ProtocolGetRxPacket(port, false);

        return;
    }
    else if (port->PolicyRxHeader.NumDataObjects > 0)
    {
        /* Data message - Grab the data from the FIFO, including 4 byte CRC */
        DeviceRead(port->I2cAddr, regFIFO,
                   ((port->PolicyRxHeader.NumDataObjects << 2) + 4),
                   &port->ProtocolRxBuffer[0]);

#ifdef CONFIG_FSC_HAVE_EXT_MSG
        if (port->PolicyRxHeader.Extended == 1)
        {
            /* Copy ext header first */
            port->ExtRxHeader.byte[0] = port->ProtocolRxBuffer[0];
            port->ExtRxHeader.byte[1] = port->ProtocolRxBuffer[1];

            if (port->ExtRxHeader.ReqChunk == 1)
            {
                /* Received request for another chunk. Continue sending....*/
                port->ExtWaitTxRx = false;
                if (port->ExtRxHeader.ChunkNum < port->ExtChunkNum)
                {
                    /* Resend the previous chunk */
                    port->ExtChunkNum = port->ExtRxHeader.ChunkNum;
                    port->ExtChunkOffset =
                        port->ExtChunkNum * EXT_MAX_MSG_LEGACY_LEN;
                }
            }
            else
            {
                if (port->ExtRxHeader.DataSize > EXT_MAX_MSG_LEGACY_LEN)
                {
                    if (port->ExtRxHeader.DataSize > 260)
                    {
                        port->ExtRxHeader.DataSize = 260;
                    }
                    if (port->ExtRxHeader.ChunkNum == 0)
                    {
                        port->ExtChunkOffset = 0;   /* First message */
                        port->ExtChunkNum = 1;      /* Next chunk number */
                    }
                }
            }
        }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
        for (i = 0; i < port->PolicyRxHeader.NumDataObjects; i++)
        {
            for (j = 0; j < 4; j++)
            {
#ifdef CONFIG_FSC_HAVE_EXT_MSG
                if (port->PolicyRxHeader.Extended == 1)
                {
                    /* Skip ext header */
                    if (i == 0 && (j == 0 || j == 1)){continue;}

                    if (port->ExtRxHeader.ReqChunk == 0)
                    {
                        port->ExtMsgBuffer[port->ExtChunkOffset++] =
                                port->ProtocolRxBuffer[j + (i << 2)];
                    }
                }
                else
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
                {
                    port->PolicyRxDataObj[i].byte[j] =
                            port->ProtocolRxBuffer[j + (i << 2)];
                }
            }
       }

#ifdef CONFIG_FSC_HAVE_EXT_MSG
        if (port->PolicyRxHeader.Extended == 1)
        {
            if (port->ExtRxHeader.ReqChunk == 0)
            {
                if (port->ExtChunkOffset < port->ExtRxHeader.DataSize)
                {
                    /* more message left. continue receiving */
                    port->ExtTxOrRx = RXing;
                    port->ProtocolMsgRx = false;
                    port->ExtWaitTxRx = false;
                }
                else
                {
                    port->ExtTxOrRx = NoXfer;
                    port->ProtocolMsgRx = true;
                }
            }
            else if (port->ExtRxHeader.ReqChunk == 1 &&
                     port->ExtChunkOffset < port->ExtTxHeader.DataSize)
            {
                port->ExtWaitTxRx = false;
            }
            else
            {
                /* Last message received */
                port->ExtTxOrRx = NoXfer;
            }
        }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
    }
    else
    {
        /* Command message */
        /* Read out the 4 CRC bytes to move the address to the next packet */
        DeviceRead(port->I2cAddr, regFIFO, 4, data);
    }

#ifdef CONFIG_CONFIG_FSC_DEBUG
    /* Store the received PD message for the device policy manager */
    StoreUSBPDMessage(port, port->PolicyRxHeader,
                      (doDataObject_t*)port->ProtocolRxBuffer,
                      false, port->ProtocolMsgRxSop);

    /* Store the GoodCRC message that we have sent (SOP) */
    StoreUSBPDMessage(port, logHeader, &port->PolicyTxDataObj[0], true,
                      port->ProtocolMsgRxSop);

    /*
     * Special debug case where PD state log will provide the time elapsed
     * in this function, and the number of I2C bytes read during this period.
     */
    WriteStateLog(&port->PDStateLog, dbgGetRxPacket, platform_get_log_time());
#endif /* CONFIG_CONFIG_FSC_DEBUG */

    /* Special VDM use case where a second message appears too quickly */
    if ((port->PolicyRxHeader.NumDataObjects != 0) &&
        (port->PolicyRxHeader.MessageType == DMTVenderDefined) &&
        (port->PolicyRxDataObj[0].SVDM.CommandType == 0)) /* Initiator */
    {
        /* Delay and check if a new mesage has been received */
        /* Note: May need to increase this delay (2-3ms) or find alternate
         * method for some slow systems - e.g. Android.
         */
        platform_delay_10us(100); /* 1ms */

        DeviceRead(port->I2cAddr, regInterruptb, 3,
                   &port->Registers.Status.byte[3]);

        if (port->Registers.Status.I_GCRCSENT &&
            !port->Registers.Status.RX_EMPTY)
        {
            /* Get the next message - overwriting the current message */
            ProtocolGetRxPacket(port, false);
        }
    }
    else
    {
        /* A quickly sent second message can be received
         * into the buffer without triggering an (additional) interrupt.
         */
        DeviceRead(port->I2cAddr, regStatus0, 2,
            &port->Registers.Status.byte[4]);

        if (!port->Registers.Status.ACTIVITY &&
            !port->Registers.Status.RX_EMPTY)
        {
            platform_delay_10us(50);
            DeviceRead(port->I2cAddr, regStatus0, 2,
                        &port->Registers.Status.byte[4]);

            if (!port->Registers.Status.ACTIVITY &&
                !port->Registers.Status.RX_EMPTY)
            {
                port->ProtocolMsgRxPending = true;
            }
        }
    }

    /* If a message has been received during an attempt to transmit,
     * abort and handle the received message before trying again.
     */
    if (port->ProtocolMsgRx && (port->PDTxStatus == txSend))
    {
        port->PDTxStatus = txAbort;
    }
}

void ProtocolTransmitMessage(Port_t *port)
{
    uint32_t i, j;
    sopMainHeader_t temp_PolicyTxHeader = { 0 };

    port->DoTxFlush = false;

    /* Note: Power needs to be set a bit before we write TX_START to update */
    ProtocolLoadSOP(port, port->ProtocolMsgTxSop);

#ifdef CONFIG_FSC_HAVE_EXT_MSG
    if (port->ExtTxOrRx == RXing)
    {
        /* Set up chunk request */
        temp_PolicyTxHeader.word = port->PolicyRxHeader.word;
        temp_PolicyTxHeader.PortPowerRole = port->PolicyIsSource;
        temp_PolicyTxHeader.PortDataRole = port->PolicyIsDFP;
    }
    else
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
    {
        temp_PolicyTxHeader.word = port->PolicyTxHeader.word;
    }

    if ((temp_PolicyTxHeader.NumDataObjects == 0) &&
        (temp_PolicyTxHeader.MessageType == CMTSoftReset))
    {
        port->MessageIDCounter[port->ProtocolMsgTxSop] = 0;
        port->MessageID[port->ProtocolMsgTxSop] = 0xFF;
#ifdef CONFIG_CONFIG_FSC_DEBUG
        port->SourceCapsUpdated = true;
#endif /* CONFIG_CONFIG_FSC_DEBUG */
    }

#ifdef CONFIG_FSC_HAVE_EXT_MSG
    if (temp_PolicyTxHeader.Extended == 1)
    {
        if (port->ExtTxOrRx == TXing)
        {
            /* Remaining bytes */
            i = port->ExtTxHeader.DataSize - port->ExtChunkOffset;

            if (i > EXT_MAX_MSG_LEGACY_LEN)
            {
                temp_PolicyTxHeader.NumDataObjects = 7;
            }
            else
            {
                /* Round up to 4 byte boundary.
                 * Two extra byte is for the extended header.
                 */
                temp_PolicyTxHeader.NumDataObjects = (i + 4) / 4;
            }
            port->PolicyTxHeader.NumDataObjects =
                    temp_PolicyTxHeader.NumDataObjects;
            port->ExtTxHeader.ChunkNum = port->ExtChunkNum;
        }
        else if (port->ExtTxOrRx == RXing)
        {
            temp_PolicyTxHeader.NumDataObjects = 1;
        }
        port->ExtWaitTxRx = true;
    }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */

    temp_PolicyTxHeader.MessageID =
            port->MessageIDCounter[port->ProtocolMsgTxSop];
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
            PACKSYM | (2 + (temp_PolicyTxHeader.NumDataObjects << 2));
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
            temp_PolicyTxHeader.byte[0];
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
            temp_PolicyTxHeader.byte[1];

    /* If this is a data object... */
    if (temp_PolicyTxHeader.NumDataObjects > 0)
    {
#ifdef CONFIG_FSC_HAVE_EXT_MSG
        if (temp_PolicyTxHeader.Extended == 1)
        {
            if (port->ExtTxOrRx == RXing)
            {
                port->ExtTxHeader.ChunkNum = port->ExtChunkNum;
                port->ExtTxHeader.DataSize = 0;
                port->ExtTxHeader.Chunked = 1;
                port->ExtTxHeader.ReqChunk = 1;
            }
            else if (port->ExtTxOrRx == TXing)
            {
                port->ExtTxHeader.ChunkNum = port->ExtChunkNum;
            }

            /* Copy the two byte extended header. */
            port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
                    port->ExtTxHeader.byte[0];
            port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
                    port->ExtTxHeader.byte[1];
        }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
        for (i = 0; i < temp_PolicyTxHeader.NumDataObjects; i++)
        {
            for (j = 0; j < 4; j++)
            {
#ifdef CONFIG_FSC_HAVE_EXT_MSG
                if (temp_PolicyTxHeader.Extended == 1)
                {
                    /* Skip extended header */
                    if (i == 0 && (j == 0 || j == 1)) { continue; }

                    if (port->ExtChunkOffset < port->ExtTxHeader.DataSize)
                    {
                        port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
                                port->ExtMsgBuffer[port->ExtChunkOffset++];
                    }
                    else
                    {
                        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = 0;
                    }
                }
                else
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
                {
                    /* Load the actual bytes */
                    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
                            port->PolicyTxDataObj[i].byte[j];
                }
            }
        }
    }

    /* Load the CRC, EOP and stop sequence */
    ProtocolLoadEOP(port);

    /* Commit the FIFO to the device */
    if (DeviceWrite(port->I2cAddr, regFIFO, port->ProtocolTxBytes,
                    &port->ProtocolTxBuffer[0]) == false)
    {
        /* If a FIFO write happens while a GoodCRC is being transmitted,
         * the transaction will NAK and will need to be discarded.
         */
        port->DoTxFlush = true;
        port->PDTxStatus = txAbort;
        return;
    }

    port->Registers.Control.N_RETRIES =
            DPM_Retries(port, port->ProtocolMsgTxSop);
    port->Registers.Control.AUTO_RETRY = 1;

    DeviceWrite(port->I2cAddr, regControl3, 1,&port->Registers.Control.byte[3]);
    port->Registers.Control.TX_START = 1;
    DeviceWrite(port->I2cAddr, regControl0, 1,&port->Registers.Control.byte[0]);
    port->Registers.Control.TX_START = 0;

    /* Set the transmitter status to busy */
    port->PDTxStatus = txBusy;
    port->ProtocolState = PRLTxSendingMessage;

    /* Timeout specifically for chunked messages, but used with each transmit
     * to prevent a theoretical protocol hang.
     */
    TimerStart(&port->ProtocolTimer, tChunkSenderRequest);

#ifdef CONFIG_CONFIG_FSC_DEBUG
    /* Store all messages that we attempt to send for debugging (SOP) */
    StoreUSBPDMessage(port, temp_PolicyTxHeader,
                      (doDataObject_t*)&port->ProtocolTxBuffer[7],
                      true, port->ProtocolMsgTxSop);
    WriteStateLog(&port->PDStateLog, dbgSendTxPacket, platform_get_log_time());
#endif /* CONFIG_CONFIG_FSC_DEBUG */
}

void ProtocolSendingMessage(Port_t *port)
{

    /* Waiting on result/status of transmission */
    if (port->Registers.Status.I_TXSENT || port->Registers.Status.I_CRC_CHK)
    {
        port->Registers.Status.I_TXSENT = 0;
        port->Registers.Status.I_CRC_CHK = 0;
        ProtocolVerifyGoodCRC(port);
    }
    else if (port->Registers.Status.I_COLLISION)
    {
        port->Registers.Status.I_COLLISION = 0;
        port->PDTxStatus = txCollision;
        port->ProtocolState = PRLIdle;
    }
    else if (port->Registers.Status.I_RETRYFAIL)
    {
        port->Registers.Status.I_RETRYFAIL = 0;
        port->PDTxStatus = txError;
        port->ProtocolState = PRLIdle;
    }
    else if (port->Registers.Status.I_GCRCSENT)
    {
        /* Interruption */
        port->PDTxStatus = txError;
        port->ProtocolState = PRLIdle;
        port->ProtocolMsgRxPending = true;
        port->Registers.Status.I_GCRCSENT = 0;
    }

    /* Make an additional check for missed/pending message data */
    if (port->ProtocolState == PRLIdle)
    {
        ProtocolIdle(port);
    }
}

void ProtocolVerifyGoodCRC(Port_t *port)
{
    uint8_t data[4];
    sopMainHeader_t header;
    SopType sop;

    /* Read the Rx token and two header bytes */
    DeviceRead(port->I2cAddr, regFIFO, 3, &data[0]);
    header.byte[0] = data[1];
    header.byte[1] = data[2];

    /* Figure out what SOP* the data came in on */
    sop = TokenToSopType(data[0]);

    if ((header.NumDataObjects == 0) &&
        (header.MessageType == CMTGoodCRC))
    {
        uint8_t MIDcompare;
        if (sop == SOP_TYPE_ERROR)
            MIDcompare = 0xFF;
        else
            MIDcompare = port->MessageIDCounter[sop];

#ifdef CONFIG_CONFIG_FSC_DEBUG
        /* Store the received PD message for the DPM (GUI) */
        StoreUSBPDMessage(port, header, &port->PolicyRxDataObj[0],
                          false, sop);
#endif /* CONFIG_CONFIG_FSC_DEBUG */

        if (header.MessageID != MIDcompare)
        {
            /* Read out the 4 CRC bytes to move the addr to the next packet */
            DeviceRead(port->I2cAddr, regFIFO, 4, data);
#ifdef CONFIG_CONFIG_FSC_DEBUG
            /* Store that there was a bad message ID received in the buffer */
            StoreUSBPDToken(port, false, pdtBadMessageID);
#endif /* CONFIG_CONFIG_FSC_DEBUG */
            port->PDTxStatus = txError;
            port->ProtocolState = PRLIdle;
        }
        else
        {
            if (sop != SOP_TYPE_ERROR)
            {
                /* Increment and roll over */
                port->MessageIDCounter[sop]++;
                port->MessageIDCounter[sop] &= 0x07;
#ifdef CONFIG_FSC_HAVE_EXT_MSG
                if (port->ExtTxOrRx != NoXfer)
                {
                    if (port->ExtChunkOffset >= port->ExtTxHeader.DataSize)
                    {
                        /* All data has been sent */
                        port->ExtTxOrRx = NoXfer;
                    }
                    port->ExtChunkNum++;
                }
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
            }
            port->ProtocolState = PRLIdle;
            port->PDTxStatus = txSuccess;

            /* Read out the 4 CRC bytes to move the addr to the next packet */
            DeviceRead(port->I2cAddr, regFIFO, 4, data);
        }
    }
    else
    {
        port->ProtocolState = PRLIdle;
        port->PDTxStatus = txError;

        /* Pass header and SOP* on to GetRxPacket */
        port->PolicyRxHeader.word = header.word;
        port->ProtocolMsgRxSop = sop;

        /* Rare case, next received message preempts GoodCRC */
        ProtocolGetRxPacket(port, true);
    }
}

SopType DecodeSopFromPdMsg(uint8_t msg0, uint8_t msg1)
{
    /* this SOP* decoding is based on FUSB302 GUI:
     * SOP   => 0b 0xx1 xxxx xxx0 xxxx
     * SOP'  => 0b 1xx1 xxxx xxx0 xxxx
     * SOP'' => 0b 1xx1 xxxx xxx1 xxxx
     */
    if (((msg1 & 0x90) == 0x10) && ((msg0 & 0x10) == 0x00))
        return SOP_TYPE_SOP;
    else if (((msg1 & 0x90) == 0x90) && ((msg0 & 0x10) == 0x00))
        return SOP_TYPE_SOP1;
    else if (((msg1 & 0x90) == 0x90) && ((msg0 & 0x10) == 0x10))
        return SOP_TYPE_SOP2;
    else
        return SOP_TYPE_SOP;
}

void ProtocolSendGoodCRC(Port_t *port, SopType sop)
{
    ProtocolLoadSOP(port, sop);

    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = PACKSYM | 0x02;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
            port->PolicyTxHeader.byte[0];
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] =
            port->PolicyTxHeader.byte[1];
    ProtocolLoadEOP(port);
    DeviceWrite(port->I2cAddr, regFIFO, port->ProtocolTxBytes,
                &port->ProtocolTxBuffer[0]);
}

void ProtocolLoadSOP(Port_t *port, SopType sop)
{
    /* Clear the Tx byte counter */
    port->ProtocolTxBytes = 0;

    switch (sop)
    {
    case SOP_TYPE_SOP1:
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC3_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC3_TOKEN;
        break;
    case SOP_TYPE_SOP2:
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC3_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC3_TOKEN;
        break;
    case SOP_TYPE_SOP:
    default:
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC1_TOKEN;
        port->ProtocolTxBuffer[port->ProtocolTxBytes++] = SYNC2_TOKEN;
        break;
    }
}

void ProtocolLoadEOP(Port_t *port)
{
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = JAM_CRC;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = EOP;
    port->ProtocolTxBuffer[port->ProtocolTxBytes++] = TXOFF;
}

void ProtocolSendHardReset(Port_t *port)
{
    uint8_t data = port->Registers.Control.byte[3] | 0x40;  /* Hard Reset bit */

    /* If the shortcut flag is set, we've already sent the HR command */
    if (port->WaitingOnHR)
    {
        port->WaitingOnHR = false;
    }
    else
    {
        DeviceWrite(port->I2cAddr, regControl3, 1, &data);
    }
}

void ProtocolFlushRxFIFO(Port_t *port)
{
    uint8_t data = port->Registers.Control.byte[1] | 0x04;  /* RX_FLUSH bit */
    DeviceWrite(port->I2cAddr, regControl1, 1, &data);
}

void ProtocolFlushTxFIFO(Port_t *port)
{
    uint8_t data = port->Registers.Control.byte[0] | 0x40;  /* TX_FLUSH bit */
    DeviceWrite(port->I2cAddr, regControl0, 1, &data);
}

void ResetProtocolLayer(Port_t *port, bool ResetPDLogic)
{
    uint32_t i;
    uint8_t data = 0x02; /* PD_RESET bit */

    if (ResetPDLogic)
    {
        DeviceWrite(port->I2cAddr, regReset, 1, &data);
    }

    port->ProtocolState = PRLIdle;
    port->PDTxStatus = txIdle;

    port->WaitingOnHR = false;

#ifdef CONFIG_FSC_HAVE_VDM
    TimerDisable(&port->VdmTimer);
    port->VdmTimerStarted = false;
#endif /* CONFIG_FSC_HAVE_VDM */

    port->ProtocolTxBytes = 0;

    for (i = 0; i < SOP_TYPE_NUM; i++)
    {
        port->MessageIDCounter[i] = 0;
        port->MessageID[i] = 0xFF;
    }

    port->ProtocolMsgRx = false;
    port->ProtocolMsgRxSop = SOP_TYPE_SOP;
    port->ProtocolMsgRxPending = false;
    port->USBPDTxFlag = false;
    port->PolicyHasContract = false;
    port->USBPDContract.object = 0;

#ifdef CONFIG_CONFIG_FSC_DEBUG
    port->SourceCapsUpdated = true;
#endif // CONFIG_CONFIG_FSC_DEBUG

    port->SrcCapsHeaderReceived.word = 0;
    port->SnkCapsHeaderReceived.word = 0;
    for (i = 0; i < 7; i++)
    {
        port->SrcCapsReceived[i].object = 0;
        port->SnkCapsReceived[i].object = 0;
    }

#ifdef CONFIG_FSC_HAVE_EXT_MSG
    port->ExtWaitTxRx = false;
    port->ExtChunkNum = 0;
    port->ExtTxOrRx = NoXfer;
    port->ExtChunkOffset = 0;
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
    
    port->SendFastGoodCRC = false;
}

#ifdef CONFIG_CONFIG_FSC_DEBUG
/* USB PD Debug Buffer Routines */
bool StoreUSBPDToken(Port_t *port, bool transmitter,
                         USBPD_BufferTokens_t token)
{
    uint8_t header1 = 1;

    if (ClaimBufferSpace(port, 2) == false)
    {
        return false;
    }

    if (transmitter)
    {
        header1 |= 0x40;
    }

    port->USBPDBuf[port->USBPDBufEnd++] = header1;
    port->USBPDBufEnd %= PDBUFSIZE;
    token &= 0x0F;
    port->USBPDBuf[port->USBPDBufEnd++] = token;
    port->USBPDBufEnd %= PDBUFSIZE;

    return true;
}

bool StoreUSBPDMessage(Port_t *port, sopMainHeader_t Header,
                           doDataObject_t* DataObject,
                           bool transmitter, uint8_t SOPToken)
{
    uint32_t i, j, required;
    uint8_t header1;

    required = Header.NumDataObjects * 4 + 2 + 2;

    if (ClaimBufferSpace(port, required) == false)
    {
        return false;
    }

    header1 = (0x1F & (required - 1)) | 0x80;

    if (transmitter)
    {
        header1 |= 0x40;
    }

    port->USBPDBuf[port->USBPDBufEnd++] = header1;
    port->USBPDBufEnd %= PDBUFSIZE;
    port->USBPDBuf[port->USBPDBufEnd++] = SOPToken;
    port->USBPDBufEnd %= PDBUFSIZE;
    port->USBPDBuf[port->USBPDBufEnd++] = Header.byte[0];
    port->USBPDBufEnd %= PDBUFSIZE;
    port->USBPDBuf[port->USBPDBufEnd++] = Header.byte[1];
    port->USBPDBufEnd %= PDBUFSIZE;

    for (i = 0; i < Header.NumDataObjects; i++)
    {
        for (j = 0; j < 4; j++)
        {
            port->USBPDBuf[port->USBPDBufEnd++] = DataObject[i].byte[j];
            port->USBPDBufEnd %= PDBUFSIZE;
        }
    }

    return true;
}

uint8_t GetNextUSBPDMessageSize(Port_t *port)
{
    uint8_t numBytes = 0;

    if (port->USBPDBufStart == port->USBPDBufEnd)
    {
       numBytes = 0;
    }
    else
    {
        numBytes = (port->USBPDBuf[port->USBPDBufStart] & 0x1F) + 1;
    }

    return numBytes;
}

uint8_t GetUSBPDBufferNumBytes(Port_t *port)
{
    uint8_t bytes = 0;

    if (port->USBPDBufStart == port->USBPDBufEnd)
    {
        bytes = 0;
    }
    else if (port->USBPDBufEnd > port->USBPDBufStart)
    {
        bytes = port->USBPDBufEnd - port->USBPDBufStart;
    }
    else
    {
        bytes = port->USBPDBufEnd + (PDBUFSIZE - port->USBPDBufStart);
    }

    return bytes;
}

bool ClaimBufferSpace(Port_t *port, int32_t intReqSize)
{
    int32_t available;
    uint8_t numBytes;

    if (intReqSize >= PDBUFSIZE)
    {
        return false;
    }

    if (port->USBPDBufStart == port->USBPDBufEnd)
    {
        available = PDBUFSIZE;
    }
    else if (port->USBPDBufStart > port->USBPDBufEnd)
    {
        available = port->USBPDBufStart - port->USBPDBufEnd;
    }
    else
    {
        available = PDBUFSIZE - (port->USBPDBufEnd - port->USBPDBufStart);
    }

    do
    {
        /* Overwrite entries until space is adequate */
        if (intReqSize >= available)
        {
            port->USBPDBufOverflow = true;
            numBytes = GetNextUSBPDMessageSize(port);
            if (numBytes == 0)
            {
                return false;
            }
            available += numBytes;
            port->USBPDBufStart += numBytes;
            port->USBPDBufStart %= PDBUFSIZE;
        }
        else
        {
            break;
        }
    } while (1);

    return true;
}

/* USB HID Commmunication Routines */
uint8_t ReadUSBPDBuffer(Port_t *port, uint8_t* pData, uint8_t bytesAvail)
{
    uint8_t i, msgSize, bytesRead;
    bytesRead = 0;
    msgSize = GetNextUSBPDMessageSize(port);

    while (msgSize != 0 && msgSize < bytesAvail)
    {
        for (i = 0; i < msgSize; i++)
        {
            *pData++ = port->USBPDBuf[port->USBPDBufStart++];
            port->USBPDBufStart %= PDBUFSIZE;
        }
        bytesAvail -= msgSize;
        bytesRead += msgSize;
        msgSize = GetNextUSBPDMessageSize(port);
    }

    return bytesRead;
}

#endif /* CONFIG_CONFIG_FSC_DEBUG */
