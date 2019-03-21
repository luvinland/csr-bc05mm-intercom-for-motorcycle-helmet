/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_charger.h
@brief   Interface to the headset charger controls. 
*/

#ifndef _HEADSET_CHARGER_H_
#define _HEADSET_CHARGER_H_

#include "headset_powermanager.h"


/****************************************************************************
  FUNCTIONS
*/


/****************************************************************************
NAME    
    chargerHandler
    
DESCRIPTION
  	This function is called to monitor the charger readings.
    
*/
void chargerHandler(hsTaskData* app);


/****************************************************************************
NAME    
    chargerInit
    
DESCRIPTION
  	This function is called by the Power Manager to initialise the battery 
	charger subsystem.
    
*/
void chargerInit(hsTaskData* theHeadset);


/****************************************************************************
NAME    
    chargerConnected
    
DESCRIPTION
  	This function is called by the Power Manager when the charger has been 
	plugged into the headset.
    
*/
void chargerConnected(hsTaskData* theHeadset);


/****************************************************************************
NAME    
    chargerDisconnected
    
DESCRIPTION
  	This function is called by the Power Manager when the charger has been 
	unplugged from the headset.
    
*/
void chargerDisconnected(hsTaskData* theHeadset);


#endif /* _HEADSET_CHARGER_H_ */
