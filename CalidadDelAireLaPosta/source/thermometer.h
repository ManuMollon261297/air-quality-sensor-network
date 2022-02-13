/*
 * thermometer.h
 *
 *  Created on: 24 Jan 2021
 *      Author: G5
 */

#ifndef THERMOMETER_H_
#define THERMOMETER_H_

/********************************************************
 * 					HEADERS UTILIZADOS					*
 ********************************************************/

#include <stdint.h>

/********************************************************
 * 						DEFINICIONES					*
 ********************************************************/
typedef enum{ERROR=-1, NO_ERROR}temp_Errors_t;

/********************************************************
 * 						FUNCIONES						*
 ********************************************************/

//Recieves handle for serial communication
temp_Errors_t InitializeThermometer(void*);

/* Returns current temperature without performing a measurement */
double getTemperature(void);

/* Returns current humidity without performing a measurement */
double getHumidity(void);

/* Returns current pressure without performing a measurement */
double getPressure(void);

/*Performs new temperature and humidity measurement */
temp_Errors_t newSampleRequest(void);

#endif /* THERMOMETER_H_ */
