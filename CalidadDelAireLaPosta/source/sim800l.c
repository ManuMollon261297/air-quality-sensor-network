/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "peripherals.h"
#include "board.h"
#include "fsl_uart_freertos.h"
#include "fsl_uart.h"
#include "fsl_gpio.h"

/* For implementation of ftoa() */
#include <math.h>
#include <stdio.h>

#include "sim800l.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* UART instance and clock */
#define UART_RX_TX_IRQn UART3_RX_TX_IRQn
/* Task priorities. */
#define uart_task_PRIORITY (configMAX_PRIORITIES - 1)

#define BUFFER_LEN 100
#define INTERNAL_UART_MS_MAX_WAIT	1000
#define MAX_NETWORK_RETRIES 25

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*  */
bool getNetworkStatus(void);
/*  */
void send_at_cmd(char *sent_str);
/*  */
void receive_cmd(uint8_t *message_buffer, uint32_t max_ms_wait);
/* */
void ftoa(float n, char* res, int afterpoint);
/* */
int intToStr(int x, char str[], int d);
/* */
void reverse(char* str, int len);
/* */
bool parse_response(uint8_t *message_buffer, char *response);
/* */
bool shut_sim_connection(void);
/* */
void reset_sim(void);
/* */
void power_on_sim(void);
/* */
void power_off_sim(void);
/* */
void set_datetime(char *s, rtc_datetime_t*date);


/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t message_buffer[BUFFER_LEN];

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Application entry point.
 */


char *apn = "internet.movil";
char *apnusername = "internet";
char *apnpassword = "internet";
char *useragent = "TUENTI_WEB";
char *ok_reply = "OK";
size_t n = 0;

void Init_sim(void){
	power_off_sim();
}


void power_on_sim(void){
	//GPIO PB20 IN HIGH IMPEDANCE
	gpio_pin_config_t gpio_config;
	gpio_config.pinDirection = 0; //INPUT DIRECTION FOR HIGH IMPEDANCE
	GPIO_PinInit(BOARD_RF_WIFI_CE_GPIO, BOARD_RF_WIFI_CE_PIN, &gpio_config);

}
void power_off_sim(void){
	gpio_pin_config_t gpio_config;
	gpio_config.pinDirection = 1; //INPUT DIRECTION FOR HIGH IMPEDANCE
	gpio_config.outputLogic = 0;
	GPIO_PinInit(BOARD_RF_WIFI_CE_GPIO, BOARD_RF_WIFI_CE_PIN, &gpio_config);
}

void receive_cmd(uint8_t *message_buffer, uint32_t max_ms_wait){
	int status = UART_RTOS_Receive(&UART3_rtos_handle, message_buffer, BUFFER_LEN, &n, max_ms_wait);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	PRINTF("String: %s\n", message_buffer);
#endif

}

bool getNetworkStatus(void){

	bool status_ok;
	char *connection_response = "CREG: 0,1";
	/* Get Network Status */
	char *cmd_get_network_status = "AT+CREG?\n";
	send_at_cmd(cmd_get_network_status); // Expects OK reply if not send CIPSHUT
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, connection_response);
	memset(message_buffer, 0, BUFFER_LEN);

	return status_ok;
}

bool sendGetRequest(char *get_request_string){

	bool status_ok;
	power_on_sim();
	uint8_t retries = 0;
	power_on_sim();
	/* Test connectivity */
	do{
		status_ok = getNetworkStatus();
		retries++;
	}while((!status_ok) && (retries < MAX_NETWORK_RETRIES));

	if(!status_ok){
		reset_sim();
		return false;
	}

	char *ok_response = "OK";
	char *ip_response = ".";
	char *connect_response = "CONNECT OK";
	char *send_response = "SEND OK";
	char *close_response = "CLOSED";

	/* Set Single IP connection */
	char *cmd_set_mux = "AT+CIPMUX=0\n";
	send_at_cmd(cmd_set_mux); // Expects OK reply if not send CIPSHUT
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, ok_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}


	/* Set APN */
	char *cmd_set_apn = "AT+CSTT=\"internet.movil\",\"internet\",\"internet\"\n";
	send_at_cmd(cmd_set_apn); // Expects OK reply  if not start again
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, ok_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Bring Up Wireless Connection with GPRS */
	char *cmd_set_gprs_conn = "AT+CIICR\n";
	send_at_cmd(cmd_set_gprs_conn); // Expects OK reply if not start again
	receive_cmd(message_buffer, 5000);
	status_ok = parse_response(message_buffer, ok_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Get local IP Address */
	char *cmd_get_ip = "AT+CIFSR\n";
	send_at_cmd(cmd_get_ip); // Expects IP return
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, ip_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Set Prompt of ‘>’ When Module Sends Data */
	char *cmd_set_prompt = "AT+CIPSPRT=0\n";
	send_at_cmd(cmd_set_prompt); // Expects OK reply
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, ok_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Start Up TCP Connection */
	char *cmd_start_tcp_conn = "AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"\n";
	send_at_cmd(cmd_start_tcp_conn); // Expects OK reply and later a CONNECT OK
	receive_cmd(message_buffer, 15000);
	status_ok = parse_response(message_buffer, ok_response);
	status_ok = parse_response(message_buffer, connect_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Send Data Through TCP Connection */
	char *cmd_send_data = "AT+CIPSEND\n";
	send_at_cmd(cmd_send_data); // Doesnt expect response, wait for long time
	receive_cmd(message_buffer, 10000);
	memset(message_buffer, 0, BUFFER_LEN);


	/* Send Data Through TCP Connection */
	char *cmd_enter = "\n";
	send_at_cmd(get_request_string);
	send_at_cmd(cmd_enter); // Doesn't expect response
	receive_cmd(message_buffer, 3000);
	memset(message_buffer, 0, BUFFER_LEN);

	/* Send end of command */
	char cmd_eoc = 26;
	send_at_cmd(&cmd_eoc);
	send_at_cmd(cmd_enter); // Expects SEND OK a number and CLOSED
	receive_cmd(message_buffer, 10000);
	status_ok = parse_response(message_buffer, send_response) && parse_response(message_buffer, close_response);
	memset(message_buffer, 0, BUFFER_LEN);
	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Close Connection */
	status_ok = shut_sim_connection();
	if(!status_ok){
		reset_sim();
		return false;
	}
	power_off_sim();
	return true;

}

bool getLiveTime(uint8_t *message_buffer, uint8_t buffer_length, rtc_datetime_t* date){

	bool status_ok;
	char *ok_response = "OK";
	uint8_t retries = 0;
	power_on_sim();
	/* Test connectivity */
	do{
		status_ok = getNetworkStatus();
		retries++;
	}while((!status_ok) && (retries < MAX_NETWORK_RETRIES));

	if(!status_ok){
		reset_sim();
		return false;
	}

	/* Get current time */
	char *cmd_get_time = "AT+CCLK?\n";
	send_at_cmd(cmd_get_time); // Expects OK reply if not send CIPSHUT
	receive_cmd(message_buffer, INTERNAL_UART_MS_MAX_WAIT);
	status_ok = parse_response(message_buffer, ok_response);
	if(!status_ok){
		reset_sim();
		return false;
	}
	power_off_sim();
	char *s;
 	s = (char*)message_buffer+18;
	set_datetime(s, date);
	return true;

}

void set_datetime(char *s, rtc_datetime_t*date){
	char aux_day[3];
	char aux_hour[3];
	char aux_minute[3];
	char aux_month[3];
	char aux_second[3];
	char aux_year[5];

	aux_day[0] = s[6];
	aux_day[1] = s[7];
	aux_day[2] = 0;

	aux_year[0] = '2';
	aux_year[1] = '0';
	aux_year[2] = s[0];
	aux_year[3] = s[1];
	aux_year[4] = 0;

	aux_month[0] = s[3];
	aux_month[1] = s[4];
	aux_month[2] = 0;

	aux_hour[0] = s[9];
	aux_hour[1] = s[10];
	aux_hour[2] = 0;

	aux_minute[0] = s[12];
	aux_minute[1] = s[13];
	aux_minute[2] = 0;

	aux_second[0] = s[15];
	aux_second[1] = s[16];
	aux_second[2] = 0;

	date->day = atoi(aux_day);
	date->year = atoi(aux_year);
	date->month = atoi(aux_month);
	date->hour = atoi(aux_hour);
	date->minute = atoi(aux_minute);
	date->second = atoi(aux_second);
}

void reset_sim(void){
	/* Close Connection */
	bool status_ok = shut_sim_connection();
	power_off_sim();
	vTaskDelay(pdMS_TO_TICKS(1000));
}

bool shut_sim_connection(void){
	char *shut_response = "SHUT OK";
	char *cmd_disc_sockets = "AT+CIPSHUT\n";
	send_at_cmd(cmd_disc_sockets); // Expects SHUT OK reply
	receive_cmd(message_buffer, 5000);
	return parse_response(message_buffer, shut_response);
}

bool parse_response(uint8_t *message_buffer, char *response){

	if(strstr((char*)message_buffer, response) != NULL) {
	    return true;
	}
	else{
		return false;
	}
}

// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}


// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

int create_string(char *final_string, char *url_string, float pressure_value, float temp_value, float pm_value, float v_value, float h_value, int node_value){

	char str_pressure_value[10];
	char str_temp_value[10];
	char str_pm_value[10];
	char str_v_value[10];
	char str_h_value[10];
	char str_node_value[10];

	ftoa(pressure_value, str_pressure_value, 2);
	ftoa(temp_value, str_temp_value, 2);
	ftoa(pm_value, str_pm_value, 2);
	ftoa(v_value, str_v_value, 2);
	ftoa(h_value, str_h_value, 2);
	sprintf(str_node_value, "%d", node_value);

    strcat(final_string, url_string);
    strcat(final_string, "&field1=");
    strcat(final_string, str_pm_value);
    strcat(final_string, "&field2=");
    strcat(final_string, str_temp_value);
    strcat(final_string, "&field3=");
    strcat(final_string, str_h_value);
    strcat(final_string, "&field4=");
    strcat(final_string, str_pressure_value);
    strcat(final_string, "&field5=");
    strcat(final_string, str_v_value);
    strcat(final_string, "&field6=");
    strcat(final_string, str_node_value);

    return strlen(final_string);

}


void send_at_cmd(char *sent_str){

	/* Send AT command */
    UART_RTOS_Send(&UART3_rtos_handle, (uint8_t *)sent_str, strlen(sent_str));

}
