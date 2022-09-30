/*******************************************************************************
 * @file     bitfield_translators.h
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
#ifndef __VDM_BITFIELD_TRANSLATORS_H__
#define __VDM_BITFIELD_TRANSLATORS_H__

#include "platform.h"

#ifdef CONFIG_FSC_HAVE_VDM
/*
 * Functions that convert bits into internal header representations...
 */
UnstructuredVdmHeader getUnstructuredVdmHeader(uint32_t in);
StructuredVdmHeader getStructuredVdmHeader(uint32_t in);
IdHeader getIdHeader(uint32_t in);
VdmType getVdmTypeOf(uint32_t in);
/*
 * Functions that convert internal header representations into bits...
 */
uint32_t getBitsForUnstructuredVdmHeader(UnstructuredVdmHeader in);
uint32_t getBitsForStructuredVdmHeader(StructuredVdmHeader in);
uint32_t getBitsForIdHeader(IdHeader in);

/*
 * Functions that convert bits into internal VDO representations...
 */
CertStatVdo getCertStatVdo(uint32_t in);
ProductVdo getProductVdo(uint32_t in);
CableVdo getCableVdo(uint32_t in);
AmaVdo getAmaVdo(uint32_t in);

/*
 * Functions that convert internal VDO representations into bits...
 */
uint32_t getBitsForProductVdo(ProductVdo in);
uint32_t getBitsForCertStatVdo(CertStatVdo in);
uint32_t getBitsForCableVdo(CableVdo in);
uint32_t getBitsForAmaVdo(AmaVdo in);

#endif /* __VDM_BITFIELD_TRANSLATORS_H__ */ // header guard

#endif /* CONFIG_FSC_HAVE_VDM */
