/*
 * ads1115.h
 *
 *  Created on: Jan 12, 2022
 *      Author: mmollon
 */

#ifndef ADS1115_H_
#define ADS1115_H_

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

#define ADS1115_ADDRESS_ADDR_GND    0x48 // address pin low (GND)
#define ADS1115_ADDRESS_ADDR_VDD    0x49 // address pin high (VCC)
#define ADS1115_ADDRESS_ADDR_SDA    0x4A // address pin tied to SDA pin
#define ADS1115_ADDRESS_ADDR_SCL    0x4B // address pin tied to SCL pin
#define ADS1115_DEFAULT_ADDRESS     ADS1115_ADDRESS_ADDR_GND

#define ADS1115_RA_CONVERSION       0x00
#define ADS1115_RA_CONFIG           0x01
#define ADS1115_RA_LO_THRESH        0x02
#define ADS1115_RA_HI_THRESH        0x03

#define ADS1115_CFG_OS_BIT          15
#define ADS1115_CFG_MUX_BIT         14
#define ADS1115_CFG_MUX_LENGTH      3
#define ADS1115_CFG_PGA_BIT         11
#define ADS1115_CFG_PGA_LENGTH      3
#define ADS1115_CFG_MODE_BIT        8
#define ADS1115_CFG_DR_BIT          7
#define ADS1115_CFG_DR_LENGTH       3
#define ADS1115_CFG_COMP_MODE_BIT   4
#define ADS1115_CFG_COMP_POL_BIT    3
#define ADS1115_CFG_COMP_LAT_BIT    2
#define ADS1115_CFG_COMP_QUE_BIT    1
#define ADS1115_CFG_COMP_QUE_LENGTH 2


#define ADS1115_MUX_P0_N1           0x00 // default
#define ADS1115_MUX_P0_N3           0x01
#define ADS1115_MUX_P1_N3           0x02
#define ADS1115_MUX_P2_N3           0x03
#define ADS1115_MUX_P0_NG           0x04
#define ADS1115_MUX_P1_NG           0x05
#define ADS1115_MUX_P2_NG           0x06
#define ADS1115_MUX_P3_NG           0x07

#define ADS1115_PGA_6P144           0x00
#define ADS1115_PGA_4P096           0x01
#define ADS1115_PGA_2P048           0x02 // default
#define ADS1115_PGA_1P024           0x03
#define ADS1115_PGA_0P512           0x04
#define ADS1115_PGA_0P256           0x05
#define ADS1115_PGA_0P256B          0x06
#define ADS1115_PGA_0P256C          0x07

#define ADS1115_MV_6P144            0.187500
#define ADS1115_MV_4P096            0.125000
#define ADS1115_MV_2P048            0.062500 // default
#define ADS1115_MV_1P024            0.031250
#define ADS1115_MV_0P512            0.015625
#define ADS1115_MV_0P256            0.007813
#define ADS1115_MV_0P256B           0.007813
#define ADS1115_MV_0P256C           0.007813

#define ADS1115_MODE_CONTINUOUS     0x00
#define ADS1115_MODE_SINGLESHOT     0x01 // default

#define ADS1115_RATE_8              0x00
#define ADS1115_RATE_16             0x01
#define ADS1115_RATE_32             0x02
#define ADS1115_RATE_64             0x03
#define ADS1115_RATE_128            0x04 // default
#define ADS1115_RATE_250            0x05
#define ADS1115_RATE_475            0x06
#define ADS1115_RATE_860            0x07

#define ADS1115_COMP_MODE_HYSTERESIS    0x00 // default
#define ADS1115_COMP_MODE_WINDOW        0x01

#define ADS1115_COMP_POL_ACTIVE_LOW     0x00 // default
#define ADS1115_COMP_POL_ACTIVE_HIGH    0x01

#define ADS1115_COMP_LAT_NON_LATCHING   0x00 // default
#define ADS1115_COMP_LAT_LATCHING       0x01

#define ADS1115_COMP_QUE_ASSERT1    0x00
#define ADS1115_COMP_QUE_ASSERT2    0x01
#define ADS1115_COMP_QUE_ASSERT4    0x02
#define ADS1115_COMP_QUE_DISABLE    0x03 // default

extern uint16_t configRegister;

void ADS1115(uint8_t address, uint8_t gain, i2c_rtos_handle_t* handler);

void ADS1115initialize();
bool ADS1115testConnection();

// SINGLE SHOT utilities
bool ADS1115pollConversion(uint16_t max_retries);
void ADS1115triggerConversion();

// Read the current CONVERSION register
int16_t ADS1115getConversion(bool triggerAndPoll);

// Utility
float ADS1115getMilliVolts(bool triggerAndPoll);
float ADS1115getMvPerCount();

// CONFIG register
bool ADS1115isConversionReady();
uint8_t ADS1115getMultiplexer();
void ADS1115setMultiplexer(uint8_t mux);
uint8_t ADS1115getGain();
void ADS1115setGain(uint8_t gain);
bool ADS1115getMode();
void ADS1115setMode(bool mode);
uint8_t ADS1115getRate();
void ADS1115setRate(uint8_t rate);
bool ADS1115getComparatorMode();
void ADS1115setComparatorMode(bool mode);
bool ADS1115getComparatorPolarity();
void ADS1115setComparatorPolarity(bool polarity);
bool ADS1115getComparatorLatchEnabled();
void ADS1115setComparatorLatchEnabled(bool enabled);
uint8_t ADS1115getComparatorQueueMode();
void ADS1115setComparatorQueueMode(uint8_t mode);
void ADS1115setConversionReadyPinMode();

// *_THRESH registers
int16_t ADS1115getLowThreshold();
void ADS1115setLowThreshold(int16_t threshold);
int16_t ADS1115getHighThreshold();
void ADS1115setHighThreshold(int16_t threshold);

// DEBUG
void ADS1115showConfigRegister();

#endif /* ADS1115_H_ */
