/*
 * bluetooth_com.c
 *
 *  Created on: Jan 24, 2021
 *      Author: Lu
 */

#include "data_com.h"
#include "sim800l.h"

#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
#include "fsl_debug_console.h"
#endif
//FreeRTOS includes
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "peripherals.h"
#include "rtc.h"
#include "sdcard.h"
#include "power_module.h"
#include "fsl_gpio.h"

/****************************************************************
 * 							CONSTANTS							*
 ****************************************************************/
#define URL_STRING "GET https://api.thingspeak.com/update\?api_key=0CM35ZLCLQ2JMTLG"
#define MAX_RETRIES	3
#define NODE_ID 1

/****************************************************************
 * 						STATIC VARIABLES						*
 ****************************************************************/
static uint8_t hourCounter = 1;

/************************************************************
 *					 HEADER FUNCTIONS						*
 ************************************************************/

void InitDataCom(void){
}

uint8_t GetHourCounter(){
	return hourCounter;
}
void ResetHourCounter(){
	hourCounter = 1;
}
void IncrementHourCounter(){
	hourCounter++;
}

/* RTC_IRQn interrupt handler */
void RTC_COMMON_IRQHANDLER(void) {
  /* Get status flags */
  uint32_t status_flags = RTC_GetStatusFlags(RTC_PERIPHERAL);

  /* Place your interrupt code here */

  /* Clear status flags */
  RTC_ClearStatusFlags(RTC_PERIPHERAL, status_flags);
  rtc_datetime_t date;
  RTC_GetDatetime(RTC_PERIPHERAL, &date);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
  PRINTF("Triggered Alarm: %02d:%02d:%02d\r\n", date.hour, date.minute, date.second);
#endif

  BaseType_t woke_flag = pdFALSE;
  //xSemaphoreGiveFromISR(communication_sem, &woke_flag);
  portYIELD_FROM_ISR(woke_flag);

  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
     Store immediate overlapping exception return operation might vector to incorrect interrupt. */
  #if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
  #endif
}


void ComWaitForSamples(void){
	Enter_power_mode(DEEP_SLEEP);
	RebootAll();
}

bool SendDataPackage(data_trans_t* pkg, rtc_datetime_t datetime)
{
	char final_string[150] = {0};
	bool status_ok = false;
	create_string(final_string, URL_STRING, pkg->pressAvg, pkg->tempAvg, pkg->PM10Avg, pkg->battery_lvlAvg, pkg->humAvg, NODE_ID);
	uint8_t retries = 0;
	do{
		status_ok = sendGetRequest(final_string);
		retries = retries + 1;
	}while((!status_ok) && (retries < MAX_RETRIES));

	return status_ok;

}




