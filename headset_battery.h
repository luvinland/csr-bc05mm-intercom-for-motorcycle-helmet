/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_battery.h
@brief  Interface to battery functionality.
*/

#ifndef _HEADSET_BATTERY_H_
#define _HEADSET_BATTERY_H_


#include "headset_powermanager.h"
#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    batteryInit
    
DESCRIPTION
  	This function will initialise the battery sub-system.  The sub-system 
	manages the reading and calulation of the battery voltage and temperature.
    
*/
void batteryInit(hsTaskData* theHeadset);


/****************************************************************************
NAME    
    batteryRead
    
DESCRIPTION
  	Call this function to start immediate battery reading.
    
*/
void batteryRead(hsTaskData* theHeadset);


#endif /* _HEADSET_BATTERY_H_ */
