/*******************************************************************************
 * @file     Log.c
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
#include "Log.h"

#ifdef CONFIG_CONFIG_FSC_DEBUG

void InitializeStateLog(StateLog *log)
{
    log->Count = 0;
    log->End = 0;
    log->Start = 0;
}

bool WriteStateLog(StateLog *log, uint16_t state, uint32_t time)
{
    if(!IsStateLogFull(log))
    {
        uint8_t index = log->End;
        log->logQueue[index].state = state;
        log->logQueue[index].time_s = time >> 16;     /* Upper 16: seconds */
        log->logQueue[index].time_ms = time & 0xFFFF; /* Lower 16: 0.1ms */

        log->End += 1;
        if(log->End == LOG_SIZE)
        {
            log->End = 0;
        }

        log->Count += 1;

        return true;
    }
    else
    {
        return false;
    }
}

bool ReadStateLog(StateLog *log, uint16_t * state,
                      uint16_t * time_ms, uint16_t * time_s)
{
    if(!IsStateLogEmpty(log))
    {
        uint8_t index = log->Start;
        *state = log->logQueue[index].state;
        *time_ms = log->logQueue[index].time_ms;
        *time_s = log->logQueue[index].time_s;

        log->Start += 1;
        if(log->Start == LOG_SIZE)
        {
            log->Start = 0;
        }

        log->Count -= 1;
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t GetStateLog(StateLog *log, uint8_t *data, uint8_t bufLen)
{
    int32_t i;
    int32_t entries = log->Count;
    uint16_t state_temp;
    uint16_t time_tms_temp;
    uint16_t time_s_temp;
    uint32_t len = 0;

    for (i = 0; i < entries; i++)
    {
        if (bufLen < 5 ) { break; }
        if (ReadStateLog(log, &state_temp, &time_tms_temp, &time_s_temp))
        {
            data[len++] = state_temp;
            data[len++] = (uint8_t) time_tms_temp;
            data[len++] = (time_tms_temp >> 8);
            data[len++] = (uint8_t) time_s_temp;
            data[len++] = (time_s_temp) >> 8;
            bufLen -= 5;
        }
    }

    return len;
}
bool IsStateLogFull(StateLog *log)
{
    return (log->Count == LOG_SIZE) ? true : false;
}

bool IsStateLogEmpty(StateLog *log)
{
    return (!log->Count) ? true : false;
}

#endif /* CONFIG_CONFIG_FSC_DEBUG */

