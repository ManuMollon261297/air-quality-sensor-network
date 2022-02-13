/*
 * sim800l.h
 *
 *  Created on: 10 Jan 2022
 *      Author: user
 */

#ifndef SIM800L_H_
#define SIM800L_H_

/* Init module*/
void Init_sim(void);

/* Get Live Time */
bool getLiveTime(uint8_t *message_buffer, uint8_t buffer_length, rtc_datetime_t* date);

/* Sends Get Request to web server */
bool sendGetRequest(char *get_request_string);

//Generates string used for senGetRequest call
int create_string(char *final_string, char *url_string, float pressure_value, float temp_value, float pm_value, float v_value, float h_value, int node_value);

#endif /* SIM800L_H_ */
