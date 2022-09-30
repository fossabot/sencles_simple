/*******************************************************************************
 * @file     Port.h
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
#ifndef _PORT_H_
#define _PORT_H_

#include "TypeC_Types.h"
#include "PD_Types.h"
#include "fusb30X.h"
#include "platform.h"
#ifdef CONFIG_FSC_HAVE_VDM
#include "vdm_callbacks_defs.h"
#endif /* CONFIG_FSC_HAVE_VDM */
#ifdef CONFIG_FSC_HAVE_DP
#include "dp.h"
#endif /* CONFIG_FSC_HAVE_DP */
#include "observer.h"
#include "Log.h"
#include "dpm.h"
#include "timer.h"

/* Size of Rx/Tx FIFO protocol buffers */
#define FSC_PROTOCOL_BUFFER_SIZE 64

/* Number of timer objects in list */
#define FSC_NUM_TIMERS 10

/**
 * All state variables here for now.
 */
typedef struct Port
{
    DevicePolicy_t*         dpm;
    PortConfig_t            PortConfig;
    uint8_t                  PortID;                     /* Optional Port ID */
    uint8_t                  I2cAddr;
    DeviceReg_t             Registers;
    bool                TCIdle;                     /* True: Type-C idle */
    bool                PEIdle;                     /* True: PE idle */

    /* All Type C State Machine variables */
    CCTermType              CCTerm;                     /* Active CC */
    CCTermType              CCTermCCDebounce;           /* Debounced CC */
    CCTermType              CCTermPDDebounce;
    CCTermType              CCTermPDDebouncePrevious;
    CCTermType              VCONNTerm;

    SourceOrSink            sourceOrSink;               /* TypeC Src or Snk */
    USBTypeCCurrent         SinkCurrent;                /* PP Current */
    USBTypeCCurrent         SourceCurrent;              /* Our Current */
    CCOrientation           CCPin;                      /* CC == CC1 or CC2 */
    bool                SMEnabled;               /* TypeC SM Enabled */
    ConnectionState         ConnState;                  /* TypeC State */
    uint8_t                  TypeCSubState;              /* TypeC Substate */
    uint16_t                 DetachThreshold;            /* TypeC detach level */
    uint8_t                  loopCounter;                /* Count and prevent
                                                           attach/detach loop */
    bool                C2ACable;                   /* Possible C-to-A type
                                                           cable detected */
    /* All Policy Engine variables */
    PolicyState_t           PolicyState;                /* PE State */
    uint8_t                  PolicySubIndex;             /* PE Substate */
    SopType                 PolicyMsgTxSop;             /* Tx to SOP? */
    bool                USBPDActive;                /* PE Active */
    bool                USBPDEnabled;               /* PE Enabled */
    bool                PolicyIsSource;             /* Policy Src/Snk? */
    bool                PolicyIsDFP;                /* Policy DFP/UFP? */
    bool                PolicyHasContract;          /* Have contract */
    bool                isContractValid;            /* PD Contract Valid */
    bool                IsHardReset;                /* HR is occurring */
    bool                IsPRSwap;                   /* PR is occurring */
    bool                IsVCONNSource;              /* VConn state */
    bool                USBPDTxFlag;                /* Have msg to Tx */
    uint8_t                  CollisionCounter;           /* Collisions for PE */
    uint8_t                  HardResetCounter;
    uint8_t                  CapsCounter;                /* Startup caps tx'd */

    sopMainHeader_t         src_cap_header;
    sopMainHeader_t         snk_cap_header;
    doDataObject_t          src_caps[7];
    doDataObject_t          snk_caps[7];

    uint8_t                  PdRevSop;              /* Partner spec rev */
    uint8_t                  PdRevCable;                 /* Cable spec rev */
    bool                PpsEnabled;                 /* True if PPS mode */

    bool                WaitingOnHR;                /* HR shortcut */
    bool                WaitSent;                   /* Waiting on PR Swap */

    bool                WaitInSReady;               /* Snk/SrcRdy Delay */

    /* All Protocol State Machine Variables */
    ProtocolState_t         ProtocolState;              /* Protocol State */
    sopMainHeader_t         PolicyRxHeader;             /* Header Rx'ing */
    sopMainHeader_t         PolicyTxHeader;             /* Header Tx'ing */
    sopMainHeader_t         PDTransmitHeader;           /* Header to Tx */
    sopMainHeader_t         SrcCapsHeaderReceived;      /* Recent caps */
    sopMainHeader_t         SnkCapsHeaderReceived;      /* Recent caps */
    doDataObject_t          PolicyRxDataObj[7];         /* Rx'ing objects */
    doDataObject_t          PolicyTxDataObj[7];         /* Tx'ing objects */
    doDataObject_t          PDTransmitObjects[7];       /* Objects to Tx */
    doDataObject_t          SrcCapsReceived[7];         /* Recent caps header */
    doDataObject_t          SnkCapsReceived[7];         /* Recent caps header */
    doDataObject_t          USBPDContract;              /* Current PD request */
    doDataObject_t          SinkRequest;                /* Sink request  */
    doDataObject_t          PartnerCaps;                /* PP's Sink Caps */
    doPDO_t                 vendor_info_source[7];      /* Caps def'd by VI */
    doPDO_t                 vendor_info_sink[7];        /* Caps def'd by VI */
    SopType                 ProtocolMsgRxSop;           /* SOP of msg Rx'd */
    SopType                 ProtocolMsgTxSop;           /* SOP of msg Tx'd */
    PDTxStatus_t            PDTxStatus;                 /* Protocol Tx state */
    bool                ProtocolMsgRx;              /* Msg Rx'd Flag */
    bool                ProtocolMsgRxPending;       /* Msg in FIFO */
    uint8_t                  ProtocolTxBytes;            /* Bytes to Tx */
    uint8_t                  ProtocolTxBuffer[FSC_PROTOCOL_BUFFER_SIZE];
    uint8_t                  ProtocolRxBuffer[FSC_PROTOCOL_BUFFER_SIZE];
    uint8_t                  MessageIDCounter[SOP_TYPE_NUM]; /* Local ID count */
    uint8_t                  MessageID[SOP_TYPE_NUM];    /* Local last msg ID */

    bool                DoTxFlush;                  /* Collision -> Flush */
    uint8_t                  FastGoodCRCBuffer[10];
    bool                SendFastGoodCRC;

    /* Timer objects */
    struct TimerObj         PDDebounceTimer;            /* First debounce */
    struct TimerObj         CCDebounceTimer;            /* Second debounce */
    struct TimerObj         StateTimer;                 /* TypeC state timer */
    struct TimerObj         LoopCountTimer;             /* Loop delayed clear */
    struct TimerObj         PolicyStateTimer;           /* PE state timer */
    struct TimerObj         ProtocolTimer;              /* Prtcl state timer */
    struct TimerObj         SwapSourceStartTimer;       /* PR swap delay */
    struct TimerObj         PpsTimer;                   /* PPS timeout timer */
    struct TimerObj         VBusPollTimer;              /* VBus monitor timer */
    struct TimerObj         VdmTimer;                   /* VDM timer */

    struct TimerObj         *Timers[FSC_NUM_TIMERS];

#ifdef CONFIG_FSC_HAVE_EXT_MSG
    ExtMsgState_t           ExtTxOrRx;                  /* Tx' or Rx'ing  */
    ExtHeader_t             ExtTxHeader;
    ExtHeader_t             ExtRxHeader;
    bool                ExtWaitTxRx;                /* Waiting to Tx/Rx */
    uint16_t                 ExtChunkOffset;             /* Next chunk offset */
    uint8_t                  ExtMsgBuffer[260];
    uint8_t                  ExtChunkNum;                /* Next chunk number */
#else
    ExtHeader_t             ExtHeader;                  /* For sending NS */
    bool                WaitForNotSupported;        /* Wait for timer */
#endif

#ifdef CONFIG_CONFIG_FSC_DEBUG
    bool                SourceCapsUpdated;          /* GUI new caps flag */
#endif /* CONFIG_CONFIG_FSC_DEBUG */

#ifdef CONFIG_FSC_HAVE_VDM
    VdmDiscoveryState_t     AutoVdmState;
    VdmManager              vdmm;
    PolicyState_t           vdm_next_ps;
    PolicyState_t           originalPolicyState;
    SvidInfo                core_svid_info;
    SopType                 VdmMsgTxSop;
    doDataObject_t          vdm_msg_obj[7];
    uint32_t                 my_mode;
    uint32_t                 vdm_msg_length;
    bool                sendingVdmData;
    bool                VdmTimerStarted;
    bool                vdm_timeout;
    bool                vdm_expectingresponse;
    bool                svid_enable;
    bool                mode_enable;
    bool                mode_entered;
    bool                AutoModeEntryEnabled;
    int32_t                 AutoModeEntryObjPos;
    int16_t                 svid_discvry_idx;
    bool                svid_discvry_done;
    uint16_t                 my_svid;
    uint16_t                 discoverIdCounter;
    bool                cblPresent;
    CableResetState_t       cblRstState;
#endif /* CONFIG_FSC_HAVE_VDM */

#ifdef CONFIG_FSC_HAVE_DP
    DisplayPortData_t       DisplayPortData;
#endif /* CONFIG_FSC_HAVE_DP */

#ifdef CONFIG_CONFIG_FSC_DEBUG
    StateLog                TypeCStateLog;          /* Log for TypeC states */
    StateLog                PDStateLog;             /* Log for PE states */
    uint8_t                  USBPDBuf[PDBUFSIZE];    /* Circular PD msg buffer */
    uint8_t                  USBPDBufStart;          /* PD msg buffer head */
    uint8_t                  USBPDBufEnd;            /* PD msg buffer tail */
    bool                USBPDBufOverflow;       /* PD Log overflow flag */
#endif /* CONFIG_CONFIG_FSC_DEBUG */
} Port_t;

/**
 * @brief Initializes the port structure and state machine behaviors
 *
 * Initializes the port structure with default values.
 * Also sets the i2c address of the device and enables the TypeC state machines.
 *
 * @param port Pointer to the port structure
 * @param i2c_address 8-bit value with bit zero (R/W bit) set to zero.
 * @return None
 */
void PortInit(Port_t *port, uint8_t i2cAddr);

/**
 * @brief Set the next Type-C state.
 *
 * Also clears substate and logs the new state value.
 *
 * @param port Pointer to the port structure
 * @param state Next Type-C state
 * @return None
 */
void SetTypeCState(Port_t *port, ConnectionState state);

/**
 * @brief Set the next Policy Engine state.
 *
 * Also clears substate and logs the new state value.
 *
 * @param port Pointer to the port structure
 * @param state Next Type-C state
 * @return None
 */
void SetPEState(Port_t *port, PolicyState_t state);

/**
 * @brief Revert the ports current setting to the configured value.
 *
 * @param port Pointer to the port structure
 * @return None
 */
void SetConfiguredCurrent(Port_t *port);

#endif /* _PORT_H_ */
