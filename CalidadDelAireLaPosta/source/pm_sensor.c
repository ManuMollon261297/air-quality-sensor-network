/*
 * pm_sensor.c
 *
 *  Created on: 3 Oct 2021
 *      Author: user
 */
#include "pm_sensor.h"
#include "sensirion_uart.h"
#include "sps30.h"


#define MEASUREMENT_DELAY 5 * 1000000
#define N_RETRIES 5
#define RETRY_DELAY_US 5000 //Minimum delay in us between retry attempts
#define RESET_DELAY_US (100*1000)
#define WAKE_UP_DELAY 5000 //Wake up execution time of 5ms

/************************************************************
 * 					FUNCIONES DEL HEADER					*
 ************************************************************/

int InitializeAirSensor(pm_sensor_config_t* config){
	int result = -1;
	result = sensirion_uart_open(config->comm_handle);
	if( result != 0 ){
		return result;
	}
	else{
		result = sps30_wake_up();
		for(int i = 0; (i < N_RETRIES)&&(result!=0); i++){ //Retries Wake up N_RETRIES or until successful.
			sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
			result = sps30_wake_up();
		}
		if(result != 0){
			return -1; //If wake up fails return invalid measurement.
		}
		sensirion_sleep_usec(WAKE_UP_DELAY);
		result = -1;
		for(int i=0; (i<N_RETRIES)&&(result!=0); i++){
			result = sps30_reset();
			sensirion_sleep_usec(RETRY_DELAY_US);
		}
		if(result!=0){
			return result;
		}
		sensirion_sleep_usec(RESET_DELAY_US);
		return sps30_sleep();
	}

}

float GetPM10reading(void){
	struct sps30_measurement result;
	int16_t ret = -1;

	ret = sps30_wake_up();
	for(int i = 0; (i < N_RETRIES)&&(ret!=0); i++){ //Retries Wake up N_RETRIES or until successful.
		sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
		ret = sps30_wake_up();
	}
	if(ret != 0){
		return -1; //If wake up fails return invalid measurement.
	}
	sensirion_sleep_usec(WAKE_UP_DELAY);
	ret = sps30_start_measurement();
	for(int i = 0; (i < N_RETRIES)&&(ret!=0); i++){ //Retries switch to measurement mode N_RETRIES or until successful.
		sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
		ret = sps30_start_measurement();
	}
	if(ret != 0){
		return -1; //If switch to measurement mode fails return invalid measurement.
	}

	sensirion_sleep_usec(MEASUREMENT_DELAY); //Waits for new measurement to be performed
	ret = sps30_read_measurement(&result);
	for(int i = 0; (i < N_RETRIES)&&(ret!=0); i++){ //Retries measurement read N_RETRIES or until successful.
		sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
		ret = sps30_read_measurement(&result);
	}
	if(ret != 0){
		return -1; //If measurement reading fails return invalid measurement.
	}

	ret = sps30_stop_measurement();
	for(int i = 0; (i < N_RETRIES)&&(ret!=0); i++){ //Retries switch to idle mode N_RETRIES or until successful.
		sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
		ret = sps30_stop_measurement();
	}
	if(ret != 0){
		return -1; //If switch to idle mode fails return invalid measurement.
	}
	ret = sps30_sleep();
	for(int i = 0; (i < N_RETRIES)&&(ret!=0); i++){ //Retries switch to sleep mode N_RETRIES or until successful.
		sensirion_sleep_usec(RETRY_DELAY_US); //Delay of RETRY_DELAY_US between tries
		ret =  sps30_sleep();
	}
	if(ret != 0){
		return -1; //If switch to sleep mode fails return invalid measurement.
	}
	return result.mc_10p0;

}
