/**
 ******************************************************************************
 * @file        FSCTypes.h
 * @author      ON Semiconductor USB-PD Firmware Team
 * Software License Agreement:
 *
 * The software supplied herewith by ON Semiconductor (the Company)
 * is supplied to you, the Company's customer, for exclusive use with its
 * USB Type C / USB PD products.  The software is owned by the Company and/or
 * its supplier, and is protected under applicable copyright laws.
 * All rights are reserved. Any use in violation of the foregoing restrictions
 * may subject the user to criminal sanctions under applicable laws, as well
 * as to civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN AS IS CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************************
 */

#ifndef FSCTYPES_H_
#define FSCTYPES_H_

#if !defined(__PACKED)
    #define __PACKED
#endif

typedef enum _BOOL { FALSE = 0, TRUE } FSC_BOOL;

/* system default */
typedef signed char         FSC_S8;
typedef signed short int    FSC_S16;
typedef signed long int     FSC_S32;

typedef unsigned char       FSC_U8;
typedef unsigned short int  FSC_U16;
typedef unsigned long int   FSC_U32;

#endif /* _FSCTYPES_H_ */

