/*
 * bluetooth_com.h
 *
 *  Created on: Jan 24, 2021
 *      Author: Lu
 */
/*
* El equipo deberá tener una frecuencia de muestreo no menor a 15 minutos
* de manera de obtener mediciones menos ruidosas y actualizar el mapa cada 60 minutos.
*	es decir envio cada una hora cuatro muestras.
*/
#ifndef DATA_COM_H_
#define DATA_COM_H_

#include <stdint.h>
#include "fsl_rtc.h"
#include "fsl_uart_freertos.h"

#define MAX_SAMPLE_PER_HOUR 4

typedef struct{
	double PM10[MAX_SAMPLE_PER_HOUR];	//pm 10
	double temp[MAX_SAMPLE_PER_HOUR];	//temperatura
	double hum[MAX_SAMPLE_PER_HOUR];	//humedad
	double press[MAX_SAMPLE_PER_HOUR]; //presion atmosferica
	double battery_lvl[MAX_SAMPLE_PER_HOUR]; // Tension de la alimentacion
	double PM10Avg;	//pm 10
	double tempAvg;	//temperatura
	double humAvg;	//humedad
	double pressAvg; //presion atmosferica
	double battery_lvlAvg; //Tension de bateria
}data_trans_t;

typedef struct{
	uart_rtos_handle_t* handle;
}data_init_t;

void InitDataCom(void);
uint8_t GetHourCounter();
void ResetHourCounter();
void IncrementHourCounter();
bool SendDataPackage(data_trans_t* pkg, rtc_datetime_t datetime);

void ComWaitForSamples(void);



#endif /* DATA_COM_H_ */
