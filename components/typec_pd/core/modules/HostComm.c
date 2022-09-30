/*******************************************************************************
 * @file     HostComm.c
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
#include "HostComm.h"
#include "PDPolicy.h"
#include "core.h"
#include "hcmd.h"
#include "TypeC.h"
#include "PDProtocol.h"
#include "vendor_info.h"

#ifdef CONFIG_CONFIG_FSC_DEBUG
/*******************************************************************************
 * Macros for HostCom
 ******************************************************************************/
#define PTR_DIFF(ptr1, ptr2)                (((uint8_t*)ptr1) - ((uint8_t*)ptr2))
#define SET_DEFAULT_VAL(expr, val, def)     (expr == val) ? (def) : (expr)

/* Sets the value to 1 if expr is zero */
#define ONE_IF_ZERO(expr)                   SET_DEFAULT_VAL(expr, 0, 1)

/* Wraps index if it goes outside limit */
#define SAFE_INDEX(val, limit)              (val % limit)

#define WRITE_INT(start, val)\
    ({\
        (start)[0] = val & 0xFF;\
        (start)[1] = (val & 0xFF00) >> 8;\
        (start)[2] = (val & 0xFF0000) >> 16;\
        (start)[3] = (val & 0xFF000000) >> 24;\
        sizeof(uint32_t);\
    })

#define WRITE_SHORT(start, val)\
    ({\
        (start)[0] = val & 0xFF;\
        (start)[1] = (val & 0xFF00) >> 8;\
        sizeof(uint16_t);\
    })

#define READ_WORD(start)\
    ({\
        uint32_t x;\
        x = start[0] | start[1] << 8 | start[2] << 16 | start[3] << 24;\
        x;\
     })

#define READ_SHORT(start)\
    ({\
        uint16_t x;\
        x = start[0] | start[1] << 8;\
        x;\
     })

#define HCOM_MEM_FILL(ptr, val, len)\
    ({\
       uint16_t i = 0; \
       uint8_t *p = (uint8_t *)ptr;\
       while (i++ < len) { *p++ = 0; }\
    })

/******************************************************************************
 * Definition of HostCom_t structure
 ******************************************************************************/
typedef struct host_com
{
    Port_t *port_list;
    uint8_t numPorts;
    uint8_t *inputBuf;
    uint8_t *outputBuf;
} HostCom_t;

/******************************************************************************
 * Some structure for easier reading of pd request and response
 ******************************************************************************/
struct pd_buf
{
    uint8_t id;
    uint8_t val;
};

/******************************************************************************
 * HostCom variables
 ******************************************************************************/
static uint8_t USBInputBuf[HCMD_SIZE];
static uint8_t USBOutputBuf[HCMD_SIZE];
static HostCom_t hostCom;

#ifdef CONFIG_FSC_HAVE_SNK
static uint32_t ReadSinkCapabilities(uint8_t *data, int32_t bufLen,
                                    struct Port *port)
{
    uint32_t i = 0, j = 0;
    uint32_t index = 0;
    int32_t len = 0;
    sopMainHeader_t *cap_header;
    doDataObject_t *caps;

    cap_header = DPM_GetSinkCapHeader(port->dpm, port);
    caps = DPM_GetSinkCap(port->dpm, port);
    len = cap_header->NumDataObjects * 4;

    if (bufLen < len) { return 0; }

    data[index++] = cap_header->byte[0];
    data[index++] = cap_header->byte[1];

    for (i = 0; i < cap_header->NumDataObjects; i++)
    {
        for (j = 0; j < 4; j++)
        {
            data[index++] = caps[i].byte[j];
        }
    }

    return index;
}
#endif /* CONFIG_FSC_HAVE_SNK */

#ifdef CONFIG_FSC_HAVE_SRC
static uint8_t ReadSourceCapabilities(uint8_t *data, int32_t bufLen,
                                     struct Port *port)
{
    uint32_t i = 0, j = 0;
    uint32_t index = 0;
    int32_t len = 0;
    sopMainHeader_t *header;
    doDataObject_t *caps;

    header = DPM_GetSourceCapHeader(port->dpm, port);
    caps = DPM_GetSourceCap(port->dpm, port);
    len = header->NumDataObjects * 4;

    if (bufLen < len) { return 0; }

    data[index++] = header->byte[0];
    data[index++] = header->byte[1];

    for (i = 0; i < header->NumDataObjects; i++)
    {
        for (j = 0; j < 4; j++)
        {
            data[index++] = caps[i].byte[j];
        }
    }

    return index;
}
#endif /* CONFIG_FSC_HAVE_SRC */

static uint8_t ReadReceivedSrcCaps(uint8_t *data, uint32_t bufLen,
                                  struct Port *port)
{
    uint32_t i = 0, j = 0;
    uint32_t index = 0;
    uint32_t len = port->SrcCapsHeaderReceived.NumDataObjects * 4;

    if (bufLen < len)
    {
        return 0;
    }

    data[index++] = port->SrcCapsHeaderReceived.byte[0];
    data[index++] = port->SrcCapsHeaderReceived.byte[1];

    for (i = 0; i < port->SrcCapsHeaderReceived.NumDataObjects; i++)
    {
        for (j = 0; j < 4; j++)
        {
            data[index++] = port->SrcCapsReceived[i].byte[j];
        }
    }

    return index;
}

static uint8_t ReadReceivedSnkCaps(uint8_t *data, uint32_t bufLen,
                                  struct Port *port)
{
    uint32_t i = 0, j = 0;
    uint32_t index = 0;
    uint32_t len = port->SnkCapsHeaderReceived.NumDataObjects * 4;

    if (bufLen < len)
    {
        return 0;
    }

    data[index++] = port->SnkCapsHeaderReceived.byte[0];
    data[index++] = port->SnkCapsHeaderReceived.byte[1];

    for (i = 0; i < port->SnkCapsHeaderReceived.NumDataObjects; i++)
    {
        for (j = 0; j < 4; j++)
        {
            data[index++] = port->SnkCapsReceived[i].byte[j];
        }
    }

    return index;
}

#if (defined(CONFIG_FSC_HAVE_SRC) || \
     (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)))
static void WriteSourceCapabilities(uint8_t *data, struct Port *port,
                                    uint32_t len)
{
    uint32_t i = 0, j = 0;
    sopMainHeader_t header = { 0 };
    sopMainHeader_t *cap_header = DPM_GetSourceCapHeader(port->dpm, port);
    doDataObject_t *caps = DPM_GetSourceCap(port->dpm, port);

    header.byte[0] = *data++;
    header.byte[1] = *data++;

    if (len < header.NumDataObjects*sizeof(uint32_t)) { return; }
    /* Only do anything if we decoded a source capabilities message */
    if ((header.NumDataObjects > 0)
            && (header.MessageType == DMTSourceCapabilities))
    {
        cap_header->word = header.word;

        for (i = 0; i < cap_header->NumDataObjects; i++)
        {
            for (j = 0; j < 4; j++)
            {
                caps[i].byte[j] = *data++;
            }
        }

        if (port->PolicyIsSource)
        {
            port->PDTransmitHeader.word = cap_header->word;
            port->USBPDTxFlag = true;
            port->SourceCapsUpdated = true;
        }
    }
}
#endif /* CONFIG_FSC_HAVE_SRC || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */

#ifdef CONFIG_FSC_HAVE_SNK
static void WriteSinkCapabilities(uint8_t *data, struct Port *port, uint32_t len)
{
    uint32_t i = 0, j = 0;
    sopMainHeader_t header = { 0 };
    sopMainHeader_t *caps_header = DPM_GetSinkCapHeader(port->dpm, port);
    doDataObject_t *caps = DPM_GetSinkCap(port->dpm, port);

    header.byte[0] = *data++;
    header.byte[1] = *data++;

    if (len < header.NumDataObjects*sizeof(uint32_t)) { return; }
    /* Only do anything if we decoded a sink capabilities message */
    if ((header.NumDataObjects > 0)
            && (header.MessageType == DMTSinkCapabilities))
    {
        caps_header->word = header.word;
        for (i = 0; i < caps_header->NumDataObjects; i++)
        {
            for (j = 0; j < 4; j++)
            {
                caps[i].byte[j] = *data++;
            }
        }

        /* We could also trigger sending the caps or re-evaluating,
         * but we don't do anything with this info here...
         */
    }
}
#endif

#ifdef CONFIG_FSC_HAVE_VDM
/* Svid 1 buffer layout */
struct svid1_buf
{
    uint8_t svid[2];
    uint8_t num_modes;
    uint8_t modes[4 * SVID1_num_modes_max_SOP];
};

struct vdm_mode_buf
{
    uint8_t num_svid;
    struct svid1_buf svid1;
/* Add more if needed */
};

uint8_t ReadVdmModes(struct Port *port, uint8_t *buf, uint32_t len)
{
    struct vdm_mode_buf *vdm_obj = (struct vdm_mode_buf*) buf;
    /* Check if buffer has enough size */
    if (len < (sizeof(struct vdm_mode_buf)))
    {
        return 0;
    }
    /* Assuming that number of svids and modes is 1. If not change
     * the following code to write all values in buffer and use
     * constants from vendor_info. Incorrect number might cause
     * GUI to crash as it tries to read all the svids.
     * Num_SVIDs_max_SOP = 1
     */
    vdm_obj->num_svid = 1;
    /* Iterate through each svid if needed. Assuming we only have one */
    WRITE_SHORT(vdm_obj->svid1.svid, port->my_svid);
    vdm_obj->svid1.num_modes = SVID1_num_modes_max_SOP;
    WRITE_INT(vdm_obj->svid1.modes, port->my_mode);
    return sizeof(struct vdm_mode_buf);
}

void WriteVdmModes(struct Port *port, uint8_t *buf)
{
    struct vdm_mode_buf *vdm_obj = (struct vdm_mode_buf*) buf;
    if (vdm_obj->num_svid > 0)
    {
        port->my_svid = READ_SHORT(vdm_obj->svid1.svid);
        port->my_mode = READ_WORD(vdm_obj->svid1.modes);
    }
}
#endif /* CONFIG_FSC_HAVE_VDM */

static void SendUSBPDMessage(uint8_t *data, struct Port *port, uint32_t len)
{
    uint32_t i = 0, j = 0;

    /* First byte is sop */
    port->PolicyMsgTxSop = *data++;

    /* 2 header bytes */
    port->PDTransmitHeader.byte[0] = *data++;
    port->PDTransmitHeader.byte[1] = *data++;

    if (len < port->PDTransmitHeader.NumDataObjects * sizeof(uint32_t))
    {
        return;
    }

    /* Data objects */
    for (i = 0; i < port->PDTransmitHeader.NumDataObjects; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            port->PDTransmitObjects[i].byte[j] = *data++;
        }
    }
    port->USBPDTxFlag = true;
}

static void ProcessTCSetState(ConnectionState state, struct Port *port)
{
    switch (state)
    {
    case (Disabled):
        SetStateDisabled(port);
        break;
    case (ErrorRecovery):
        SetStateErrorRecovery(port);
        break;
    case (Unattached):
        SetStateUnattached(port);
        break;
#ifdef CONFIG_FSC_HAVE_SNK
    case (AttachWaitSink):
        SetStateAttachWaitSink(port);
        break;
    case (AttachedSink):
        SetStateAttachedSink(port);
        break;
#ifdef CONFIG_FSC_HAVE_DRP
    case (TryWaitSink):
        SetStateTryWaitSink(port);
        break;
    case (TrySink):
        SetStateTrySink(port);
        break;
#endif /* CONFIG_FSC_HAVE_DRP */
#endif /* CONFIG_FSC_HAVE_SNK */
#ifdef CONFIG_FSC_HAVE_SRC
    case (AttachWaitSource):
        SetStateAttachWaitSource(port);
        break;
    case (AttachedSource):
        SetStateAttachedSource(port);
        break;
#ifdef CONFIG_FSC_HAVE_DRP
    case (TrySource):
        SetStateTrySource(port);
        break;
    case (TryWaitSource):
        SetStateTryWaitSource(port);
        break;
    case (UnattachedSource):
        SetStateUnattachedSource(port);
        break;
#endif /* CONFIG_FSC_HAVE_DRP */
#endif /* CONFIG_FSC_HAVE_SRC */
#ifdef CONFIG_FSC_HAVE_ACCMODE
    case (AudioAccessory):
        SetStateAudioAccessory(port);
        break;
#endif /* CONFIG_FSC_HAVE_ACCMODE */
#ifdef CONFIG_FSC_HAVE_SRC
    case (DebugAccessorySource):
        SetStateDebugAccessorySource(port);
        break;
#endif /* CONFIG_FSC_HAVE_SRC */
#if (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE))
    case (AttachWaitAccessory):
        SetStateAttachWaitAccessory(port);
        break;
    case (PoweredAccessory):
        SetStatePoweredAccessory(port);
        break;
    case (UnsupportedAccessory):
        SetStateUnsupportedAccessory(port);
        break;
#endif /* (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)) */
    default:
        SetStateUnattached(port);
        break;
    }
}

static bool ProcessTCWrite(uint8_t *subCmds, struct Port *port)
{
    bool setUnattached = false;
    uint32_t i;
    uint32_t len;
    struct pd_buf *pCmd = (struct pd_buf*) subCmds;

    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&subCmds[HCMD_PAYLOAD_SIZE], pCmd);
        if (len < sizeof(struct pd_buf))
        {
            goto end_of_cmd;
        }
        len -= sizeof(struct pd_buf);
        switch (pCmd->id)
        {
        case TYPEC_ENABLE:
            if (pCmd->val != port->SMEnabled)
                port->SMEnabled = (pCmd->val) ? true : false;
            break;
        case TYPEC_PORT_TYPE:
            if (pCmd->val < USBTypeC_UNDEFINED
                    && pCmd->val != port->PortConfig.PortType)
            {
                port->PortConfig.PortType = pCmd->val;
                setUnattached = true;
            }
            break;
        case TYPEC_ACC_SUPPORT:
            if (pCmd->val != port->PortConfig.poweredAccSupport ||
                pCmd->val != port->PortConfig.audioAccSupport)
            {
                port->PortConfig.poweredAccSupport = (pCmd->val) ? true : false;
                port->PortConfig.audioAccSupport =
                        port->PortConfig.poweredAccSupport;
                setUnattached = true;
            }
            break;
        case TYPEC_SRC_PREF:
            if (pCmd->val != port->PortConfig.SrcPreferred)
            {
                port->PortConfig.SrcPreferred = (pCmd->val) ? true : false;
                setUnattached = true;
            }
            break;
        case TYPEC_SNK_PREF:
            if (pCmd->val != port->PortConfig.SnkPreferred)
            {
                port->PortConfig.SnkPreferred = (pCmd->val) ? true : false;
                setUnattached = true;
            }
            break;
        case TYPEC_STATE:
            ProcessTCSetState(pCmd->val, port);
            break;
        case TYPEC_DFP_CURRENT_DEF:
            if (pCmd->val != port->PortConfig.RpVal && pCmd->val < utccInvalid)
            {
                port->PortConfig.RpVal = pCmd->val;

                if (port->ConnState == Unattached)
                {
                    SetConfiguredCurrent(port);
                }
            }
            break;
        case TYPEC_EOP:
        default:
            goto end_of_cmd;
            break;
        }

        pCmd++;
    }

end_of_cmd:
    /** set unattached if command requires for transition */
    if (setUnattached)
    {
        SetStateUnattached(port);
        setUnattached = false;
    }

  return true;
}

static bool ProcessTCRead(uint8_t *subCmds, uint8_t *outBuf,
                              struct Port *port)
{
    uint32_t len;
    uint32_t i;
    struct pd_buf *pRsp = (struct pd_buf*) outBuf;

    /* limit it to 30 commands because the output requires
     * twice the size for each command
     */
    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&outBuf[HCMD_PAYLOAD_SIZE], pRsp);
        if (len < sizeof(struct pd_buf))
        {
            /* at least 2 bytes needed */
            goto out_buf_full;
        }
        pRsp->id = subCmds[i];

        /* Bytes that additional info can use */
        len -= sizeof(struct pd_buf);

        switch (pRsp->id)
        {
        case TYPEC_ENABLE:
            pRsp->val = port->SMEnabled;
            break;
        case TYPEC_PORT_TYPE:
            pRsp->val = port->PortConfig.PortType;
            break;
        case TYPEC_ACC_SUPPORT:
            pRsp->val = (port->PortConfig.poweredAccSupport &&
                         port->PortConfig.audioAccSupport) ? true : false;
            break;
        case TYPEC_SRC_PREF:
            pRsp->val = port->PortConfig.SrcPreferred;
            break;
        case TYPEC_SNK_PREF:
            pRsp->val = port->PortConfig.SnkPreferred;
            break;
        case TYPEC_STATE:
            pRsp->val = port->ConnState;
            break;
        case TYPEC_SUBSTATE:
            pRsp->val = port->TypeCSubState;
            break;
        case TYPEC_CC_ORIENT:
        {
            pRsp->val = port->CCPin;
            break;
        }
        case TYPEC_CC_TERM:
            pRsp->val = port->CCTerm;
            break;
        case TYPEC_VCON_TERM:
            pRsp->val = port->VCONNTerm;
            break;
        case TYPEC_DFP_CURRENT_AD:
            pRsp->val = port->SourceCurrent;
            break;
        case TYPEC_DFP_CURRENT_DEF:
            pRsp->val = port->PortConfig.RpVal;
            break;
        case TYPEC_UFP_CURRENT:
            pRsp->val = port->SinkCurrent;
            break;
        case TYPEC_STATE_LOG:
        {
            #ifdef CONFIG_CONFIG_FSC_DEBUG
            pRsp->val = GetStateLog(&port->TypeCStateLog, &pRsp->val + 1, len);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            #endif
            break;
        }
        case TYPEC_EOP:
        default:
            pRsp->id = TYPEC_EOP;
            goto end_of_cmd;
            break;
        }
        pRsp++;
    }

end_of_cmd:
    return true;
out_buf_full:
    return false;
}

static void ProcessTCStatus(HostCmd_t *inCmd, HostCmd_t *outMsg,
                            struct Port *port)
{
    bool read = (inCmd->pd.cmd.req.rw == 0) ? true : false;
    bool status;

    if (read == true)
    {
        status = ProcessTCRead(inCmd->typeC.cmd.req.payload,
                               outMsg->typeC.cmd.rsp.payload, port);
    }
    else
    {
        status = ProcessTCWrite(inCmd->typeC.cmd.req.payload, port);
    }

    if (status == true)
    {
        outMsg->typeC.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    }
    else
    {
        outMsg->typeC.cmd.rsp.error = HCMD_STATUS_FAILED;
    }
}

static bool ProcessPDRead(uint8_t *subCmds, uint8_t *outBuf,
                              struct Port *port)
{
    uint32_t len;
    uint32_t i;

    struct pd_buf *pRsp = (struct pd_buf*) outBuf;

    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&outBuf[HCMD_PAYLOAD_SIZE], pRsp);
        if (len < sizeof(struct pd_buf))
        {
            /* Buffer is full */
            if (subCmds[i] == USBPD_EOP)
            {
                /* Done with commands, so full buffer is OK */
                goto end_of_cmd;
            }
            else
            {
                /* At least 2 bytes needed to continue */
                goto out_buf_full;
            }
        }

        pRsp->id = subCmds[i];
        len -= sizeof(struct pd_buf);
        switch (pRsp->id)
        {
        case PD_ENABLE:
            pRsp->val = port->USBPDEnabled;
            break;
        case PD_ACTIVE:
            pRsp->val = port->USBPDActive;
            break;
        case PD_HAS_CONTRACT:
            pRsp->val = port->PolicyHasContract;
            break;
        case PD_SPEC_REV:
            pRsp->val = port->PortConfig.PdRevPreferred;
            break;
        case PD_PWR_ROLE:
            pRsp->val = port->PolicyIsSource;
            break;
        case PD_DATA_ROLE:
            pRsp->val = port->PolicyIsDFP;
            break;
        case PD_VCON_SRC:
            pRsp->val = port->IsVCONNSource;
            break;
        case PD_STATE:
            pRsp->val = port->PolicyState;
            break;
        case PD_SUB_STATE:
            pRsp->val = port->PolicySubIndex;
            break;
        case PD_PROTOCOL_STATE:
            pRsp->val = port->ProtocolState;
            break;
        case PD_PROTOCOL_SUB_STATE:
            pRsp->val = 0;
            break;
        case PD_TX_STATUS:
            pRsp->val = port->PDTxStatus;
            break;
        case PD_SNK_TX_STATUS:
            pRsp->val = 0;
            break;
        case PD_CAPS_CHANGED:
            pRsp->val = port->SourceCapsUpdated;
            port->SourceCapsUpdated = false;
            break;
        case PD_GOTOMIN_COMPAT:
            pRsp->val =port->PortConfig.SinkGotoMinCompatible;
            break;
        case PD_USB_SUSPEND:
            pRsp->val = port->PortConfig.SinkUSBSuspendOperation;
            break;
        case PD_COM_CAPABLE:
            pRsp->val = port->PortConfig.SinkUSBCommCapable;
            break;
#ifdef CONFIG_FSC_HAVE_SRC
        case PD_SRC_CAP:
            pRsp->val = ReadSourceCapabilities(&pRsp->val + 1, len, port);
            pRsp = (struct pd_buf*) ((uint8_t*) pRsp + pRsp->val);
            break;
        case PD_PPR_SINK_CAP:
            pRsp->val = ReadReceivedSnkCaps(&pRsp->val + 1, len, port);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
#endif /* CONFIG_FSC_HAVE_SRC */
#ifdef CONFIG_FSC_HAVE_SNK
        case PD_SNK_CAP:
            pRsp->val = ReadSinkCapabilities(&pRsp->val + 1, len, port);
            pRsp = (struct pd_buf*) ((uint8_t*) pRsp + pRsp->val);
            break;
        case PD_PPR_SRC_CAP:
            pRsp->val = ReadReceivedSrcCaps(&pRsp->val + 1, len, port);
            pRsp = (struct pd_buf*) ((uint8_t*) pRsp + pRsp->val);
            break;
#endif /* CONFIG_FSC_HAVE_SNK */
#ifdef CONFIG_CONFIG_FSC_DEBUG
        case PD_PD_LOG:
            pRsp->val = ReadUSBPDBuffer(port, &pRsp->val + 1, len);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
        case PD_PE_LOG:
            pRsp->val = GetStateLog(&port->PDStateLog, &pRsp->val + 1, len);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
#endif  /* FSC_LOGGING */
        case PD_MAX_VOLTAGE:
            if (len < sizeof(int)) break;
            pRsp->val = WRITE_INT((&pRsp->val + 1),
                    port->PortConfig.SinkRequestMaxVoltage);
            pRsp = (struct pd_buf*) ((uint8_t*) pRsp + pRsp->val);
            break;
        case PD_OP_POWER:
            if (len < sizeof(int)) break;
            /* GUI uses 10mW units */
            pRsp->val = WRITE_INT((&pRsp->val + 1),
                    port->PortConfig.SinkRequestOpPower / 10);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
        case PD_MAX_POWER:
            if (len < sizeof(int)) break;
            /* GUI uses 10mW units */
            pRsp->val = WRITE_INT((&pRsp->val + 1),
                    port->PortConfig.SinkRequestMaxPower / 10);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
#ifdef CONFIG_FSC_HAVE_VDM
        case PD_VDM_MODES:
        {
            pRsp->val = ReadVdmModes(port, &pRsp->val + 1, len);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
        }
        case PD_SVID_ENABLE:
            pRsp->val = port->svid_enable;
            break;
        case PD_MODE_ENABLE:
            pRsp->val = port->mode_enable;
            break;
        case PD_SVID_AUTO_ENTRY:
            pRsp->val = port->AutoModeEntryEnabled;
            break;
#endif /* CONFIG_FSC_HAVE_VDM */
        case USBPD_EOP:
        default:
            pRsp->id = USBPD_EOP;
            goto end_of_cmd;
            break;
        }
        pRsp++;
    }

out_buf_full:
    return false;
end_of_cmd:
    return true;
}

static bool ProcessPDWrite(uint8_t *subCmds, struct Port *port)
{

    bool setUnattached = false;
    struct pd_buf *pCmd = (struct pd_buf*) subCmds;
    uint32_t i;
    uint32_t len;

    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&subCmds[HCMD_PAYLOAD_SIZE], pCmd);
        if (len < sizeof(struct pd_buf))
        {
            goto end_of_cmd;
        }
        len -= sizeof(struct pd_buf);
        switch (pCmd->id)
        {
        case PD_ENABLE:
            port->USBPDEnabled = (pCmd->val == 0) ? false : true;
            break;
        case PD_SPEC_REV:
            if (port->PortConfig.PdRevPreferred != pCmd->val) {
                port->PortConfig.PdRevPreferred = pCmd->val;
                setUnattached = true;
            }
            break;
        case PD_GOTOMIN_COMPAT:
            port->PortConfig.SinkGotoMinCompatible =
                (pCmd->val == 0) ? false : true;
            break;
        case PD_USB_SUSPEND:
            port->PortConfig.SinkUSBSuspendOperation =
                (pCmd->val == 0) ? false : true;
            break;
        case PD_COM_CAPABLE:
            port->PortConfig.SinkUSBCommCapable =
                (pCmd->val == 0) ? false : true;
            break;
        case PD_MAX_VOLTAGE:
            if (len < sizeof(uint32_t)) { goto end_of_cmd; }
            port->PortConfig.SinkRequestMaxVoltage =
                READ_WORD((&pCmd->val + 1));
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
        case PD_OP_POWER:
            if (len < sizeof(uint32_t)) { goto end_of_cmd; }
            port->PortConfig.SinkRequestOpPower =
                    READ_WORD((&pCmd->val + 1)) * 10; /* GUI sends 10mW units */
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
        case PD_MAX_POWER:
            if (len < sizeof(uint32_t)) { goto end_of_cmd; }
            port->PortConfig.SinkRequestMaxPower =
                    READ_WORD((&pCmd->val + 1)) * 10; /* GUI sends 10mW units */
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
        case PD_HARD_RESET:
            SendUSBPDHardReset(port);
            break;
#if (defined(CONFIG_FSC_HAVE_SRC) || \
     (defined(CONFIG_FSC_HAVE_SNK) && defined(CONFIG_FSC_HAVE_ACCMODE)))
        case PD_SRC_CAP:
            if (len < sizeof(sopMainHeader_t)) { goto end_of_cmd; }
            WriteSourceCapabilities(&pCmd->val+1, port, len);
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
#endif /* CONFIG_FSC_HAVE_SRC || (CONFIG_FSC_HAVE_SNK && CONFIG_FSC_HAVE_ACCMODE) */
#ifdef CONFIG_FSC_HAVE_SNK
        case PD_SNK_CAP:
            if (len < sizeof(sopMainHeader_t)) { goto end_of_cmd; }
            WriteSinkCapabilities(&pCmd->val+1, port, len);
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
#endif /* CONFIG_FSC_HAVE SNK */
        case PD_MSG_WRITE:
            if (len < sizeof(sopMainHeader_t)) { goto end_of_cmd; }
            SendUSBPDMessage(&pCmd->val+1, port, len);
            pCmd = (struct pd_buf*)((uint8_t*)pCmd + pCmd->val);
            break;
#ifdef CONFIG_FSC_HAVE_VDM
        case PD_SVID_ENABLE:
            port->svid_enable = (pCmd->val == 0) ? false : true;
            break;
        case PD_MODE_ENABLE:
            port->mode_enable = (pCmd->val == 0) ? false : true;
            break;
        case PD_SVID_AUTO_ENTRY:
            port->AutoModeEntryEnabled = (pCmd->val == 0) ? false : true;
            break;
        case PD_VDM_MODES:
            if (len < sizeof(struct vdm_mode_buf)) { goto end_of_cmd; }
            WriteVdmModes(port, &pCmd->val + 1);
            pCmd = (struct pd_buf*) ((uint8_t*) pCmd + pCmd->val);
            break;
        case PD_CABLE_RESET:
            port->cblRstState = CBL_RST_START;
            break;
#endif  /* CONFIG_FSC_HAVE_VDM */
        case USBPD_EOP:
        default:
            goto end_of_cmd;
            break;
        }
        pCmd++;
    }

end_of_cmd:
    /** set unattached if command requires for transition */
    if (setUnattached)
    {
        SetStateUnattached(port);
        setUnattached = false;
    }
    return true;
}

static void ProcessPDStatus(HostCmd_t *inCmd, HostCmd_t *outMsg,
                            struct Port *port)
{
    bool read = (inCmd->pd.cmd.req.rw == 0) ? true : false;
    bool status = false;

    if (read == true)
    {
        status = ProcessPDRead(inCmd->pd.cmd.req.payload,
                               outMsg->pd.cmd.rsp.payload, port);
    }
    else
    {
        status = ProcessPDWrite(inCmd->pd.cmd.req.payload, port);
    }

    if (status == true)
    {
        outMsg->typeC.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    }
    else
    {
        outMsg->typeC.cmd.rsp.error = HCMD_STATUS_FAILED;
    }
}

static void ProcessUserClassCmd(HostCmd_t *inCmd, HostCmd_t *outMsg,
                                struct Port *port)
{
    outMsg->userClass.cmd.rsp.id = inCmd->userClass.cmd.req.id;
    switch (inCmd->userClass.cmd.req.id)
    {
    case 0:
        outMsg->userClass.cmd.rsp.payload[0] =
                platform_get_device_irq_state(port->PortID) ? 0x00 : 0xFF;
        outMsg->userClass.cmd.rsp.error = HCMD_STATUS_SUCCESS;
        break;
    case 1:
        port->I2cAddr = inCmd->userClass.cmd.req.payload[0];
        outMsg->userClass.cmd.rsp.error = HCMD_STATUS_SUCCESS;
        SetStateUnattached(port);
        break;
    default:
        outMsg->userClass.cmd.rsp.error = HCMD_STATUS_NOT_IMPLEMENTED;
        break;
    }
}

#ifdef CONFIG_FSC_HAVE_DP
static bool ProcessDPWrite(uint8_t *subCmds, struct Port *port)
{

    uint32_t i;
    uint32_t len;
    struct pd_buf *pCmd = (struct pd_buf*) subCmds;

    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&subCmds[HCMD_PAYLOAD_SIZE], pCmd);
        if (len < sizeof(struct pd_buf))
        {
            goto end_of_cmd;
        }
        len -= sizeof(struct pd_buf);
        switch (pCmd->id)
        {
        case DP_ENABLE:
            port->DisplayPortData.DpEnabled = (pCmd->val == 0) ? false : true;
            break;
        case DP_AUTO_MODE_ENTRY:
            port->DisplayPortData.DpAutoModeEntryEnabled =
                    (pCmd->val == 0) ? false : true;
            break;
        case DP_SEND_STATUS:
            if (port->PolicyIsDFP == true &&
                port->PolicyState == peSourceReady)
            {
                DP_RequestPartnerStatus(port);
            } else if (port->PolicyIsDFP == false &&
                       port->PolicyState == peSinkReady)
            {
                DP_SendAttention(port);
            }
            break;
        case DP_CAP:
            if (len < sizeof(uint32_t)) { goto end_of_cmd; }
            port->DisplayPortData.DpCap.word = READ_WORD((&pCmd->val + 1));
            pCmd = (struct pd_buf*) ((uint8_t*) pCmd + pCmd->val);
            break;
        case DP_STATUS:
            if (len < sizeof(uint32_t)) { goto end_of_cmd; }
            port->DisplayPortData.DpStatus.word = READ_WORD((&pCmd->val + 1));
            pCmd = (struct pd_buf*) ((uint8_t*) pCmd + pCmd->val);
            break;
        case DP_EOP:
        default:
            goto end_of_cmd;
            break;
        }
        pCmd++;
    }

end_of_cmd:
    return true;
}

static bool ProcessDPRead(uint8_t *subCmds, uint8_t *outBuf,
                              struct Port *port)
{
    uint32_t i;
    uint32_t len;
    struct pd_buf *pRsp = (struct pd_buf*) outBuf;

    for (i = 0; i < HCMD_PAYLOAD_SIZE / 2; i++)
    {
        len = PTR_DIFF(&outBuf[HCMD_PAYLOAD_SIZE], pRsp);
        if (len < sizeof(struct pd_buf))
        {
            /* at least 2 bytes needed */
            goto out_buf_full;

        }
        pRsp->id = subCmds[i];
        len -= sizeof(struct pd_buf);
        switch (pRsp->id)
        {
        case DP_ENABLE:
            pRsp->val = port->DisplayPortData.DpEnabled;
            break;
        case DP_AUTO_MODE_ENTRY:
            pRsp->val = port->DisplayPortData.DpAutoModeEntryEnabled;
            break;
        case DP_CAP:
            if (len < sizeof(uint32_t)) break;
            pRsp->val = WRITE_INT((&pRsp->val + 1),
                                   port->DisplayPortData.DpCap.word);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
        case DP_STATUS:
            if (len < sizeof(uint32_t)) break;
            pRsp->val = WRITE_INT((&pRsp->val + 1),
                                   port->DisplayPortData.DpStatus.word);
            pRsp = (struct pd_buf*)((uint8_t*)pRsp + pRsp->val);
            break;
        case DP_EOP:
        default:
            pRsp->id = DP_EOP;
            goto end_of_cmd;
            break;
        }
        pRsp++;
    }

out_buf_full:
    return false;
end_of_cmd:
    return true;
}

static void ProcessDpCommands(HostCmd_t *inCmd, HostCmd_t *outMsg,
                            struct Port *port)
{
    bool read = (inCmd->dp.cmd.req.rw == 0) ? true : false;
    bool status = false;
    if (read == true)
    {
        status = ProcessDPRead(inCmd->dp.cmd.req.payload,
                               outMsg->dp.cmd.rsp.payload, port);
    }
    else
    {
        status = ProcessDPWrite(inCmd->dp.cmd.req.payload, port);
    }

    if (status == true)
    {
        outMsg->dp.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    }
    else
    {
        outMsg->dp.cmd.rsp.error = HCMD_STATUS_FAILED;
    }
}
#endif /* CONFIG_FSC_HAVE_DP */

static void ProcessDeviceInfo(HostCmd_t *outMsg)
{
    outMsg->deviceInfo.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    outMsg->deviceInfo.cmd.rsp.mcu = 2;
    outMsg->deviceInfo.cmd.rsp.device = 3;
    outMsg->deviceInfo.cmd.rsp.hostcom[0] = 2; /* Low */
    outMsg->deviceInfo.cmd.rsp.hostcom[1] = 1; /* High */
    outMsg->deviceInfo.cmd.rsp.config[0] = 0x00;
    outMsg->deviceInfo.cmd.rsp.config[1] = 0x00;
    outMsg->deviceInfo.cmd.rsp.fw[0] = core_get_rev_lower();
    outMsg->deviceInfo.cmd.rsp.fw[1] = core_get_rev_middle();
    outMsg->deviceInfo.cmd.rsp.fw[2] = core_get_rev_upper();
}

static void ProcessWriteI2CFCSDevice(HostCmd_t* inCmd, HostCmd_t *outMsg,
                                     struct Port *port)
{
    bool result;
    uint8_t reg_addr = inCmd->wrI2CDev.cmd.req.reg[0];
    uint8_t slave_addr = inCmd->wrI2CDev.cmd.req.addr;
    uint8_t *value = inCmd->wrI2CDev.cmd.req.data;

    /* If values are zero default to platform specific values. */
    uint8_t regAlen = ONE_IF_ZERO(inCmd->wrI2CDev.cmd.req.alen);
    uint8_t regDlen = ONE_IF_ZERO(inCmd->wrI2CDev.cmd.req.dlen);
    uint8_t addrInc = ONE_IF_ZERO(inCmd->wrI2CDev.cmd.req.inc);
    uint8_t pktLen = ONE_IF_ZERO(inCmd->wrI2CDev.cmd.req.pktlen);

    result = platform_i2c_write(slave_addr,regAlen, regDlen, pktLen, addrInc,
                                reg_addr, value);

    if (result == true)
    {
        outMsg->rdI2CDev.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    }
    else
    {
        outMsg->rdI2CDev.cmd.rsp.error = HCMD_STATUS_FAILED;
    }
}

static void ProcessReadI2CFCSDevice(HostCmd_t* inCmd, HostCmd_t* outMsg,
                                    struct Port *port)
{
    bool result;
    uint8_t slave_addr = inCmd->rdI2CDev.cmd.req.addr;
    uint8_t reg_addr = inCmd->rdI2CDev.cmd.req.reg[0];
    /* If values are zero default to platform specific values. */
    uint8_t regAlen = ONE_IF_ZERO(inCmd->rdI2CDev.cmd.req.alen);
    uint8_t regDlen = ONE_IF_ZERO(inCmd->rdI2CDev.cmd.req.dlen);
    uint8_t addrInc = ONE_IF_ZERO(inCmd->rdI2CDev.cmd.req.inc);
    uint8_t pktLen = ONE_IF_ZERO(inCmd->rdI2CDev.cmd.req.pktlen);

    uint8_t *buf = outMsg->rdI2CDev.cmd.rsp.data;

    result = platform_i2c_read(slave_addr, regAlen, regDlen, pktLen, addrInc,
                               reg_addr, buf);
    if (result == true)
    {
        outMsg->rdI2CDev.cmd.rsp.error = HCMD_STATUS_SUCCESS;
    }
    else
    {
        outMsg->rdI2CDev.cmd.rsp.error = HCMD_STATUS_FAILED;
    }
}

static void ProcessMsg(uint8_t *inMsgBuffer, uint8_t *outMsgBuffer)
{
    HostCmd_t *inCmd = (HostCmd_t*) inMsgBuffer;
    HostCmd_t *outMsg = (HostCmd_t*) outMsgBuffer;
    uint8_t portIdx = 0;

    HCOM_MEM_FILL(outMsg, 0, sizeof(HostCmd_t));

    outMsg->request.opcode = inCmd->request.opcode;
    outMsg->request.cmd.rsp.status = HCMD_STATUS_SUCCESS;

    switch (inCmd->request.opcode)
    {
    case HCMD_GET_DEVICE_INFO:
        ProcessDeviceInfo(outMsg);
        break;
    case HCMD_READ_I2C_FCS_DEV:
        portIdx = SAFE_INDEX(inCmd->rdI2CDev.cmd.req.module, hostCom.numPorts);
        ProcessReadI2CFCSDevice(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
    case HCMD_WRITE_I2C_FCS_DEV:
        portIdx = SAFE_INDEX(inCmd->wrI2CDev.cmd.req.module, hostCom.numPorts);
        ProcessWriteI2CFCSDevice(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
    case HCMD_USER_CLASS:
        portIdx = SAFE_INDEX(inCmd->userClass.cmd.req.port, hostCom.numPorts);
        ProcessUserClassCmd(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
    case HCMD_TYPEC_CLASS:
        portIdx = SAFE_INDEX(inCmd->typeC.cmd.req.port, hostCom.numPorts);
        ProcessTCStatus(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
    case HCMD_PD_CLASS:
        portIdx = SAFE_INDEX(inCmd->pd.cmd.req.port, hostCom.numPorts);
        ProcessPDStatus(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
#ifdef CONFIG_FSC_HAVE_DP
    case HCMD_DP_CLASS:
        portIdx = SAFE_INDEX(inCmd->dp.cmd.req.port, hostCom.numPorts);
        ProcessDpCommands(inCmd, outMsg, &hostCom.port_list[portIdx]);
        break;
#endif /* CONFIG_FSC_HAVE_DP */
    default:
        /* Return that the request is not implemented */
        outMsg->request.cmd.rsp.status = HCMD_STATUS_FAILED;
        break;
    }
} /* ProcessMsg */

void HCom_Process(void)
{
    ProcessMsg(hostCom.inputBuf, hostCom.outputBuf);
}

void HCom_Init(Port_t *port, uint8_t num)
{
    hostCom.port_list = port;
    hostCom.numPorts = num;
    hostCom.inputBuf = USBInputBuf;
    hostCom.outputBuf = USBOutputBuf;
}

uint8_t* HCom_InBuf(void)
{
    return hostCom.inputBuf;
}

uint8_t* HCom_OutBuf(void)
{
    return hostCom.outputBuf;
}

#endif /* CONFIG_CONFIG_FSC_DEBUG */

