/*******************************************************************************
 * @file     Log.h
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
#ifndef FSC_LOG_H
#define	FSC_LOG_H

#include "platform.h"

#ifdef CONFIG_CONFIG_FSC_DEBUG

#define LOG_SIZE 64

typedef struct{
    uint16_t state;
    uint16_t time_ms;
    uint16_t time_s;
} StateLogEntry;

typedef struct{
    StateLogEntry logQueue[ LOG_SIZE ];
    uint8_t Start;
    uint8_t End;
    uint8_t Count;
} StateLog;

/**
 * @brief Initializes the log values.
 *
 * @param log Pointer to the log structure
 * @return None
 */
void InitializeStateLog(StateLog *log);

/**
 * @brief Write a single state entry to the log and advance the log pointer
 *
 * @param log Pointer to the log structure
 * @param state Current state enum value
 * @param time Packed value - upper 16: seconds, lower 16: 0.1ms
 * @return TRUE if successful (i.e. the log isn't full)
 */
bool WriteStateLog(StateLog *log, uint16_t state, uint32_t time);

/**
 * @brief Read a single entry from the log and advance the log pointer
 *
 * @param log Pointer to the log structure
 * @param state Current state enum value
 * @param time_tenthms Timestamp (fraction value - 0.1ms resolution)
 * @param time_s Timestamp (seconds value)
 * @return TRUE if successful (i.e. the log isn't empty)
 */
bool ReadStateLog(StateLog *log, uint16_t *state,
                      uint16_t *time_tenthms, uint16_t *time_s);

/**
 * @brief Fill a byte buffer with log data
 *
 * @param log Pointer to the log structure
 * @param data Byte array to copy log data into
 * @param bufLen Available space in data buffer
 * @return Number of bytes written into buffer
 */
uint32_t GetStateLog(StateLog *log, uint8_t *data, uint8_t bufLen);

/**
 * @brief Check state of log, whether Full or Empty.
 *
 * @param log Pointer to the log structure
 * @return TRUE or FALSE, whether Full or Empty
 */
bool IsStateLogFull(StateLog *log);
bool IsStateLogEmpty(StateLog *log);

#endif // CONFIG_CONFIG_FSC_DEBUG

#endif	/* FSC_LOG_H */

