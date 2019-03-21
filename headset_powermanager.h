/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_powermanager.h
@brief    Interface to module responsible for managing the battery monitoring and battery charging functionaility.
*/
#ifndef HEADSET_POWER_MANAGER_H
#define HEADSET_POWER_MANAGER_H


#include "headset_private.h"


/****************************************************************************
NAME    
    powerManagerInit
    
DESCRIPTION
  	Initialise power management
    
RETURNS
    void
*/
power_type* powerManagerInit(void *block, uint16 size);

/****************************************************************************
NAME    
    powerManagerConfig
    
DESCRIPTION
  	Configure power management
    
RETURNS
    void
*/
bool powerManagerConfig(hsTaskData* theHeadset, const power_config_type* config);


/****************************************************************************
NAME    
    powerManagerChargerConnected
    
DESCRIPTION
  	This function is called when the charger is plugged into the headset
    
RETURNS
    void
*/
void powerManagerChargerConnected(hsTaskData* theHeadset);


/****************************************************************************
NAME    
    powerManagerChargerDisconnected
    
DESCRIPTION
  	This function is called when the charger is unplugged from the headset
    
RETURNS
    void
*/
void powerManagerChargerDisconnected(hsTaskData* theHeadset);


#endif 
