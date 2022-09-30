/*******************************************************************************
 * @file     PD_Types.h
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
#ifndef __USBPD_TYPES_H__
#define __USBPD_TYPES_H__

#include "platform.h"

#ifdef CONFIG_CONFIG_FSC_DEBUG
#define PDBUFSIZE               128
#endif /* CONFIG_CONFIG_FSC_DEBUG */

#define STAT_BUSY               0
#define STAT_SUCCESS            1
#define STAT_ERROR              2
#define STAT_ABORT              3

/* USB PD Header Message Definitions */
#define PDPortRoleSink          0
#define PDPortRoleSource        1
#define PDDataRoleUFP           0
#define PDDataRoleDFP           1
#define PDCablePlugSource       0
#define PDCablePlugPlug         1

/* USB PD Control Message Types */
#define CMTGoodCRC              0x1
#define CMTGotoMin              0x2
#define CMTAccept               0x3
#define CMTReject               0x4
#define CMTPing                 0x5
#define CMTPS_RDY               0x6
#define CMTGetSourceCap         0x7
#define CMTGetSinkCap           0x8
#define CMTDR_Swap              0x9
#define CMTPR_Swap              0xa
#define CMTVCONN_Swap           0xb
#define CMTWait                 0xc
#define CMTSoftReset            0xd
#define CMTNotSupported         0x10
#define CMTGetSourceCapExt      0x11
#define CMTGetStatus            0x12
#define CMTGetFRSwap            0x13
#define CMTGetPPSStatus         0x14
#define CMTGetCountryCodes      0x15

/* USB PD Data Message Types */
#define DMTSourceCapabilities   0x1
#define DMTRequest              0x2
#define DMTBIST                 0x3
#define DMTSinkCapabilities     0x4
#define DMTBatteryStatus        0x5
#define DMTAlert                0x6
#define DMTGetCountryInfo       0x7
#define DMTVenderDefined        0xf

/* Extended message types opcode */
#ifdef CONFIG_FSC_HAVE_EXT_MSG
#define EXTSourceCapExt         0x1
#define EXTStatus               0x2
#define EXTGetBatteryCap        0x3
#define EXTGetBatteryStatus     0x4
#define EXTBatteryCapabilities  0x5
#define EXTGetManufacturerInfo  0x6
#define EXTManufacturerInfo     0x7
#define EXTSecurityRequest      0x8
#define EXTSecurityResponse     0x9
#define EXTFirmUpdateRequest    0xa
#define EXTFirmUpdateResponse   0xb
#define EXTPPSStatus            0xc
#define EXTCountryInfo          0xd
#define EXTCountryCodes         0xe

/** Extended message data size in bytes
 *  This must be set in the extended header
 */
#define EXT_STATUS_MSG_BYTES        5
#define EXT_MAX_MSG_LEN             260
#endif /* CONFIG_FSC_HAVE_EXT_MSG */
#define EXT_MAX_MSG_LEGACY_LEN      26

/* BIST Data Objects */
#define BDO_BIST_Receiver_Mode      0x0  /* Not Implemented */
#define BDO_BIST_Transmit_Mode      0x1  /* Not Implemented */
#define BDO_Returned_BIST_Counters  0x2  /* Not Implemented */
#define BDO_BIST_Carrier_Mode_0     0x3  /* Not Implemented */
#define BDO_BIST_Carrier_Mode_1     0x4  /* Not Implemented */
#define BDO_BIST_Carrier_Mode_2     0x5  /* Implemented */
#define BDO_BIST_Carrier_Mode_3     0x6  /* Not Implemented */
#define BDO_BIST_Eye_Pattern        0x7  /* Not Implemented */
#define BDO_BIST_Test_Data          0x8  /* Implemented */

/* USB PD Timing Parameters in milliseconds * scale factor */
/* Some values are changed from spec to allow for processor response time. */
#define tRetry                  3        
#define tNoResponse             5000     
#define tSenderResponse         24      
#define tTypeCSendSourceCap     150     
#define tSinkWaitCap            500      
#define tTypeCSinkWaitCap       350      
#define tSrcTransition          25       
#define tPSHardReset            30       
#define tPSHardResetMax         34       
#define tPSTransition           500      
#define tPSSourceOff            835      
#define tPSSourceOn             435      
#define tVCONNSourceOn          90       
#define tVCONNTransition        20       
#define tBISTContMode           50       
#define tSwapSourceStart        25       
#define tSrcRecover             675      
#define tSrcRecoverMax          1000     
#define tGoodCRCDelay           5        
#define t5To12VTransition       8        
#define tSafe0V                 650      
#define tSrcTurnOn              275      
#define tVBusSwitchDelay        5        
#define tVBusPollShort          10       
#define tVBusPollLong           100      
#define tSourceRiseTimeout      350      
#define tHardResetOverhead      0        
#define tPPSTimeout             14000    
#define tPPSRequest             10000    
#define tWaitCableReset         1        
#define tChunkReceiverRequest   15       
#define tChunkReceiverResponse  15       
#define tChunkSenderRequest     30       
#define tChunkSenderResponse    30       
#define tChunkingNotSupported   40       

#define nHardResetCount         2
#define nRetryCount             3
#define nCapsCount              50
#define nDiscoverIdentityCount  18  /* Common multiple of 2 and 3 retries */
/**
 * @brief Message header struct
 */
typedef union
{
    uint16_t word;
    uint8_t byte[2];

    struct
    {
        uint16_t MessageType :5;         /* 0-4      : Message Type */
        uint16_t PortDataRole :1;        /* 5        : Port Data Role */
        uint16_t SpecRevision :2;        /* 6-7      : Specification Revision */
        uint16_t PortPowerRole :1;       /* 8        : Port Power Role */
        uint16_t MessageID :3;           /* 9-11     : Message ID */
        uint16_t NumDataObjects :3;      /* 12-14    : Number of Data Objects */
        uint16_t Extended :1;            /* 15       : Extended message */
    };
} sopMainHeader_t;

/**
 * @brief Cable message header struct
 */
typedef union
{
    uint16_t word;
    uint8_t byte[2];
    struct
    {
        uint16_t MessageType :5;         /* 0-4      : Message Type */
        uint16_t :1;                     /* 5        : */
        uint16_t SpecRevision :2;        /* 6-7      : Specification Revision */
        uint16_t CablePlug :1;           /* 8        : Cable Plug */
        uint16_t MessageID :3;           /* 9-11     : Message ID */
        uint16_t NumDataObjects :3;      /* 12-14    : Number of Data Objects */
        uint16_t Extended :1;            /* 15       : Extended message */
    };
} sopPlugHeader_t;

/**
 * @brief Extended message header struct
 */
typedef union
{
    uint16_t word;
    uint8_t byte[2];
    struct
    {
        uint16_t DataSize :9;            /* Bit[0-8] Length of data in bytes */
        uint16_t :1;                     /* Bit[9] */
        uint16_t ReqChunk :1;            /* Bit[10] Request chunk boolean */
        uint16_t ChunkNum :4;            /* Bit[11-14] Chunk Number */
        uint16_t Chunked :1;             /* Bit[15] Chunked flag */
    };
} ExtHeader_t;

/**
 * @brief PDO structs for supplies, sinks, requests, etc.
 */
typedef union
{
    uint32_t object;
    uint16_t word[2];
    uint8_t byte[4];
    struct
    {
        uint32_t :30;
        uint32_t SupplyType :2;
    } PDO;                              /* General purpose PDO */
    struct
    {
        uint32_t MaxCurrent :10;         /* Max current in 10mA units */
        uint32_t Voltage :10;            /* Voltage in 50mV units */
        uint32_t PeakCurrent :2;         /* Peak current (from Ioc ratings) */
        uint32_t :3;
        uint32_t DataRoleSwap :1;        /* Supports DR_Swap message */
        uint32_t USBCommCapable :1;      /* USB communications capable */
        uint32_t ExternallyPowered :1;   /* Externally powered  */
        uint32_t USBSuspendSupport :1;   /* USB Suspend Supported */
        uint32_t DualRolePower :1;       /* Dual-Role power */
        uint32_t SupplyType :2;          /* Supply Type - Fixed: 0  */
    } FPDOSupply;                       /* Fixed PDO for Supplies */
    struct
    {
        uint32_t OperationalCurrent :10; /* Operational current in 10mA units */
        uint32_t Voltage :10;            /* Voltage in 50mV units */
        uint32_t :5;
        uint32_t DataRoleSwap :1;        /* Supports DR_Swap message */
        uint32_t USBCommCapable :1;      /* USB communications capable */
        uint32_t ExternallyPowered :1;   /* Externally powered  */
        uint32_t HigherCapability :1;    /* Sink needs more than vSafe5V */
        uint32_t DualRolePower :1;       /* Dual-Role power */
        uint32_t SupplyType :2;          /* Supply Type - Fixed: 0  */
    } FPDOSink;                         /* Fixed Power Data Object for Sinks */
    struct
    {
        uint32_t MaxCurrent :10;         /* Max current in 10mA units */
        uint32_t MinVoltage :10;         /* Minimum Voltage in 50mV units */
        uint32_t MaxVoltage :10;         /* Maximum Voltage in 50mV units */
        uint32_t SupplyType :2;          /* Supply Type - Variable: 1 */
    } VPDO;                             /* Variable Power Data Object */
    struct
    {
        uint32_t MaxPower :10;           /* Max power in 250mW units */
        uint32_t MinVoltage :10;         /* Minimum Voltage in 50mV units */
        uint32_t MaxVoltage :10;         /* Maximum Voltage in 50mV units */
        uint32_t SupplyType :2;          /* Supply Type - Battery: 2 */
    } BPDO;                             /* Battery Power Data Object */
    struct
    {
        uint32_t MaxCurrent :7;          /* Max current in 50mA units */
        uint32_t :1;
        uint32_t MinVoltage :8;          /* Min voltage in 100mV units */
        uint32_t :1;
        uint32_t MaxVoltage :8;          /* Max voltage in 100mV units */
        uint32_t :5;
        uint32_t SupplyType :2;          /* Apdo type 11b */
    } PPSAPDO;
    struct
    {
        uint32_t MinMaxCurrent :10;      /* Min/Max current in 10mA units */
        uint32_t OpCurrent :10;          /* Operating current in 10mA units */
        uint32_t :3;
        uint32_t UnChnkExtMsgSupport :1; /* Unchunked extended msg supported */
        uint32_t NoUSBSuspend :1;        /* USB suspend not available */
        uint32_t USBCommCapable :1;      /* USB communications capable */
        uint32_t CapabilityMismatch :1;  /* Sink cannot make valid request */
        uint32_t GiveBack :1;            /* Sink will respond to the GotoMin */
        uint32_t ObjectPosition :3;      /* Object being requested */
        uint32_t :1;
    } FVRDO;                            /* Fixed/Variable Request Data Object*/
    struct
    {
        uint32_t MinMaxPower :10;        /* Min/Max power in 250mW units */
        uint32_t OpPower :10;            /* Operating power in 250mW units */
        uint32_t :3;
        uint32_t UnChnkExtMsgSupport :1; /* Unchunked extended msg supported */
        uint32_t NoUSBSuspend :1;        /* USB suspend not available */
        uint32_t USBCommCapable :1;      /* USB communications capable */
        uint32_t CapabilityMismatch :1;  /* Sink cannot make valid request */
        uint32_t GiveBack :1;            /* Sink will respond to the GotoMin */
        uint32_t ObjectPosition :3;      /* Object being requested */
        uint32_t :1;
    } BRDO;                             /* Battery Request Data Object */
    struct
    {
        uint32_t OpCurrent :7;           /* Operating current in 50mA units */
        uint32_t :2;
        uint32_t Voltage :11;            /* Requested voltage in 20mV units */
        uint32_t :3;
        uint32_t UnChnkExtMsgSupport :1; /* Unchunked extended msg supported */
        uint32_t NoUSBSuspend :1;        /* USB suspend not available */
        uint32_t USBCommCapable :1;      /* USB communications capable */
        uint32_t CapabilityMismatch :1;  /* Sink cannot make valid request */
        uint32_t :1;
        uint32_t ObjectPosition :3;      /* PDO object index */
        uint32_t :1;
    } PPSRDO;                           /* PPS Request Data Object */
    struct {
        uint32_t OutputVoltage: 16;      /* Output voltage in 20mV, or 0xFFFF */
        uint32_t OutputCurrent: 8;       /* Output current in 50mA, or 0xFF */
        uint32_t :1;
        uint32_t Ptf:2;                  /* Temperature flag */
        uint32_t Omf:1;                  /* Current foldback mode */
        uint32_t :4;
    } PPSSDB;                           /* PPS Data block */
    struct {
        uint32_t :16;
        uint32_t HotSwapBat0:1;
        uint32_t HotSwapBat1:1;
        uint32_t HotSwapBat2:1;
        uint32_t HotSwpaBat3:1;
        uint32_t FixedBat4:1;
        uint32_t FixedBat5:1;
        uint32_t FixedBat6:1;
        uint32_t FixedBat7:1;
        uint32_t :1;
        uint32_t Battery:1;              /* Battery status changed */
        uint32_t OCP:1;                  /* Over current */
        uint32_t OTP:1;                  /* Over temperature */
        uint32_t OpCondition:1;          /* Operating condition */
        uint32_t Input:1;                /* Input change event */
        uint32_t OVP:1;                  /* Over Voltage */
        uint32_t :1;
    } ADO;                              /* Alert Data Object */
    struct
    {
        uint32_t VendorDefined :15;      /* Defined by the vendor */
        uint32_t VDMType :1;             /* Unstructured or structured header */
        uint32_t VendorID :16;           /* Unique VID value */
    } UVDM;
    struct
    {
        uint32_t Command :5;             /* VDM Command */
        uint32_t :1;
        uint32_t CommandType :2;         /* Init, ACK, NAK, BUSY... */
        uint32_t ObjPos :3;              /* Object position */
        uint32_t :2;
        uint32_t Version :2;             /* Structured VDM version */
        uint32_t VDMType :1;             /* Unstructured or structured header */
        uint32_t SVID :16;               /* Unique SVID value */
    } SVDM;
} doDataObject_t;

/**
 * @brief Generic PDO struct
 */
typedef struct
{
    uint32_t Voltage :10;
    uint32_t Current :10;
    uint32_t MinVoltage :10;
    uint32_t MaxVoltage :10;
    uint32_t MaxPower :10;
    uint32_t SupplyType :2;
} doPDO_t;

/**
 * @brief Supply status struct
 */
typedef union
{
    uint8_t byte[5];
    struct
    {
        uint8_t InternalTemp :8;         /* Bit[0-7] Temperature (Deg C) */
        uint8_t :1;                      /* Bit[0] */
        uint8_t ExternalPower :1;        /* Bit[1] Externally powered */
        uint8_t ACDC :1;                 /* Bit[2] Ext. Powered 0:DC 1:AC */
        uint8_t InternalBattery :1;      /* Bit[3] Int. Powered - Battery */
        uint8_t InternalNonBattery :1;   /* Bit[4] Int. Powered - Non-battery */
        uint8_t :3;                      /* Bit[5-7] */
        uint8_t PresentBatteryInput :8;  /* Bit[0-7] */
        uint8_t :1;                      /* Bit[0] */
        uint8_t EventOCP :1;             /* Bit[1] Chunk Number */
        uint8_t EventOTP :1;             /* Bit[2] Chunk Number */
        uint8_t EventOVP :1;             /* Bit[3] Chunk Number */
        uint8_t EventCVCF :1;            /* Bit[4] Chunk Number */
        uint8_t :3;                      /* Bit[5-7] */
        uint8_t :1;                      /* Bit[0] */
        uint8_t TemperatureStatus :2;    /* Bit[1-2] Temp Code */
        uint8_t :5;                      /* Bit[3-7] */
    };
} Status_t;

/**
 * @brief PPS status struct
 */
typedef union
{
    uint32_t object;
    uint16_t word[2];
    uint8_t byte[4];
    struct
    {
        uint32_t OutputVoltage :16;      /* Bit[0-15] Output Voltage (20mV) */
        uint32_t OutputCurrent :8;       /* Bit[0-7] Output Current (50mA) */
        uint32_t :4;                     /* Bit[4-7] */
        uint32_t FlagOMF :1;             /* Bit[3] Operating Mode Flag */
        uint32_t FlagPTF :2;             /* Bit[1-2] Present Temp Flag */
        uint32_t :1;                     /* Bit[0] */
    };
} PPSStatus_t;

/**
 * @brief Country info response struct
 */
typedef union
{
    uint8_t bytes[260];
    struct
    {
        uint8_t CountryCode[2];
        uint8_t Reserved[2];
        uint8_t Data[256];
    };
} CountryInfoResp;

/**
 * @brief Country code request struct
 */
typedef union
{
    uint8_t bytes[4];
    struct
    {
        uint8_t Reserved[2];
        uint8_t CountryCode[2];
    };
} CountryInfoReq;

/**
 * @brief Non-message entries for the PD message log
 */
typedef enum
{
    pdtNone = 0,                /* Reserved token (nothing) */
    pdtAttach,                  /* USB PD attached */
    pdtDetach,                  /* USB PD detached */
    pdtHardReset,               /* USB PD hard reset */
    pdtBadMessageID,            /* An incorrect message ID was received */
    pdtCableReset,              /* Cable reset */
    pdtHardResetTxd,            /* USB PD hard reset transmitted */
    pdtHardResetRxd,            /* USB PD hard reset received */
} USBPD_BufferTokens_t;

/**
 * @brief Policy Engine state machine enum
 */
typedef enum
{
    /* 0 */
    peDisabled = 0,             /* Policy engine is disabled */
    FIRST_PE_ST = peDisabled,   /* mark lowest value in enum */
    peErrorRecovery,            /* Error recovery state */
    peSourceHardReset,          /* Received a hard reset */
    peSourceSendHardReset,      /* Source send a hard reset */
    peSourceSoftReset,          /* Received a soft reset */
    peSourceSendSoftReset,      /* Send a soft reset */
    peSourceStartup,            /* Initial state */
    peSourceSendCaps,           /* Send the source capabilities */
    peSourceDiscovery,          /* Waiting to detect a USB PD sink */
    peSourceDisabled,           /* Disabled state */
    /* 10 */
    peSourceTransitionDefault,  /* Transition to default 5V state */
    peSourceNegotiateCap,       /* Negotiate capability and PD contract */
    peSourceCapabilityResponse, /* Respond to a request with a reject/wait */
    peSourceWaitNewCapabilities, /* Wait for new Source Caps from DPM */
    peSourceTransitionSupply,   /* Transition the power supply */
    peSourceReady,              /* Contract is in place and voltage is stable */
    peSourceGiveSourceCaps,     /* State to resend source capabilities */
    peSourceGetSinkCaps,        /* State to request the sink capabilities */
    peSourceSendPing,           /* State to send a ping message */
    peSourceGotoMin,            /* State to send gotoMin and prep the supply */
    /* 20 */
    peSourceGiveSinkCaps,       /* State to send sink caps if dual-role */
    peSourceGetSourceCaps,      /* State to request source caps from the UFP */
    peSourceSendDRSwap,         /* State to send a DR_Swap message */
    peSourceEvaluateDRSwap,     /* Evaluate DR swap request */
    peSourceAlertReceived,      /* Source has received an alert */

    peSinkHardReset,            /* Received a hard reset */
    peSinkSendHardReset,        /* Sink send hard reset */
    peSinkSoftReset,            /* Sink soft reset */
    peSinkSendSoftReset,        /* Sink send soft reset */
    peSinkTransitionDefault,    /* Transition to the default state */
    /* 30 */
    peSinkStartup,              /* Initial sink state */
    peSinkDiscovery,            /* Sink discovery state */
    peSinkWaitCaps,             /* Sink wait for capabilities state */
    peSinkEvaluateCaps,         /* Sink evaluate the rx'd source caps */
    peSinkSelectCapability,     /* Sink selecting a capability */
    peSinkTransitionSink,       /* Sink transitioning the current power */
    peSinkReady,                /* Sink ready state */
    peSinkGiveSinkCap,          /* Sink send capabilities state */
    peSinkGetSourceCap,         /* Sink get source capabilities state */
    peSinkGetSinkCap,           /* Sink get the sink caps of the source */
    /* 40 */
    peSinkGiveSourceCap,        /* Sink send the source caps if dual-role */
    peSinkSendDRSwap,           /* State to send a DR_Swap message */
    peSinkAlertReceived,        /* Sink received alert message */
    peSinkEvaluateDRSwap,       /* Evaluate DR swap request */
    peSourceSendVCONNSwap,      /* Initiate a VCONN swap sequence */
    peSourceEvaluateVCONNSwap,  /* Evaluate VCONN swap request */
    peSinkSendVCONNSwap,        /* Initiate a VCONN swap sequence */
    peSinkEvaluateVCONNSwap,    /* Evaluate VCONN swap request */
    peSourceSendPRSwap,         /* Initiate a PR Swap sequence */
    peSourceEvaluatePRSwap,     /* Evaluate PR swap request */
    /* 50 */
    peSinkSendPRSwap,           /* Initiate a PR Swap sequence */
    peSinkEvaluatePRSwap,       /* Evaluate PR swap request */

    peGetCountryCodes,          /* Send Get country code message */
    peGiveCountryCodes,         /* Send country codes */
    peNotSupported,             /* Send a reject/NS to unknown msg */
    peGetPPSStatus,             /* Sink request PPS source status */
    peGivePPSStatus,            /* Source provide PPS status */
    peGiveCountryInfo,          /* Send country info */

    /* VDM states */
    peGiveVdm,
    /* ---------- UFP VDM State Diagram ---------- */
    peUfpVdmGetIdentity,        /* Requesting Identity information from DPM */
    FIRST_VDM_STATE = peUfpVdmGetIdentity,
    /* 60 */
    peUfpVdmSendIdentity,       /* Sending Discover Identity ACK */

    peUfpVdmGetSvids,           /* Requesting SVID info from DPM */
    peUfpVdmSendSvids,          /* Sending Discover SVIDs ACK */
    peUfpVdmGetModes,           /* Requesting Mode info from DPM */
    peUfpVdmSendModes,          /* Sending Discover Modes ACK */
    peUfpVdmEvaluateModeEntry,  /* Evaluate request to enter a mode */
    peUfpVdmModeEntryNak,       /* Sending Enter Mode NAK response */
    peUfpVdmModeEntryAck,       /* Sending Enter Mode ACK response */
    peUfpVdmModeExit,           /* Evalute request to exit mode */
    peUfpVdmModeExitNak,        /* Sending Exit Mode NAK reponse */
    peUfpVdmModeExitAck,        /* Sending Exit Mode ACK Response */

    /* ---------- UFP VDM Attention State Diagram ---------- */
    peUfpVdmAttentionRequest,   /* Sending Attention Command */
    /* ---------- DFP to UFP VDM Discover Identity State Diagram ---------- */
    peDfpUfpVdmIdentityRequest, /* Sending Identity Request */
    peDfpUfpVdmIdentityAcked,   /* Inform DPM of Identity */
    peDfpUfpVdmIdentityNaked,   /* Inform DPM of result */
    /* ---------- DFP to Cable Plug VDM Discover Identity State Diagram --- */
    peDfpCblVdmIdentityRequest, /* Sending Identity Request */
    peDfpCblVdmIdentityAcked,   /* Inform DPM */
    peDfpCblVdmIdentityNaked,   /* Inform DPM */
    /* ---------- DFP VDM Discover SVIDs State Diagram ---------- */
    peDfpVdmSvidsRequest,       /* Sending Discover SVIDs request */
    peDfpVdmSvidsAcked,         /* Inform DPM */
    peDfpVdmSvidsNaked,         /* Inform DPM */

    /* ---------- DFP VDM Discover Modes State Diagram ---------- */
    peDfpVdmModesRequest,       /* Sending Discover Modes request */
    peDfpVdmModesAcked,         /* Inform DPM */
    peDfpVdmModesNaked,         /* Inform DPM */
    /* ---------- DFP VDM Enter Mode State Diagram ---------- */
    peDfpVdmModeEntryRequest,   /* Sending Mode Entry request */
    peDfpVdmModeEntryAcked,     /* Inform DPM */
    peDfpVdmModeEntryNaked,     /* Inform DPM */
    /* ---------- DFP VDM Exit Mode State Diagram ---------- */
    peDfpVdmModeExitRequest,    /* Sending Exit Mode request */
    peDfpVdmExitModeAcked,      /* Inform DPM */
    /* if Exit Mode not Acked, go to Hard Reset state */
    /* ---------- Source Startup VDM Discover Identity State Diagram ------ */
    peSrcVdmIdentityRequest,    /* sending Discover Identity request */
    peSrcVdmIdentityAcked,      /* inform DPM */

    peSrcVdmIdentityNaked,      /* inform DPM */
    /* ---------- DFP VDM Attention State Diagram ---------- */
    peDfpVdmAttentionRequest,   /* Attention Request received */
    /* ---------- Cable Ready VDM State Diagram ---------- */
    peCblReady,                 /* Cable power up state? */
    /* ---------- Cable Discover Identity VDM State Diagram ---------- */
    peCblGetIdentity,           /* Discover Identity request received */
    peCblGetIdentityNak,        /* Respond with NAK */
    peCblSendIdentity,          /* Respond with Ack */
    /* ---------- Cable Discover SVIDs VDM State Diagram ---------- */
    peCblGetSvids,              /* Discover SVIDs request received */
    peCblGetSvidsNak,           /* Respond with NAK */
    peCblSendSvids,             /* Respond with ACK */
    /* ---------- Cable Discover Modes VDM State Diagram ---------- */
    peCblGetModes,              /* Discover Modes request received */

    peCblGetModesNak,           /* Respond with NAK */
    peCblSendModes,             /* Respond with ACK */
    /* ---------- Cable Enter Mode VDM State Diagram ---------- */
    peCblEvaluateModeEntry,     /* Enter Mode request received */
    peCblModeEntryAck,          /* Respond with NAK */
    peCblModeEntryNak,          /* Respond with ACK */
    /* ---------- Cable Exit Mode VDM State Diagram ---------- */
    peCblModeExit,              /* Exit Mode request received */
    peCblModeExitAck,           /* Respond with NAK */
    peCblModeExitNak,           /* Respond with ACK */
    /* ---------- DP States ---------- */
    peDpRequestStatus,          /* Requesting PP Status */
    peDpRequestStatusAck,
    peDpRequestStatusNak,
    peDpRequestConfig,         /* Request Port partner Config */
    peDpRequestConfigAck,
    peDpRequestConfigNak,
    LAST_VDM_STATE = peDpRequestConfigNak,
    /* ---------- BIST Receive Mode --------------------- */
    PE_BIST_Receive_Mode,       /* Bist Receive Mode */
    PE_BIST_Frame_Received,     /* Test Frame received by Protocol layer */
    /* ---------- BIST Carrier Mode and Eye Pattern ----- */
    PE_BIST_Carrier_Mode_2,     /* BIST Carrier Mode 2 */
    PE_BIST_Test_Data,          /* BIST Test Data State */
    LAST_PE_ST = PE_BIST_Test_Data, /* mark last value in enum */
    dbgGetRxPacket,             /* Debug point for Rx packet handling */
    dbgSendTxPacket,            /* Debug point for Tx packet handling */
    peSendCableReset,           /* State to send cable reset */
    peSendGenericCommand,       /* Send an arbitrary command from the GUI */
    peSendGenericData,          /* Send arbitrary data from the GUI */
} PolicyState_t;

/**
 * @brief Protocol state machine enum
 */
typedef enum
{
    PRLDisabled = 0,            /* Protocol state machine is disabled */
    PRLIdle,                    /* Ready to Rx or Tx */
    PRLReset,                   /* Rx'd a soft reset or exit from hard reset */
    PRLResetWait,               /* Waiting for the hard reset to complete */
    PRLRxWait,                  /* Actively receiving a message */
    PRLTxSendingMessage,        /* Pass msg to the device */
    PRLTxWaitForPHYResponse,    /* Wait for activity on CC or a timeout */
    PRLTxVerifyGoodCRC,         /* Verify the good CRC message */
    PRLManualRetries,           /* Handle retries manually */

    /* ------- BIST Receiver Test -------- */
    PRL_BIST_Rx_Reset_Counter,  /* Reset BISTErrorCounter and preload PRBS */
    PRL_BIST_Rx_Test_Frame,     /* Wait for test Frame form PHY */
    PRL_BIST_Rx_Error_Count,    /* Set and send BIST error count msg to PHY */
    PRL_BIST_Rx_Inform_Policy,  /* Inform PE error count has been sent */
} ProtocolState_t;

/**
 * @brief Protocol transmit state machine enum
 */
typedef enum
{
    txIdle = 0,
    txReset,
    txSend,
    txBusy,
    txWait,
    txSuccess,
    txError,
    txCollision,
    txAbort
} PDTxStatus_t;

/**
 * @brief Supply type for PDOs
 */
typedef enum
{
    pdoTypeFixed = 0,
    pdoTypeBattery,
    pdoTypeVariable,
    pdoTypeAugmented,
    pdoTypeReserved
} pdoSupplyType;

/**
 * @brief Auto VDM process status
 */
typedef enum
{
    AUTO_VDM_INIT,
    AUTO_VDM_DISCOVER_ID_PP,
    AUTO_VDM_DISCOVER_SVIDS_PP,
    AUTO_VDM_DISCOVER_MODES_PP,
    AUTO_VDM_ENTER_MODE_PP,
    AUTO_VDM_DP_GET_STATUS,
    AUTO_VDM_DP_SET_CONFIG,
    AUTO_VDM_DONE
} VdmDiscoveryState_t;

/**
 * @brief Supported SOP types
 */
typedef enum
{
    SOP_TYPE_SOP,
    SOP_TYPE_SOP1,
    SOP_TYPE_SOP2,
    SOP_TYPE_SOP1_DEBUG,
    SOP_TYPE_SOP2_DEBUG,
    SOP_TYPE_NUM,
    SOP_TYPE_ERROR = 0xFF
} SopType;

/**
 * @brief Supported PD Specification Revisions
 */
typedef enum
{
    USBPDSPECREV1p0 = 0x0,
    USBPDSPECREV2p0 = 0x1,
    USBPDSPECREV3p0 = 0x2,
    USBPDSPECREVMAX = 0x3
} SpecRev;

/**
 * @brief State of extended message Tx/Rx handling
 */
typedef enum
{
    NoXfer,
    TXing,
    RXing,
} ExtMsgState_t;

/*
 * @brief State for cable reset
 */
typedef enum
{
    CBL_RST_DISABLED,
    CBL_RST_START,
    CBL_RST_VCONN_SOURCE,
    CBL_RST_DR_DFP,
    CBL_RST_SEND,
} CableResetState_t;

#endif /* __USBPD_TYPES_H__ */

