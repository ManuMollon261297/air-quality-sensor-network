/*
 * sdcard.h
 *
 *  Created on: Sep 5, 2021
 *      Author: Lu
 */

#ifndef SDCARD_H_
#define SDCARD_H_



/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
//#include "../sdmmc/inc/fsl_sd.h"
#include "fsl_debug_console.h"
//#include "../fatfs/source/ff.h"
//#include "diskio.h"
//#include "fsl_sd_disk.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "board.h"
#include "sdmmc_config.h"

#include "fsl_sysmpu.h"
#include "pin_mux.h"
#include "fsl_rtc.h"
#include "clock_config.h"
#include "fsl_uart.h"
#include "data_com.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEMO_TASK_GET_SEM_BLOCK_TICKS 1U
#define DEMO_TASK_ACCESS_SDCARD_TIMES 2U
/*! @brief Task stack size. */
//#define ACCESSFILE_TASK_STACK_SIZE (1024U)
///*! @brief Task stack priority. */
//#define ACCESSFILE_TASK_PRIORITY (configMAX_PRIORITIES - 2U)

/*! @brief Task stack size. */
#define CARDDETECT_TASK_STACK_SIZE (1024U)
/*! @brief Task stack priority. */
#define CARDDETECT_TASK_PRIORITY (configMAX_PRIORITIES - 1U)

//typedef struct{
// uint8_t data;
//}measurements_t;

typedef enum{ERROR_SD=-1, NO_ERROR_SD}sd_Errors_t;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
sd_Errors_t InitSDCard(void);

void sd_reset_status(void);
sd_Errors_t CardDetect(void *pvParameters);
void DeInitSDCard(void);
void SaveDataSDCard(data_trans_t* data, rtc_datetime_t datetime);
status_t MakeFileSystem(char*);

void FileAccessTask1(void *pvParameters);

void RebootSD(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/




#endif /* SDCARD_H_ */
