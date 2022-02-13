/*
 * battery_monitor.c
 *
 *  Created on: Jan 15, 2022
 *      Author: mmollon
 */

#include "battery_monitor.h"
#include "ads1115.h"
#include "peripherals.h"
#include <math.h>
#include "uart_debug.h"

#define UART_INIT_DEBUG_CONSOLE_PERIPHERAL

/* Initialize Monitor */
void InitBaterryMonitor(void){
	ADS1115(ADS1115_DEFAULT_ADDRESS, ADS1115_PGA_4P096, &I2C0_rtosHandle);
}

/* Get Current Battery Level in percentage format (0-100) */
float getBatteryLevel(void){

	int16_t mV;
	float percentage;

	mV = ADS1115getMilliVolts(true);
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
	char message[100] = {0};
	sprintf(message, "Milivolts measured: %d\r\n", mV);
	uart_print(message);
#endif
	percentage = ((mV-ZERO_PERCENT_MV)/MV_RANGE)*100;
	if(percentage>100){
		percentage = 100;
	}else if(percentage<0){
		percentage = 0;
	}else{
		percentage = floorf(percentage);
	}

	return percentage;
}
