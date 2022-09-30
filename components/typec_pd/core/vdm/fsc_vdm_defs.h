/*******************************************************************************
 * @file     fsc_vdm_defs.h
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
#ifndef __FSC_VDM_DEFS_H__
#define __FSC_VDM_DEFS_H__

#include "vdm_types.h"

#ifdef CONFIG_FSC_HAVE_VDM

/*
 *  definition/configuration object - these are all the things that the system needs to configure.
 */
typedef struct {
    bool                        data_capable_as_usb_host    : 1;
    bool                        data_capable_as_usb_device  : 1;
    ProductType                     product_type                : 3;
    bool                        modal_operation_supported   : 1;
    uint16_t                         usb_vendor_id               : 16;
    /* for Cert Stat VDO, "allocated by USB-IF during certification" */
    uint32_t                         test_id                     : 20;
    uint16_t                         usb_product_id              : 16;
    uint16_t                         bcd_device                  : 16;
    uint8_t                          cable_hw_version            : 4;
    uint8_t                          cable_fw_version            : 4;
    CableToType                     cable_to_type               : 2;
    CableToPr                       cable_to_pr                 : 1;
    CableLatency                    cable_latency               : 4;
    CableTermType                   cable_term                  : 2;
    SsDirectionality                sstx1_dir_supp              : 1;
    SsDirectionality                sstx2_dir_supp              : 1;
    SsDirectionality                ssrx1_dir_supp              : 1;
    SsDirectionality                ssrx2_dir_supp              : 1;
    VbusCurrentHandlingCapability   vbus_current_handling_cap   : 2;
    VbusThruCable                   vbus_thru_cable             : 1;
    Sop2Presence                    sop2_presence               : 1;

    VConnFullPower                  vconn_full_power            : 3;
    VConnRequirement                vconn_requirement           : 1;
    VBusRequirement                 vbus_requirement            : 1;

    UsbSsSupport                    usb_ss_supp                 : 3;
    AmaUsbSsSupport                 ama_usb_ss_supp             : 3;

    uint32_t                         num_svids;
    uint16_t                         svids[MAX_NUM_SVIDS];
    uint32_t                         num_modes_for_svid[MAX_NUM_SVIDS];

    /* TODO: A lot of potential wasted memory here... */
    uint32_t                         modes[MAX_NUM_SVIDS][MAX_MODES_PER_SVID];

} VendorDefinition;

#endif /* CONFIG_FSC_HAVE_VDM */

#endif /* __FSC_VDM_DEFS_H__*/


