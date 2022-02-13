/*
 * uart_debug.c
 *
 *  Created on: Feb 4, 2022
 *      Author: mmollon
 */

#include "uart_debug.h"

#include "peripherals.h"
#include "fsl_uart_freertos.h"

void uart_print(char *sent_str){

	char message_buffer[150];
	int n;
	/* Send AT command */
    UART_RTOS_Send(&UART3_rtos_handle, (uint8_t *)sent_str, strlen(sent_str));
    UART_RTOS_Receive(&UART3_rtos_handle, message_buffer, 150, &n, 1000);
}
