/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "fsl_common.h"
#include "fsl_rtc.h"
#include "fsl_dspi.h"
#include "fsl_dspi_freertos.h"
#include "fsl_uart.h"
#include "fsl_uart_freertos.h"
#include "fsl_clock.h"
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/
/* Definitions for BOARD_InitPeripherals functional group */
/* Definition of peripheral ID */
#define RTC_PERIPHERAL RTC
/* RTC interrupt vector ID (number). */
#define RTC_COMMON_IRQN RTC_IRQn
/* RTC interrupt vector priority. */
#define RTC_COMMON_IRQ_PRIORITY 2
/* RTC interrupt handler identifier. */
#define RTC_COMMON_IRQHANDLER RTC_IRQHandler
/* BOARD_InitPeripherals defines for SPI0 */
/* Definition of peripheral ID */
#define SPI0_PERIPHERAL SPI0
/* Definition of the clock source */
#define SPI0_CLOCK_SOURCE DSPI0_CLK_SRC
/* Definition of the clock source frequency */
#define SPI0_CLK_FREQ CLOCK_GetFreq(SPI0_CLOCK_SOURCE)
/* SPI0 interrupt vector ID (number). */
#define SPI0_IRQN SPI0_IRQn
/* Definition of peripheral ID */
#define UART3_PERIPHERAL UART3
/* Definition of the clock source frequency */
#define UART3_CLOCK_SOURCE CLOCK_GetFreq(UART3_CLK_SRC)
/* Definition of the backround buffer size */
#define UART3_BACKGROUND_BUFFER_SIZE 100
/* UART3 interrupt vector ID (number). */
#define UART3_SERIAL_RX_TX_IRQN UART3_RX_TX_IRQn
/* UART3 interrupt vector ID (number). */
#define UART3_SERIAL_ERROR_IRQN UART3_ERR_IRQn
/* Definition of peripheral ID */
#define UART4_PERIPHERAL UART4
/* Definition of the clock source frequency */
#define UART4_CLOCK_SOURCE CLOCK_GetFreq(UART4_CLK_SRC)
/* Definition of the backround buffer size */
#define UART4_BACKGROUND_BUFFER_SIZE 100
/* UART4 interrupt vector ID (number). */
#define UART4_SERIAL_RX_TX_IRQN UART4_RX_TX_IRQn
/* UART4 interrupt vector ID (number). */
#define UART4_SERIAL_ERROR_IRQN UART4_ERR_IRQn
/* BOARD_InitPeripherals defines for I2C0 */
/* Definition of peripheral ID */
#define I2C0_PERIPHERAL I2C0
/* Definition of the clock source */
#define I2C0_CLOCK_SOURCE I2C0_CLK_SRC
/* Definition of the clock source frequency */
#define I2C0_CLK_FREQ CLOCK_GetFreq(I2C0_CLOCK_SOURCE)
/* I2C0 interrupt vector ID (number). */
#define I2C0_IRQN I2C0_IRQn

/***********************************************************************************************************************
 * Global variables
 **********************************************************************************************************************/
/* RTC configuration */
extern const rtc_config_t RTC_config;
/* Date and time structure */
extern rtc_datetime_t RTC_dateTimeStruct;
extern dspi_rtos_handle_t SPI0_rtosHandle;
extern const dspi_master_config_t SPI0_config;
extern uart_rtos_handle_t UART3_rtos_handle;
extern uart_handle_t UART3_uart_handle;
extern uart_rtos_config_t UART3_rtos_config;
extern uart_rtos_handle_t UART4_rtos_handle;
extern uart_handle_t UART4_uart_handle;
extern uart_rtos_config_t UART4_rtos_config;
extern i2c_rtos_handle_t I2C0_rtosHandle;
extern const i2c_master_config_t I2C0_config;

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/

void BOARD_InitPeripherals(void);

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void);

void RebootUart(void);

void RebootAll(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PERIPHERALS_H_ */
