/*
 * battery_monitor.h
 *
 *  Created on: Jan 15, 2022
 *      Author: mmollon
 */

#ifndef BATTERY_MONITOR_H_
#define BATTERY_MONITOR_H_

#define ZERO_PERCENT_MV	3000.0
#define MV_RANGE	700.0

/* Initialize Monitor */
void InitBaterryMonitor(void);
/* Get Current Battery Level in percentage format (0-100) */
float getBatteryLevel(void);

#endif /* BATTERY_MONITOR_H_ */
