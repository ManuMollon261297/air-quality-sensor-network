/*
 * i2c.cpp
 *
 *  Created on: Jan 13, 2022
 *      Author: mmollon
 */

#include "i2c.h"
#include "peripherals.h"
#include <stdio.h>
#include <stdint.h>

#define I2C_MASTER_CLK_FREQ CLOCK_GetFreq((I2C0_CLK_SRC))
#define I2C_MASTER ((I2C_Type *)I2C_MASTER_BASE)
#define I2C_MASTER_BASE     (I2C0_BASE)
#define I2C_SLAVE_ADDR_7BIT (0x48U)
#define I2C_DATA_LENGTH	(32)     /* MAX is 256 */

i2c_rtos_handle_t *master_rtos_handle;
i2c_master_transfer_t masterXfer;
status_t status;

uint16_t reverse_bytes(uint16_t bytes);

void InitI2C(i2c_rtos_handle_t *rtos_handler){
	master_rtos_handle = rtos_handler;
}

uint16_t reverse_bytes(uint16_t bytes)
{
    uint16_t aux = 0;
    uint8_t byte;
    int i;

    for(i = 0; i < 16; i+=8)
    {
        byte = (bytes >> i) & 0xff;
        aux |= byte << (16 - 8 - i);
    }
    return aux;
}


/* Read single word from a 16-bit device register. */
int8_t readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data){

	uint8_t aux_array[2];
	uint8_t aux_int;

	/* Set up master to receive data from slave. */
	memset(&masterXfer, 0, sizeof(masterXfer));
	masterXfer.slaveAddress   = devAddr;
	masterXfer.direction      = kI2C_Read;
	masterXfer.subaddress     = regAddr;
	masterXfer.subaddressSize = 1;
	masterXfer.data           = aux_array;
	masterXfer.dataSize       = 2;
	masterXfer.flags          = kI2C_TransferDefaultFlag;

	status = I2C_RTOS_Transfer(master_rtos_handle, &masterXfer);
	if (status != kStatus_Success)
	{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	    PRINTF("I2C master: error during read transaction, %d", status);
#endif
	}
	aux_int = aux_array[0];
	aux_array[0] = aux_array[1];
	aux_array[1] = aux_int;
	*data = *((uint16_t*)aux_array);

	return status;

}

/* Write single word to a 16-bit device register. */
bool writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data){

	data = reverse_bytes(data);

	memset(&masterXfer, 0, sizeof(masterXfer));
	masterXfer.slaveAddress   = devAddr;
	masterXfer.direction      = kI2C_Write;
	masterXfer.subaddress     = regAddr;
	masterXfer.subaddressSize = 1;
	masterXfer.data           = (uint8_t*) &data;
	masterXfer.dataSize       = 2;
	masterXfer.flags          = kI2C_TransferDefaultFlag;

	status = I2C_RTOS_Transfer(master_rtos_handle, &masterXfer);
	if (status != kStatus_Success)
	{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	    PRINTF("I2C master: error during write transaction, %d", status);
#endif
	    return false;
	}
	else{
		return true;
	}

}

/* Read a single bit from a 16-bit device register. */
int8_t readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data){

	status = readWord(devAddr, regAddr, data);
	/* Get Bit */
	uint16_t mask = 1 << bitNum;
	*data = ((*data) & mask);
	*data = (*data) >> bitNum;
	return status;

}

/* Write a single bit in a 16-bit device register. */
bool writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data){

	uint16_t reg_data;
	/* Get register and create new data to write */
	status = readWord(devAddr, regAddr, &reg_data);
	data = data << bitNum;
	uint16_t mask = ~(1 << bitNum);
	reg_data = reg_data & mask;
	data = reg_data | data;
	/* Write data */
	return writeWord(devAddr, regAddr, data);

}

/* Read multiple bits from a 16-bit device register. */
int8_t readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t  length, uint16_t *data){

	status = readWord(devAddr, regAddr, data);
	/* Get Bits */
	uint16_t mask = 0b1111111111111111 >> (16-length);
	mask = mask << (bitStart - (length-1));
	*data = ((*data) & mask) >> (bitStart-(length-1));
	return status;

}

/* Write multiple bits in a 16-bit device register. */
bool writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data){

	uint16_t reg_data;
	/* Get register and create new data to write */
	status = readWord(devAddr, regAddr, &reg_data);
	data = data << (bitStart- (length-1));
	uint16_t mask = 0b1111111111111111 >> (16-length);
	mask = ~(mask << (bitStart - (length-1)));
	reg_data = reg_data & mask;
	data = reg_data | data;
	/* Write data */
	return writeWord(devAddr, regAddr, data);

}
