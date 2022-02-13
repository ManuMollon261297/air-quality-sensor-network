/*
 * rtc.h
 *
 *  Created on: Sep 26, 2021
 *      Author: Lu
 */

#ifndef RTC_H_
#define RTC_H_

void InitRTC(void);
status_t RTCSetNextAlarm(RTC_Type *base, uint32_t secondsToNextAlarm);

#endif /* RTC_H_ */
