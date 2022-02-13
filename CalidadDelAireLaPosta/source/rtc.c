/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "fsl_debug_console.h"
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "peripherals.h"
#include "rtc.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Set the alarm which will be trigerred x secs later. The alarm trigger
 *        will print a notification on the console.
 *
 * @param offsetSec  Offset seconds is set for alarm.
 */


/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint8_t g_AlarmPending = 0U;
volatile bool g_SecsFlag        = false;



/*******************************************************************************
 * Code
 ******************************************************************************/

void InitRTC(){


//	/* Set RTC time to default */
//	RTC_SetDatetime(RTC, &date);
//
//	/* Enable RTC alarm interrupt */
//	RTC_EnableInterrupts(RTC, kRTC_AlarmInterruptEnable);
//
//	/* Enable at the NVIC */
//	EnableIRQ(RTC_IRQn);
}

/*!
 * @brief Override the RTC IRQ handler.
 */
//void RTC_IRQHandler(void)
//{
//    if (RTC_GetStatusFlags(RTC) & kRTC_AlarmFlag)
//    {
//        g_AlarmPending = 1U;
//        PRINTF("entré en rtc_irqhandler");
//        /* Clear alarm flag */
//        RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);
//    }
//    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//    exception return operation might vector to incorrect interrupt */
//    __DSB();
//}


/*!
 * brief Sets the RTC hourly alarm time.
 *
 * The function checks whether the specified alarm time is greater than the present
 * time. If not, the function does not set the alarm and returns an error.
 *
 * param base      RTC peripheral base address
 * param alarmTime Pointer to the structure where the alarm time is stored.
 *
 * return kStatus_Success: success in setting the RTC alarm
 *         kStatus_InvalidArgument: Error because the alarm datetime format is incorrect
 *         kStatus_Fail: Error because the alarm time has already passed
 */
status_t RTCSetNextAlarm(RTC_Type *base, uint32_t secondsToNextAlarm)
{
    uint32_t alarmSeconds = 0;
    uint32_t currSeconds  = 0;

    /* Get the current time */
    currSeconds = base->TSR;

    alarmSeconds = currSeconds + secondsToNextAlarm;

    /* Return error if the alarm time has passed */
    if (alarmSeconds < currSeconds)
    {
        return kStatus_Fail;
    }

    /* Set alarm in seconds*/
    base->TAR = alarmSeconds;

    return kStatus_Success;
}


/*!
 * @brief Override the RTC Second IRQ handler.
 */
//void RTC_Seconds_IRQHandler(void)
//{
//    g_SecsFlag = true;
//    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//    exception return operation might vector to incorrect interrupt */
//    __DSB();
//}
static void CommandAlarm(uint8_t offsetSec)
{
    rtc_datetime_t date;

    RTC_EnableInterrupts(RTC_PERIPHERAL, kRTC_AlarmInterruptEnable);
    if ((offsetSec < 1U) || (offsetSec > 9U))
    {
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("Invalid input format\r\n");
#endif
        return;
    }
    /* Get date time and add offset*/
    RTC_GetDatetime(RTC_PERIPHERAL, &date);
    date.second += offsetSec;
    if (date.second > 59U)
    {
        date.second -= 60U;
        date.minute += 1U;
        if (date.minute > 59U)
        {
            date.minute -= 60U;
            date.hour += 1U;
            if (date.hour > 23U)
            {
                date.hour -= 24U;
                date.day += 1U;
            }
        }
    }

    /* Set the datetime for alarm */
    if (RTC_SetAlarm(RTC_PERIPHERAL, &date) == kStatus_Success)
    {
        /* Alarm was successfully set, wait for alarm interrupt */
        while (g_AlarmPending == 0U)
        {
        }
    }
    else
    {
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("Failed to set alarm. Alarm time is not in the future\r\n");
#endif
        return;
    }
    /* Interrupt done */
    RTC_GetDatetime(RTC_PERIPHERAL, &date);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    PRINTF("Triggered Alarm: %02d:%02d:%02d\r\n", date.hour, date.minute, date.second);
#endif
    g_AlarmPending = 0U;
    RTC_DisableInterrupts(RTC_PERIPHERAL, kRTC_AlarmInterruptEnable);
}

static void CommandGetDatetime(void)
{
    rtc_datetime_t date;

    RTC_GetDatetime(RTC_PERIPHERAL, &date);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    PRINTF("Current datetime: %04d-%02d-%02d %02d:%02d:%02d\r\n", date.year, date.month, date.day, date.hour,
           date.minute, date.second);
#endif
}


//
///*!
// * @brief Main function
// */
//int rtc_fun(void)
//{
//    uint8_t index;
//    uint8_t secs;
//    char recvBuf[20];
//    uint32_t result;
//    uint16_t year;
//    uint16_t month;
//    uint16_t day;
//    uint16_t hour;
//    uint16_t minute;
//    uint16_t second;
//    rtc_datetime_t date;
//
//    /* Board pin, clock, debug console init */
//    BOARD_InitPins();
//    BOARD_InitBootClocks();
//    BOARD_InitBootPeripherals();
//    BOARD_InitDebugConsole();
//
//#if !(defined(FSL_FEATURE_RTC_HAS_NO_CR_OSCE) && FSL_FEATURE_RTC_HAS_NO_CR_OSCE)
//
//    /* Select RTC clock source */
//    RTC_SetClockSource(RTC_PERIPHERAL);
//#endif /* FSL_FEATURE_RTC_HAS_NO_CR_OSCE */
//
//    PRINTF("\r\nRTC Peripheral Demo running...\r\n");
//
//    /* Start loop */
//    while (1)
//    {
//        /* Print the user information */
//        PRINTF("\r\nSelect:");
//        /* Get user input */
//        index = GETCHAR();
//        PUTCHAR(index);
//
//
//    }
//}
