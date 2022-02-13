/*
 * i2c.h
 *
 *  Created on: Jan 13, 2022
 *      Author: mmollon
 */

#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"
#include "fsl_debug_console.h"

/*
*
* devAddr	I2C slave device address
* regAddr	Register regAddr to read/write from
* bitStart	First bit position to read (0-15)
* length	Number of bits to read (not more than 16)
* bitNum	Bit position to write/read (0-15)
*
*/


void InitI2C(i2c_rtos_handle_t *rtos_handler);
/* Read single word from a 16-bit device register. */
int8_t readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data); // , uint16_t timeout=I2Cdev::readTimeout
/* Write single word to a 16-bit device register. */
bool writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data);

/* Read a single bit from a 16-bit device register. */
int8_t readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data); // , uint16_t timeout=I2Cdev::readTimeout
/* Write a single bit in a 16-bit device register. */
bool writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data);

/* Read multiple bits from a 16-bit device register. */
int8_t readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t  length, uint16_t *data); // , uint16_t timeout=I2Cdev::readTimeout
/* Write multiple bits in a 16-bit device register. */
bool writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data);



#endif /* I2C_H_ */
