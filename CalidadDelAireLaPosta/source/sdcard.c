/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "sdmmc/inc/fsl_sd.h"
#include "fsl_debug_console.h"
#include "fatfs/source/ff.h"
#include "fatfs/source/diskio.h"
#include "fatfs/source/fsl_sd_disk/fsl_sd_disk.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "board.h"
#include "sdmmc_config.h"

#include "fsl_sysmpu.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_uart.h"

#include "uart_debug.h"

#ifdef LED_VISUALIZATION

static void LED_Blink(int times){
	vTaskDelay(pdMS_TO_TICKS(1000));
	for(int i=0; i<times; i++){
		LED_RED_ON();
		vTaskDelay(pdMS_TO_TICKS(500));
		LED_RED_OFF();
	}
	vTaskDelay(pdMS_TO_TICKS(1000));
}

#endif

#include "sdcard.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_TASK_GET_SEM_BLOCK_TICKS 1U
#define DEMO_TASK_ACCESS_SDCARD_TIMES 2U
/*! @brief Task stack size. */
#define ACCESSFILE_TASK_STACK_SIZE (1024U)
/*! @brief Task stack priority. */
#define ACCESSFILE_TASK_PRIORITY (configMAX_PRIORITIES - 2U)

/*! @brief Task stack size. */
#define CARDDETECT_TASK_STACK_SIZE (1024U)
/*! @brief Task stack priority. */
#define CARDDETECT_TASK_PRIORITY (configMAX_PRIORITIES - 1U)

#define UART_INIT_DEBUG_CONSOLE_PERIPHERAL


static char* currDirectoryPath = "/saves/";
static char fileName[20] = "";
//static char* currDirectoryFile = "";
static uint8_t currentDay = 0;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief SD card access task 1.
 *
 * @param pvParameters Task parameter.
 */
//static void FileAccessTask1(void *pvParameters);

/*!
 * @brief SD card detect task.
 *
 * @param pvParameters Task parameter.
 */
//static void CardDetectTask(void *pvParameters);

/*!
 * @brief call back function for SD card detect.
 *
 * @param isInserted  true,  indicate the card is insert.
 *                    false, indicate the card is remove.
 * @param userData
 */
static void SDCARD_DetectCallBack(bool isInserted, void *userData);

void write(data_trans_t* data, rtc_datetime_t datetime, char* name);
/*!
 * @brief make filesystem.
 */
//static status_t MakeFileSystem(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS g_fileSystem; /* File system object */
static FIL g_fileObject;   /* File object */

static uint32_t s_taskSleepTicks = portMAX_DELAY;
/*! @brief SD card detect flag  */
static volatile bool s_cardInserted     = false;
static volatile bool s_cardInsertStatus = false;
/*! @brief Card semaphore  */
static SemaphoreHandle_t s_fileAccessSemaphore = NULL;
static SemaphoreHandle_t s_CardDetectSemaphore = NULL;
/*******************************************************************************
 * Code
 ******************************************************************************/

void sd_reset_status(void){
	s_cardInserted = !s_cardInsertStatus;
}

static void SDCARD_DetectCallBack(bool isInserted, void *userData){
    s_cardInsertStatus = isInserted;
    xSemaphoreGiveFromISR(s_CardDetectSemaphore, NULL);
}

sd_Errors_t InitSDCard(void){

	sd_Errors_t initError;

	SYSMPU_Enable(SYSMPU, false);
	initError = CardDetect(NULL);
	return initError;
}

void DeInitSDCard(void){

	SD_HostDeinit(&g_sd);
	SD_SetCardPower(&g_sd, false);

#ifdef LED_VISUALIZATION
	LED_Blink(5);
#endif

	vSemaphoreDelete(s_fileAccessSemaphore);
	vSemaphoreDelete(s_CardDetectSemaphore);
	s_fileAccessSemaphore = NULL;
	s_CardDetectSemaphore = NULL;
	s_cardInserted     = false;
	s_cardInsertStatus = false;
}

sd_Errors_t CardDetect(void *pvParameters){

	sd_Errors_t error;
	if(s_fileAccessSemaphore == NULL){
		s_fileAccessSemaphore = xSemaphoreCreateBinary();
	}
	if(s_CardDetectSemaphore == NULL){
		s_CardDetectSemaphore = xSemaphoreCreateBinary();
	}

    BOARD_SD_Config(&g_sd, SDCARD_DetectCallBack, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

    /* SD host init function */

    if (SD_HostInit(&g_sd) == kStatus_Success){
		/* take card detect semaphore */
		if (xSemaphoreTake(s_CardDetectSemaphore, portMAX_DELAY) == pdTRUE){
			if (s_cardInserted != s_cardInsertStatus){
				s_cardInserted = s_cardInsertStatus;

				/* power off card */
				SD_SetCardPower(&g_sd, false);

				if (s_cardInserted){
					error = NO_ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
					PRINTF("\r\nCard inserted.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("\r\nCard inserted.\r\n");
#endif
					/* power on the card */
					SD_SetCardPower(&g_sd, true);
					/* make file system */
					if (MakeFileSystem(currDirectoryPath) != kStatus_Success){
						error = ERROR_SD;
					}
					xSemaphoreGive(s_fileAccessSemaphore);
					s_taskSleepTicks = DEMO_TASK_GET_SEM_BLOCK_TICKS;
				}
			}

			if (!s_cardInserted){
				error = ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
				PRINTF("\r\nPlease insert a card into board.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("\r\nPlease insert a card into board.\r\n");
#endif
			}
		}
    }else{
    	error = ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("\r\nSD host init fail\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("\r\nSD host init fail\r\n");
#endif
    }

    return error;
}

status_t MakeFileSystem(char* path)
{
    FRESULT error;
    const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};
    BYTE work[FF_MAX_SS];

    if (f_mount(&g_fileSystem, driverNumberBuffer, 0U)){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("Mount volume failed.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Mount volume failed.\r\n");
#endif
        return kStatus_Fail;
    }

#if (FF_FS_RPATH >= 2U)
    error = f_chdrive((char const *)&driverNumberBuffer[0U]);
    if (error){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("Change drive failed.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Change drive failed.\r\n");
#endif
        return kStatus_Fail;
    }
#endif

#if FF_USE_MKFS
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    PRINTF("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
#endif
    if (f_mkfs(driverNumberBuffer, 0, work, sizeof work)){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("Make file system failed.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Make file system failed.\r\n");
#endif
        return kStatus_Fail;
    }
#endif /* FF_USE_MKFS */
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    PRINTF("\r\nCreate directory......\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("\r\nCreate directory......\r\n");
#endif
    error = f_mkdir(_T("/saves"));
    if (error){
        if (error == FR_EXIST){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
            PRINTF("Directory exists.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Directory exists.\r\n");
#endif
        }else{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
            PRINTF("Make directory failed.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Make directory failed.\r\n");
#endif
            return kStatus_Fail;
        }
    }

    return kStatus_Success;
}


void SaveDataSDCard(data_trans_t* data, rtc_datetime_t datetime){
	if(currentDay == 0 || datetime.day != currentDay){
		//tengo que inicializar la variable o crear nuevo archivo
		currentDay = datetime.day;
		sprintf(fileName, "%04hd%02hd%02hd.txt", datetime.year, datetime.month, datetime.day);
		write(data, datetime, fileName);
	}else{
		//escribo sobre el mismo archivo
		write(data, datetime, fileName);
	}


}

void write(data_trans_t* data, rtc_datetime_t date, char fileName[20]){
	uint32_t bytesWritten   = 0U;
	uint32_t writeTimes = 1U;
	uint8_t error;

	char buffer[100]={0};
	snprintf(buffer, sizeof(buffer), "%02hd:%02hd:%02hd PM10:%0.1f [ug/m3] T:%0.2f [°C] H:%0.2f [RH%%] P:%0.2f [hPa] B:%0.2f [%%] \r\n",
			date.hour, date.minute, date.second, data->PM10Avg, data->tempAvg, data->humAvg, data->pressAvg, data->battery_lvlAvg);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	PRINTF("data: %s\n",buffer);
#endif
	char filePath[strlen(currDirectoryPath)+strlen(fileName)+1];
	snprintf(filePath, sizeof(filePath), "%s%s", currDirectoryPath, fileName);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	PRINTF("path: %s\n",filePath);
#endif
	/* trying to take the file access semphore */
	if (xSemaphoreTake(s_fileAccessSemaphore, s_taskSleepTicks) == pdTRUE){
		error = f_open(&g_fileObject, _T(filePath), FA_OPEN_APPEND | FA_WRITE);
		if (error){
			if (error == FR_EXIST){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
				PRINTF("File exists. Error: %d\r\n", error);
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("File exists. Error\r\n");
#endif
			}else if (error == FR_NO_FILE){ /* if file not exist, creat a new file */
				if (f_open(&g_fileObject, _T(filePath), (FA_WRITE | FA_CREATE_NEW)) != FR_OK){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
					PRINTF("Create file failed. Error: %d\r\n", error);
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Create file failed. Error\r\n");
#endif
				}
			}else{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
				PRINTF("Open file failed. Error: %d\r\n", error);
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Open file failed. Error\r\n");
#endif
			}
		}
//		/* write append */
//		if (f_lseek(&g_fileObject, g_fileObject.obj.objsize) != FR_OK){
//			PRINTF("lseek file failed. Error: %d\r\n");
//		}

		error = f_write(&g_fileObject, buffer, strlen(buffer), &bytesWritten);
		if ((error) || (bytesWritten != strlen(buffer))){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
			PRINTF("Write file failed. Error: %d\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("Write file failed. Error\r\n");
#endif
		}
		f_close(&g_fileObject);

		xSemaphoreGive(s_fileAccessSemaphore);
		if (++writeTimes > DEMO_TASK_ACCESS_SDCARD_TIMES){
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
			PRINTF("TASK1: finished.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("TASK1: finished.\r\n");
#endif
		}
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("TASK1: write file success.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("TASK1: write file success.\r\n");
#endif

		vTaskDelay(1U);
	}else{
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		PRINTF("TASK1: file access is blocking.\r\n");
#endif
#ifdef UART_INIT_DEBUG_CONSOLE_PERIPHERAL
		uart_print("TASK1: file access is blocking.\r\n");
#endif
	}
}

sd_Errors_t CardResume(void *pvParameters){
	sd_Errors_t error;
	if(s_fileAccessSemaphore == NULL){
		s_fileAccessSemaphore = xSemaphoreCreateBinary();
	}
	if(s_CardDetectSemaphore == NULL){
		s_CardDetectSemaphore = xSemaphoreCreateBinary();
	}

    BOARD_SD_Config(&g_sd, SDCARD_DetectCallBack, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

    /* SD host init function */

    if (SD_HostInit(&g_sd) == kStatus_Success){
		/* take card detect semaphore */
		if (xSemaphoreTake(s_CardDetectSemaphore, portMAX_DELAY) == pdTRUE){
			if (s_cardInserted != s_cardInsertStatus){
				s_cardInserted = s_cardInsertStatus;

				/* power off card */
				SD_SetCardPower(&g_sd, false);

				if (s_cardInserted){
					error = NO_ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
					PRINTF("\r\nCard inserted.\r\n");
#endif
					/* power on the card */
					SD_SetCardPower(&g_sd, true);
					xSemaphoreGive(s_fileAccessSemaphore);
					s_taskSleepTicks = DEMO_TASK_GET_SEM_BLOCK_TICKS;
				}
			}

			if (!s_cardInserted){
				error = ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
				PRINTF("\r\nPlease insert a card into board.\r\n");
#endif
			}
		}
    }else{
    	error = ERROR_SD;
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("\r\nSD host init fail\r\n");
#endif
    }

    return error;
}
void RebootSD(void){
	DeInitSDCard();
	sd_Errors_t initError;
	SYSMPU_Enable(SYSMPU, false);
	initError = CardResume(NULL);
#ifdef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
        PRINTF("%d", initError);
#endif
}

