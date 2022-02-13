/*
 * ads1115.c
 *
 *  Created on: Jan 12, 2022
 *      Author: mmollon
 */

/** Default constructor, uses default I2C address.
 * @see ADS1115_DEFAULT_ADDRESS
 */

#include "ads1115.h"

#define I2CDEV_DEFAULT_READ_TIMEOUT 100

uint8_t devAddr;
uint16_t buffer[2];
bool    devMode;
uint8_t muxMode;
uint8_t pgaMode;
uint16_t configRegister;

/** Specific address constructor.
 * @param address I2C address
 * @see ADS1115_DEFAULT_ADDRESS
 * @see ADS1115_ADDRESS_ADDR_GND
 * @see ADS1115_ADDRESS_ADDR_VDD
 * @see ADS1115_ADDRESS_ADDR_SDA
 * @see ADS1115_ADDRESS_ADDR_SDL
 */
void ADS1115(uint8_t address, uint8_t gain, i2c_rtos_handle_t* handler){
	/* I2C Init */
	InitI2C(handler);
	/* Set Gain */
	devAddr = address;
	ADS1115setGain(gain);
	/* Get Some Register values */
	ADS1115getMultiplexer();
	ADS1115getMode();
}

/** Power on and prepare for general usage.
 * This device is ready to use automatically upon power-up. It defaults to
 * single-shot read mode, P0/N1 mux, 2.048v gain, 128 samples/sec, default
 * comparator with hysterysis, active-low polarity, non-latching comparator,
 * and comparater-disabled operation.
 */
void ADS1115initialize() {
	ADS1115setMultiplexer(ADS1115_MUX_P0_N1);
	ADS1115setGain(ADS1115_PGA_2P048);
	ADS1115setMode(ADS1115_MODE_SINGLESHOT);
	ADS1115setRate(ADS1115_RATE_128);
	ADS1115setComparatorMode(ADS1115_COMP_MODE_HYSTERESIS);
	ADS1115setComparatorPolarity(ADS1115_COMP_POL_ACTIVE_LOW);
	ADS1115setComparatorLatchEnabled(ADS1115_COMP_LAT_NON_LATCHING);
	ADS1115setComparatorQueueMode(ADS1115_COMP_QUE_DISABLE);
}

/** Verify the I2C connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool ADS1115testConnection() {
    return readWord(devAddr, ADS1115_RA_CONVERSION, buffer) == 1;
}

/** Poll the operational status bit until the conversion is finished
 * Retry at most 'max_retries' times
 * conversion is finished, then return true;
 * @see ADS1115_CFG_OS_BIT
 * @return True if data is available, false otherwise
 */
bool ADS1115pollConversion(uint16_t max_retries) {
  for(uint16_t i = 0; i < max_retries; i++) {
    if (ADS1115isConversionReady()) return true;
  }
  return false;
}

/** Read differential value based on current MUX configuration.
 * The default MUX setting sets the device to get the differential between the
 * AIN0 and AIN1 pins. There are 8 possible MUX settings, but if you are using
 * all four input pins as single-end voltage sensors, then the default option is
 * not what you want; instead you will need to set the MUX to compare the
 * desired AIN* pin with GND. There are shortcut methods (getConversion*) to do
 * this conveniently, but you can also do it manually with setMultiplexer()
 * followed by this method.
 *
 * In single-shot mode, this register may not have fresh data. You need to write
 * a 1 bit to the MSB of the CONFIG register to trigger a single read/conversion
 * before this will be populated with fresh data. This technique is not as
 * effortless, but it has enormous potential to save power by only running the
 * comparison circuitry when needed.
 *
 * @param triggerAndPoll If true (and only in singleshot mode) the conversion trigger
 *        will be executed and the conversion results will be polled.
 * @return 16-bit signed differential value
 * @see getConversionP0N1();
 * @see getConversionPON3();
 * @see getConversionP1N3();
 * @see getConversionP2N3();
 * @see getConversionP0GND();
 * @see getConversionP1GND();
 * @see getConversionP2GND();
 * @see getConversionP3GND);
 * @see setMultiplexer();
 * @see ADS1115_RA_CONVERSION
 * @see ADS1115_MUX_P0_N1
 * @see ADS1115_MUX_P0_N3
 * @see ADS1115_MUX_P1_N3
 * @see ADS1115_MUX_P2_N3
 * @see ADS1115_MUX_P0_NG
 * @see ADS1115_MUX_P1_NG
 * @see ADS1115_MUX_P2_NG
 * @see ADS1115_MUX_P3_NG
 */
int16_t ADS1115getConversion(bool triggerAndPoll) {
    if (triggerAndPoll && devMode == ADS1115_MODE_SINGLESHOT) {
    	ADS1115triggerConversion();
    	ADS1115pollConversion(I2CDEV_DEFAULT_READ_TIMEOUT);
    }
    readWord(devAddr, ADS1115_RA_CONVERSION, buffer);
    return buffer[0];
}

/** Get the current voltage reading
 * Read the current differential and return it multiplied
 * by the constant for the current gain.  mV is returned to
 * increase the precision of the voltage
 * @param triggerAndPoll If true (and only in singleshot mode) the conversion trigger
 *        will be executed and the conversion results will be polled.
 */
float ADS1115getMilliVolts(bool triggerAndPoll) {
  switch (pgaMode) {
    case ADS1115_PGA_6P144:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_6P144);
      break;
    case ADS1115_PGA_4P096:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_4P096);
      break;
    case ADS1115_PGA_2P048:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_2P048);
      break;
    case ADS1115_PGA_1P024:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_1P024);
      break;
    case ADS1115_PGA_0P512:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_0P512);
      break;
    case ADS1115_PGA_0P256:
    case ADS1115_PGA_0P256B:
    case ADS1115_PGA_0P256C:
      return (ADS1115getConversion(triggerAndPoll) * ADS1115_MV_0P256);
      break;
  }
}

/**
 * Return the current multiplier for the PGA setting.
 *
 * This may be directly retreived by using getMilliVolts(),
 * but this causes an independent read.  This function could
 * be used to average a number of reads from the getConversion()
 * getConversionx() functions and cut downon the number of
 * floating-point calculations needed.
 *
 */

float ADS1115getMvPerCount() {
  switch (pgaMode) {
    case ADS1115_PGA_6P144:
      return ADS1115_MV_6P144;
      break;
    case ADS1115_PGA_4P096:
      return  ADS1115_MV_4P096;
      break;
    case ADS1115_PGA_2P048:
      return ADS1115_MV_2P048;
      break;
    case ADS1115_PGA_1P024:
      return ADS1115_MV_1P024;
      break;
    case ADS1115_PGA_0P512:
      return ADS1115_MV_0P512;
      break;
    case ADS1115_PGA_0P256:
    case ADS1115_PGA_0P256B:
    case ADS1115_PGA_0P256C:
      return ADS1115_MV_0P256;
      break;
  }
}

// CONFIG register

/** Get operational status.
 * @return Current operational status (false for active conversion, true for inactive)
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_OS_BIT
 */
bool ADS1115isConversionReady() {
    readBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_OS_BIT, buffer);
    return buffer[0];
}
/** Trigger a new conversion.
 * Writing to this bit will only have effect while in power-down mode (no conversions active).
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_OS_BIT
 */
void ADS1115triggerConversion() {
    writeBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_OS_BIT, 1);
}
/** Get multiplexer connection.
 * @return Current multiplexer connection setting
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_MUX_BIT
 * @see ADS1115_CFG_MUX_LENGTH
 */
uint8_t ADS1115getMultiplexer() {
    readBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_MUX_BIT, ADS1115_CFG_MUX_LENGTH, buffer);
    muxMode = (uint8_t)buffer[0];
    return muxMode;
}
/** Set multiplexer connection.  Continous mode may fill the conversion register
 * with data before the MUX setting has taken effect.  A stop/start of the conversion
 * is done to reset the values.
 * @param mux New multiplexer connection setting
 * @see ADS1115_MUX_P0_N1
 * @see ADS1115_MUX_P0_N3
 * @see ADS1115_MUX_P1_N3
 * @see ADS1115_MUX_P2_N3
 * @see ADS1115_MUX_P0_NG
 * @see ADS1115_MUX_P1_NG
 * @see ADS1115_MUX_P2_NG
 * @see ADS1115_MUX_P3_NG
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_MUX_BIT
 * @see ADS1115_CFG_MUX_LENGTH
 */
void ADS1115setMultiplexer(uint8_t mux) {
    if (writeBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_MUX_BIT, ADS1115_CFG_MUX_LENGTH, mux)) {
        muxMode = mux;
        if (devMode == ADS1115_MODE_CONTINUOUS) {
          // Force a stop/start
        	ADS1115setMode(ADS1115_MODE_SINGLESHOT);
        	ADS1115getConversion(true);
        	ADS1115setMode(ADS1115_MODE_CONTINUOUS);
        }
    }

}
/** Get programmable gain amplifier level.
 * @return Current programmable gain amplifier level
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_PGA_BIT
 * @see ADS1115_CFG_PGA_LENGTH
 */
uint8_t ADS1115getGain() {
    readBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_PGA_BIT, ADS1115_CFG_PGA_LENGTH, buffer);
    pgaMode=(uint8_t)buffer[0];
    return pgaMode;
}
/** Set programmable gain amplifier level.
 * Continous mode may fill the conversion register
 * with data before the gain setting has taken effect.  A stop/start of the conversion
 * is done to reset the values.
 * @param gain New programmable gain amplifier level
 * @see ADS1115_PGA_6P144
 * @see ADS1115_PGA_4P096
 * @see ADS1115_PGA_2P048
 * @see ADS1115_PGA_1P024
 * @see ADS1115_PGA_0P512
 * @see ADS1115_PGA_0P256
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_PGA_BIT
 * @see ADS1115_CFG_PGA_LENGTH
 */
void ADS1115setGain(uint8_t gain) {
    if (writeBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_PGA_BIT, ADS1115_CFG_PGA_LENGTH, gain)) {
      pgaMode = gain;
         if (devMode == ADS1115_MODE_CONTINUOUS) {
            // Force a stop/start
        	 ADS1115setMode(ADS1115_MODE_SINGLESHOT);
        	 ADS1115getConversion(true);
        	 ADS1115setMode(ADS1115_MODE_CONTINUOUS);
         }
    }
}
/** Get device mode.
 * @return Current device mode
 * @see ADS1115_MODE_CONTINUOUS
 * @see ADS1115_MODE_SINGLESHOT
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_MODE_BIT
 */
bool ADS1115getMode() {
    readBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_MODE_BIT, buffer);
    devMode = buffer[0];
    return devMode;
}
/** Set device mode.
 * @param mode New device mode
 * @see ADS1115_MODE_CONTINUOUS
 * @see ADS1115_MODE_SINGLESHOT
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_MODE_BIT
 */
void ADS1115setMode(bool mode) {
    if (writeBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_MODE_BIT, mode)) {
        devMode = mode;
    }
}
/** Get data rate.
 * @return Current data rate
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_DR_BIT
 * @see ADS1115_CFG_DR_LENGTH
 */
uint8_t ADS1115getRate() {
    readBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_DR_BIT, ADS1115_CFG_DR_LENGTH, buffer);
    return (uint8_t)buffer[0];
}
/** Set data rate.
 * @param rate New data rate
 * @see ADS1115_RATE_8
 * @see ADS1115_RATE_16
 * @see ADS1115_RATE_32
 * @see ADS1115_RATE_64
 * @see ADS1115_RATE_128
 * @see ADS1115_RATE_250
 * @see ADS1115_RATE_475
 * @see ADS1115_RATE_860
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_DR_BIT
 * @see ADS1115_CFG_DR_LENGTH
 */
void ADS1115setRate(uint8_t rate) {
    writeBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_DR_BIT, ADS1115_CFG_DR_LENGTH, rate);
}
/** Get comparator mode.
 * @return Current comparator mode
 * @see ADS1115_COMP_MODE_HYSTERESIS
 * @see ADS1115_COMP_MODE_WINDOW
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_MODE_BIT
 */
bool ADS1115getComparatorMode() {
    readBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_MODE_BIT, buffer);
    return buffer[0];
}
/** Set comparator mode.
 * @param mode New comparator mode
 * @see ADS1115_COMP_MODE_HYSTERESIS
 * @see ADS1115_COMP_MODE_WINDOW
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_MODE_BIT
 */
void ADS1115setComparatorMode(bool mode) {
    writeBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_MODE_BIT, mode);
}
/** Get comparator polarity setting.
 * @return Current comparator polarity setting
 * @see ADS1115_COMP_POL_ACTIVE_LOW
 * @see ADS1115_COMP_POL_ACTIVE_HIGH
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_POL_BIT
 */
bool ADS1115getComparatorPolarity() {
    readBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_POL_BIT, buffer);
    return buffer[0];
}
/** Set comparator polarity setting.
 * @param polarity New comparator polarity setting
 * @see ADS1115_COMP_POL_ACTIVE_LOW
 * @see ADS1115_COMP_POL_ACTIVE_HIGH
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_POL_BIT
 */
void ADS1115setComparatorPolarity(bool polarity) {
    writeBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_POL_BIT, polarity);
}
/** Get comparator latch enabled value.
 * @return Current comparator latch enabled value
 * @see ADS1115_COMP_LAT_NON_LATCHING
 * @see ADS1115_COMP_LAT_LATCHING
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_LAT_BIT
 */
bool ADS1115getComparatorLatchEnabled() {
    readBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_LAT_BIT, buffer);
    return buffer[0];
}
/** Set comparator latch enabled value.
 * @param enabled New comparator latch enabled value
 * @see ADS1115_COMP_LAT_NON_LATCHING
 * @see ADS1115_COMP_LAT_LATCHING
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_LAT_BIT
 */
void ADS1115setComparatorLatchEnabled(bool enabled) {
    writeBitW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_LAT_BIT, enabled);
}
/** Get comparator queue mode.
 * @return Current comparator queue mode
 * @see ADS1115_COMP_QUE_ASSERT1
 * @see ADS1115_COMP_QUE_ASSERT2
 * @see ADS1115_COMP_QUE_ASSERT4
 * @see ADS1115_COMP_QUE_DISABLE
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_QUE_BIT
 * @see ADS1115_CFG_COMP_QUE_LENGTH
 */
uint8_t ADS1115getComparatorQueueMode() {
    readBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_QUE_BIT, ADS1115_CFG_COMP_QUE_LENGTH, buffer);
    return (uint8_t)buffer[0];
}
/** Set comparator queue mode.
 * @param mode New comparator queue mode
 * @see ADS1115_COMP_QUE_ASSERT1
 * @see ADS1115_COMP_QUE_ASSERT2
 * @see ADS1115_COMP_QUE_ASSERT4
 * @see ADS1115_COMP_QUE_DISABLE
 * @see ADS1115_RA_CONFIG
 * @see ADS1115_CFG_COMP_QUE_BIT
 * @see ADS1115_CFG_COMP_QUE_LENGTH
 */
void ADS1115setComparatorQueueMode(uint8_t mode) {
    writeBitsW(devAddr, ADS1115_RA_CONFIG, ADS1115_CFG_COMP_QUE_BIT, ADS1115_CFG_COMP_QUE_LENGTH, mode);
}

// *_THRESH registers

/** Get low threshold value.
 * @return Current low threshold value
 * @see ADS1115_RA_LO_THRESH
 */
int16_t ADS1115getLowThreshold() {
    readWord(devAddr, ADS1115_RA_LO_THRESH, buffer);
    return buffer[0];
}
/** Set low threshold value.
 * @param threshold New low threshold value
 * @see ADS1115_RA_LO_THRESH
 */
void ADS1115setLowThreshold(int16_t threshold) {
    writeWord(devAddr, ADS1115_RA_LO_THRESH, threshold);
}
/** Get high threshold value.
 * @return Current high threshold value
 * @see ADS1115_RA_HI_THRESH
 */
int16_t ADS1115getHighThreshold() {
    readWord(devAddr, ADS1115_RA_HI_THRESH, buffer);
    return buffer[0];
}
/** Set high threshold value.
 * @param threshold New high threshold value
 * @see ADS1115_RA_HI_THRESH
 */
void ADS1115setHighThreshold(int16_t threshold) {
    writeWord(devAddr, ADS1115_RA_HI_THRESH, threshold);
}

/** Configures ALERT/RDY pin as a conversion ready pin.
 *  It does this by setting the MSB of the high threshold register to '1' and the MSB
 *  of the low threshold register to '0'. COMP_POL and COMP_QUE bits will be set to '0'.
 *  Note: ALERT/RDY pin requires a pull up resistor.
 */
void ADS1115setConversionReadyPinMode() {
    writeBitW(devAddr, ADS1115_RA_HI_THRESH, 15, 1);
    writeBitW(devAddr, ADS1115_RA_LO_THRESH, 15, 0);
    ADS1115setComparatorPolarity(0);
    ADS1115setComparatorQueueMode(0);
}

// Create a mask between two bits
unsigned createMask(unsigned a, unsigned b) {
   unsigned mask = 0;
   for (unsigned i=a; i<=b; i++)
       mask |= 1 << i;
   return mask;
}

uint16_t shiftDown(uint16_t extractFrom, int places) {
  return (extractFrom >> places);
}


uint16_t getValueFromBits(uint16_t extractFrom, int high, int length) {
   int low= high-length +1;
   uint16_t mask = createMask(low ,high);
   return shiftDown(extractFrom & mask, low);
}

/** Show all the config register settings
 */
void showConfigRegister() {
    readWord(devAddr, ADS1115_RA_CONFIG, buffer);
    configRegister = buffer[0];

};

uint16_t getConfigRegister(){
	return configRegister;
}
