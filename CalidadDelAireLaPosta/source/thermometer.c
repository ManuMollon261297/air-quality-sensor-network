/*
 * thermometer.c
 *
 *  Created on: 24 Jan 2021
 *      Author: G5
 */

/********************************************************
 *					 HEADERS UTILIZADOS					*
 ********************************************************/

#include "thermometer.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "../board/board.h"
#include "../utilities/fsl_debug_console.h"
#include <stdint.h>
#include <stdbool.h>
//FreeRTOS includes
#include "fsl_dspi_freertos.h"
#include "FreeRTOS.h"
#include "semphr.h"

/********************************************************
 * 						DEFINCIONES						*
 ********************************************************/
#define MEAS_DELAY 50000 //Wait time of 50ms for sensor measurement

/********************************************************
 * 					VARIABLES GLOBALES					*
 ********************************************************/

static struct bme280_data sample; //Last measured sample
static SemaphoreHandle_t sample_mutex;
static struct bme280_dev bme_handle;

/********************************************************
 * 					FUNCIONES LOCALES					*
 ********************************************************/
int8_t user_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	uint8_t address = reg_addr;
	uint8_t rx_buffer[64];
    int8_t rslt = kStatus_Fail; /* Return 0 for Success, non-zero for failure */
    dspi_transfer_t params = {&address, rx_buffer, len+1, kDSPI_MasterCtar0 |kDSPI_MasterPcs4|kDSPI_MasterPcsContinuous};
    rslt =  (int8_t) DSPI_RTOS_Transfer( (dspi_rtos_handle_t*)intf_ptr, &params );
    /*
     * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
     * to be set low to activate the relevant device on the SPI bus
     */

    /*
     * Data on the bus should be like
     * |----------------+---------------------+-------------|
     * | MOSI           | MISO                | Chip Select |
     * |----------------+---------------------|-------------|
     * | (don't care)   | (don't care)        | HIGH        |
     * | (reg_addr)     | (don't care)        | LOW         |
     * | (don't care)   | (reg_data[0])       | LOW         |
     * | (....)         | (....)              | LOW         |
     * | (don't care)   | (reg_data[len - 1]) | LOW         |
     * | (don't care)   | (don't care)        | HIGH        |
     * |----------------+---------------------|-------------|
     */
    for(int i =0; i<len; i++){
    	reg_data[i] = rx_buffer[i+1];
    }
    return rslt;
}

int8_t user_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	int8_t rslt = kStatus_Fail; /* Return 0 for Success, non-zero for failure */
	uint8_t tx[64];	//Tx buffer for SPI transfer
	tx[0] = reg_addr;
	for(int i=0; i<len; i++){
		tx[i+1] = reg_data[i];
	}
	dspi_transfer_t params = {tx, NULL, len+1, kDSPI_MasterCtar0 | kDSPI_MasterPcs4 | kDSPI_MasterPcsContinuous};
	rslt =  (int8_t) DSPI_RTOS_Transfer( (dspi_rtos_handle_t*)intf_ptr, &params );
    /*
     * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
     * to be set low to activate the relevant device on the SPI bus
     */

    /*
     * Data on the bus should be like
     * |---------------------+--------------+-------------|
     * | MOSI                | MISO         | Chip Select |
     * |---------------------+--------------|-------------|
     * | (don't care)        | (don't care) | HIGH        |
     * | (reg_addr)          | (don't care) | LOW         |
     * | (reg_data[0])       | (don't care) | LOW         |
     * | (....)              | (....)       | LOW         |
     * | (reg_data[len - 1]) | (don't care) | LOW         |
     * | (don't care)        | (don't care) | HIGH        |
     * |---------------------+--------------|-------------|
     */

    return rslt;
}

void user_delay_us(uint32_t period, void *intf_ptr){
	vTaskDelay( pdMS_TO_TICKS( 5 + (period/1000) ) );
}


/********************************************************
 * 					FUNCIONES DEL HEADER				*
 ********************************************************/

temp_Errors_t InitializeThermometer(void* spi_handle){
	sample_mutex = xSemaphoreCreateMutex();
	int8_t rslt = BME280_OK;

	/* Sensor_0 interface over SPI with native chip select line */
	bme_handle.intf_ptr = spi_handle;
	bme_handle.intf = BME280_SPI_INTF;
	bme_handle.read = user_spi_read;
	bme_handle.write = user_spi_write;
	bme_handle.delay_us = user_delay_us;
	//BME oversampling and filter settings
	bme_handle.settings.osr_p =BME280_OVERSAMPLING_1X;
	bme_handle.settings.osr_h = BME280_OVERSAMPLING_1X;
	bme_handle.settings.osr_t = BME280_OVERSAMPLING_1X;
	bme_handle.settings.filter = BME280_FILTER_COEFF_OFF;

	rslt = bme280_init(&bme_handle);
	if(rslt != BME280_OK) {
		return ERROR;	//Initialization failed
	}
	else{
		rslt = bme280_set_sensor_settings(BME280_FILTER_SEL, &bme_handle);
		if(rslt !=BME280_OK){
			return ERROR;
		}
		rslt = bme280_set_sensor_settings(BME280_OSR_PRESS_SEL, &bme_handle);
		if(rslt !=BME280_OK){
			return ERROR;
		}
		rslt = bme280_set_sensor_settings(BME280_OSR_TEMP_SEL, &bme_handle);
		if(rslt !=BME280_OK){
			return ERROR;
		}
		rslt = bme280_set_sensor_settings(BME280_OSR_HUM_SEL, &bme_handle);
		if(rslt !=BME280_OK){
			return ERROR;
		}

		rslt = bme280_set_sensor_mode(BME280_SLEEP_MODE, &bme_handle);//Put sensor on sleep mode
		if(rslt!=BME280_OK){
			return ERROR;
		}
		return NO_ERROR;
	}
}

temp_Errors_t newSampleRequest(void){
	int8_t rslt;
	rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme_handle); //Perform measurement
	if(rslt!=BME280_OK){
		return ERROR;
	}
	user_delay_us(MEAS_DELAY, NULL); //Wait for measurement to be performed
	xSemaphoreTake( sample_mutex, portMAX_DELAY ); //Bloquea recurso compartido
	rslt = bme280_get_sensor_data(BME280_ALL, &sample, &bme_handle);
	xSemaphoreGive( sample_mutex );

	rslt = bme280_set_sensor_mode(BME280_SLEEP_MODE, &bme_handle); //Puts device on sleep mode again
	if(rslt!=BME280_OK){
		return ERROR;
	}
	else{
		return NO_ERROR;
	}
}

double getTemperature(void){
	double ret_value = 0;

	xSemaphoreTake( sample_mutex, portMAX_DELAY ); //Bloquea recurso compartido
	ret_value = sample.temperature;
	xSemaphoreGive( sample_mutex );				//Desbloquea recuros compartido

	return ret_value;
}

double getHumidity(void){
	double ret_value = 0;

	xSemaphoreTake( sample_mutex, portMAX_DELAY ); //Bloquea recurso compartido
	ret_value = sample.humidity;
	xSemaphoreGive( sample_mutex );				//Desbloquea recuros compartido

	return ret_value;
}

double getPressure(void){
	double ret_value = 0;

	xSemaphoreTake( sample_mutex, portMAX_DELAY ); //Bloquea recurso compartido
	ret_value = sample.pressure;
	xSemaphoreGive( sample_mutex );				//Desbloquea recuros compartido

	return (ret_value/100);
}
