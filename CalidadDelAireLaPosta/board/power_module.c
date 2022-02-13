/*
 * power_module.c
 *
 *  Created on: 29 Jan 2022
 *      Author: user
 */

#include "power_module.h"
#include "fsl_smc.h"

/********************************************************************
 * 					DECLARACION DE FUNCIONES						*
 ********************************************************************/

bool EnterRunMode(smc_power_state_t);
bool EnterSleepMode(smc_power_state_t);
bool EnterDeepSleepMode(smc_power_state_t);

/********************************************************************
 * 						FUNCIONES DEL HEADER						*
 ********************************************************************/

void BOARD_InitBootPowerModule(void){
	SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
}

bool Enter_power_mode(power_modes_t mode){
	smc_power_state_t current_state = SMC_GetPowerModeState(SMC);
	bool successful = false;
	switch(mode){
	case RUN:
		successful = EnterRunMode(current_state);
		break;
	case SLEEP:
		//successful = EnterSleepMode(current_state);
		break;
	case DEEP_SLEEP:
		successful = EnterDeepSleepMode(current_state);
		break;
	}
	return successful;
}

/****************************************************************************
 * 							FUNCIONES LOCALES								*
 ****************************************************************************/
bool EnterRunMode(smc_power_state_t current_state){
	status_t status;
	if(current_state == kSMC_PowerStateRun){
		return true;
	}
	status = SMC_SetPowerModeRun(SMC);
	if(status != kStatus_Success){
		return false;
	}
	if(SMC_GetPowerModeState(SMC)!=kSMC_PowerStateRun){
		return false;
	}
	return true;
}

bool EnterDeepSleepMode(smc_power_state_t current_state){
	bool succesful = false;
	status_t status;
	if(current_state == kSMC_PowerStateStop){
		return true;
	}
	else if(current_state != kSMC_PowerStateRun){
		succesful = EnterRunMode(current_state);
		if(!succesful){
			return false;
		}
	}
	SMC_PreEnterStopModes();
	status = SMC_SetPowerModeStop(SMC, kSMC_PartialStop);
	if(status != kStatus_Success){
		return false;
	}
	SMC_PostExitStopModes();
	return true;
}


