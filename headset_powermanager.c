/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_powermanager.c
@brief    Module responsible for managing the battery monitoring and battery charging functionaility.
*/


#include "headset_battery.h"
#include "headset_charger.h"
#include "headset_powermanager.h"
#include "headset_private.h"

#include <panic.h>
#include <stdlib.h>

#ifdef DEBUG_POWER
    #define PM_DEBUG(x) DEBUG(x)             
#else
    #define PM_DEBUG(x) 
#endif

/*
	Power Status
	Stored as a global variable to prevent a memory slot from being used.
*/
/*static power_type gPowerState;*/

/*****************************************************************************/
power_type* powerManagerInit(void *block, uint16 size) 
{
	uint16 full_size = size + sizeof(power_type);
	uint16 *buffer = (uint16 *)block;
	
	buffer = realloc(buffer, full_size);

	return (power_type *)(buffer+size);
}


/*****************************************************************************/
bool powerManagerConfig(hsTaskData* theHeadset, const power_config_type* config)
{
    bool success = TRUE;
	power_type* power = theHeadset->power;
    
    if(config)
	{
		/* Store power configuration */
		power->config = *config;
        
        /* Initialise the battery sub-system */		
	    batteryInit(theHeadset);
    
        /* Initialise the battery charging sub-system */
	    chargerInit(theHeadset);
    }
    else
	{
		success = FALSE;
	}

	return success;
}


/*****************************************************************************/
void powerManagerChargerConnected(hsTaskData* theHeadset)
{
	chargerConnected(theHeadset);
}


/*****************************************************************************/
void powerManagerChargerDisconnected(hsTaskData* theHeadset)
{
	chargerDisconnected(theHeadset);
}

