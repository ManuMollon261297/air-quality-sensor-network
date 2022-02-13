/*
 * power_module.h
 *
 *  Created on: 29 Jan 2022
 *      Author: user
 */
#include <stdbool.h>

#ifndef POWER_MODULE_H_
#define POWER_MODULE_H_

typedef enum {RUN, SLEEP, DEEP_SLEEP}power_modes_t;

/* Initializes hardware related to power configuration of the board */
void BOARD_InitBootPowerModule(void);

/* Switches board to selected power mode. Available modes are those shown in power_modes_t */
bool Enter_power_mode(power_modes_t);

#endif /* POWER_MODULE_H_ */
