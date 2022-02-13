/*
 * Copyright 2016-2021 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    main.c
 * @brief   Application entry point.
 */
/* Board */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"


/* FreeRTOS */
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"
#include "projdefs.h"
#include "semphr.h"

/* Peripherals */
#include "thermometer.h"
#include "pm_sensor.h"
#include "sdcard.h"
#include "data_com.h"
#include "rtc.h"
#include "battery_monitor.h"
#include "sim800l.h"
#include "power_module.h"

/* Debug*/
#include "uart_debug.h"

/*
 * @brief   Application entry point.
 */
#define SAMPLES_PER_CYCLE 3
#define MAX_RETRIES 3
#define UART_INIT_DEBUG_CONSOLE_PERIPHERAL



/********************************************************
 * 					LOCAL FUNCTIONS						*
 ********************************************************/

void prvSetupHardware(void);
double average(double* array);
void GetSamplesAverage(data_trans_t* pkg);
void transmiterTask(void*);
void setNextQuarterAlarm(RTC_Type * rtc);

int main(void){
	/* Perform any hardware setup necessary. */
	prvSetupHardware();
	/* Transmiter Task */
	if(xTaskCreate(transmiterTask, "Transmission Thread", 1024, NULL, 1, NULL) != pdPASS){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("Task Trans creation failed!.\r\n");
#endif
	}
	/* Start the scheduler so the created tasks start executing. */
	vTaskStartScheduler();

	return 0;
}

void prvSetupHardware(void){
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/* RTC wakeup enabled */
	RTC_EnableWakeUpPin(RTC_PERIPHERAL, true);

	#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
	#endif
	/* Set SMC configuration */
	BOARD_InitBootPowerModule();
	/* Set interrupt priorities */
	NVIC_SetPriority(SPI0_IRQn, 2);
	NVIC_SetPriority(UART3_RX_TX_IRQn, 2);
	NVIC_SetPriority(UART4_RX_TX_IRQn, 2);
    NVIC_SetPriority(I2C0_IRQn, 2);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    PRINTF("Hardware Setup Finished\n");
#endif
}

/* Transmit through GPRS GET request and save in SD CARD with RTC (Real Time Clock) data */
void transmiterTask(void* params){
#ifdef LED_VISUALIZATION
	/* Turn on green led for initialization indicator. */
	LED_RED_ON();
#endif
	temp_Errors_t t_error;
	int sd_error;
	do{
		sd_error = InitSDCard();
		if(sd_error != 0){
			sd_reset_status();
		}
	}while(sd_error != 0);
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
	uart_print("SD initialization successful\n");
#endif
#ifdef LED_VISUALIZATION
	/* Turn off all leds. */
	LED_RED_OFF();
	/* Turn on green led for initialization indicator. */
	LED_GREEN_ON();
#endif
	InitBaterryMonitor();
	InitDataCom(); /* Creates semaphore */
	Init_sim();
	/* InitSensors */
	pm_sensor_config_t pm_config = { .comm_handle = &UART4_rtos_handle};
	int pm_error = -1;
	pm_error = InitializeAirSensor(&pm_config);
	if(pm_error != 0){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("PM sensor initialization failed \n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Hardware Setup Finished\n");
#endif
		//return;
	}
	else{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("PM sensor initialization successful \n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("PM sensor initialization successful \n");
#endif
	}
	temp_Errors_t error = InitializeThermometer(&SPI0_rtosHandle);
	if (error != NO_ERROR) {
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("Temperature sensor initialization failed \n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Temperature sensor initialization failed \n");
#endif
		return;
	}
	else{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("Temperature sensor initialization successful \n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Temperature sensor initialization successful \n");
#endif
	}

	data_trans_t pkg;
	rtc_datetime_t date;
	uint8_t message_buffer[100];
	bool status_ok = false;
	uint8_t retries = 0;
	/* Get current time and set. */
	do{
		status_ok = getLiveTime(message_buffer, 100, &date);
		retries = retries + 1;
	}while((!status_ok) && (retries < MAX_RETRIES));
	if(!status_ok){
#ifdef LED_VISUALIZATION
		LED_RED_ON();
		vTaskDelay(pdMS_TO_TICKS(500));
		LED_RED_OFF();
		vTaskDelay(pdMS_TO_TICKS(500));
		LED_RED_ON();
		vTaskDelay(pdMS_TO_TICKS(500));
		LED_RED_OFF();
		vTaskDelay(pdMS_TO_TICKS(500));
#endif

	}

	/* Stop RTC timer */
	RTC_StopTimer(RTC_PERIPHERAL);
	/* Date and time initialization */
	RTC_SetDatetime(RTC_PERIPHERAL, &date);
	/* Start RTC timer */
	RTC_StartTimer(RTC_PERIPHERAL);

	RTC_GetDatetime(RTC_PERIPHERAL, &date);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	PRINTF("Start Time: %02d:%02d:%02d\r\n", date.hour, date.minute, date.second);
#endif
	RTCSetNextAlarm(RTC_PERIPHERAL, 10);
	uint8_t hour;

#ifdef LED_VISUALIZATION
	LED_GREEN_OFF();
#endif

	while(1){
		hour = GetHourCounter();
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("%d\r\n", hour);
#endif

#ifdef LED_VISUALIZATION
		LED_BLUE_ON();
#endif
		/* Wait for next measure alarm. */
		ComWaitForSamples();
#ifdef LED_VISUALIZATION
		LED_BLUE_OFF();
#endif

		RTC_GetDatetime(RTC_PERIPHERAL, &date);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("Current Time: %02d:%02d:%02d\r\n", date.hour, date.minute, date.second);
#endif

		t_error = newSampleRequest();
#ifdef LED_VISUALIZATION
		LED_RED_ON();
		LED_GREEN_ON();
#endif
		pkg.PM10[hour-1] = GetPM10reading();
		pkg.temp[hour-1] = getTemperature();
		pkg.hum[hour-1] =  getHumidity();
		pkg.press[hour-1] = getPressure();
		pkg.battery_lvl[hour-1] = getBatteryLevel();

#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		char message[100] = {0};
		sprintf(message, "PM measured: %0.1f\r\n", pkg.PM10[hour-1]);
		uart_print(message);
		sprintf(message, "Temperature measured: %0.2f\r\n", pkg.temp[hour-1]);
		uart_print(message);
		sprintf(message, "Humidty measured: %0.2f\r\n", pkg.hum[hour-1]);
		uart_print(message);
		sprintf(message, "Pressure measured: %0.2f\r\n", pkg.press[hour-1]);
		uart_print(message);
		sprintf(message, "Battery Level measured: %0.1f\r\n", pkg.battery_lvl[hour-1]);
		uart_print(message);
#endif

#ifdef LED_VISUALIZATION
		LED_RED_OFF();
		LED_GREEN_OFF();
#endif

		setNextQuarterAlarm(RTC_PERIPHERAL);
		IncrementHourCounter();

		if(hour == SAMPLES_PER_CYCLE){	/* After one hour, send samples average and save in SD card */
			GetSamplesAverage(&pkg);
#ifdef LED_VISUALIZATION
			LED_RED_ON();
			LED_BLUE_ON();
#endif
			/* Store measurements in non volatile memory */
			SaveDataSDCard(&pkg, date);
#ifdef LED_VISUALIZATION
			LED_RED_OFF();
			LED_BLUE_OFF();
#endif

#ifdef LED_VISUALIZATION
			LED_RED_ON();
			LED_BLUE_ON();
			LED_GREEN_ON();
#endif
			bool status_ok = SendDataPackage(&pkg, date);
#ifdef LED_VISUALIZATION
			LED_RED_OFF();
			LED_BLUE_OFF();
			LED_GREEN_OFF();
#endif
			if(!status_ok){
#ifdef LED_VISUALIZATION
				LED_RED_ON();
				vTaskDelay(pdMS_TO_TICKS(500));
				LED_RED_OFF();
				vTaskDelay(pdMS_TO_TICKS(500));
				LED_RED_ON();
				vTaskDelay(pdMS_TO_TICKS(500));
				LED_RED_OFF();
				vTaskDelay(pdMS_TO_TICKS(500));
#endif
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
				PRINTF("Failed to communicate with ThingSpeak\r\n");
#endif
			}
			ResetHourCounter();
		}
	}
}

void setNextQuarterAlarm(RTC_Type * rtc){
	rtc_datetime_t  date;
	RTC_GetDatetime(rtc, &date);
	uint8_t quarter;
	if(date.minute < 5){
		quarter = 5;
	}else if(date.minute < 10){
		quarter = 10;
	}else if(date.minute < 15){
		quarter = 15;
	}else if(date.minute < 20){
		quarter = 20;
	}else if(date.minute < 25){
		quarter = 25;
	}else if(date.minute < 30){
		quarter = 30;
	}else if(date.minute < 35){
		quarter = 35;
	}else if(date.minute < 40){
		quarter = 40;
	}else if(date.minute < 45){
		quarter = 45;
	}else if(date.minute < 50){
		quarter = 50;
	}else if(date.minute < 55){
		quarter = 55;
	}else{
		quarter = 60;
	}

	RTCSetNextAlarm(rtc, (uint32_t) ((quarter - date.minute)*60 - date.second - 1));
}

void GetSamplesAverage(data_trans_t* pkg){
	pkg->PM10Avg = average(pkg->PM10);
	pkg->pressAvg = average(pkg->press);
	pkg->tempAvg = average(pkg->temp);
	pkg->humAvg = average(pkg->hum);
	pkg->battery_lvlAvg = average(pkg->battery_lvl);
}

double average(double* array){
	double avg = 0;
	for(int i =0; i<SAMPLES_PER_CYCLE; i++){
		avg += array[i] / SAMPLES_PER_CYCLE;
	}
	return avg;
}
