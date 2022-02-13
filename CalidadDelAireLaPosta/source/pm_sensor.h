/*
 * pm_sensor.h
 *
 *  Created on: 3 Oct 2021
 *      Author: user
 */

#ifndef PM_SENSOR_H_
#define PM_SENSOR_H_

#include "fsl_uart_freertos.h"

typedef struct{
	uart_rtos_handle_t* comm_handle;
}pm_sensor_config_t;

//Sets ups necessary resources for sensor use.
int InitializeAirSensor(pm_sensor_config_t* config);

//Performs new measurement of PM10 and returns the value as a float
//which represents ug/m3
float GetPM10reading(void);







#endif /* PM_SENSOR_H_ */
