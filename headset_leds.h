/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_leds.h
@brief    Module responsible for managing the PIO outputs including LEDs
*/

#ifndef HEADSET_LEDS_H
#define HEADSET_LEDS_H


#include "headset_leddata.h"
#include "headset_states.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
 LedsInit

DESCRIPTION
 	Initialise the Leds data.
    
*/
void LedsInit ( LedTaskData * pTask ) ;


/****************************************************************************
NAME 
    LedsIndicateEvent

DESCRIPTION
    This function starts an event notification. It overides any prior event notification acting on the same PIO
    It also overides any state notification on the same PIO. If a state indication is happening on a different pio, 
    this will be allowed to continue.

*/
void LedsIndicateEvent ( LedTaskData * pLEDTask , headsetEvents_t pEvent ) ;


/****************************************************************************
NAME 
    LEDManagerIndicateState

DESCRIPTION
    This function kicks off a state indication. It cancels any previous state 
    indication but does not overide an event indication. 

*/
void LedsIndicateState ( LedTaskData * pLEDTask , headsetHfpState pState , headsetA2dpState pA2dpState ) ;


/****************************************************************************
NAME 
    LedsIndicateNoState

DESCRIPTION
	Remove any state indications as there are currently none to be displayed.
    
*/
void LedsIndicateNoState ( LedTaskData * pLEDTask ) ;


/****************************************************************************
NAME 
    LedsStateCanOverideDisable

DESCRIPTION
	Returns boolean indicating if the LED pattern should overide the global
    LED disable flag.
    
RETURNS
    bool
 
*/
bool LedsStateCanOverideDisable ( LedTaskData * pLEDTask , headsetHfpState pState , headsetA2dpState pA2dpState ) ;


/****************************************************************************
NAME 
    LedsEventCanOverideDisable

DESCRIPTION
	Returns boolean indicating if the LED pattern should overide the global
    LED disable flag.
RETURNS
    bool
 
*/
bool LedsEventCanOverideDisable ( LedTaskData * pLEDTask , MessageId pEvent ) ;


/****************************************************************************
NAME 
    LedActiveFiltersCanOverideDisable

DESCRIPTION
    Check if active filters disable the global LED disable flag.
    
RETURNS
 	bool
*/
bool LedActiveFiltersCanOverideDisable( LedTaskData *pLEDTask ) ;


/****************************************************************************
NAME 
    LedsCheckForFilter

DESCRIPTION
    This function checksif a filter has been configured for the given event, 
    if it has then activates / deactivates the filter. 
    
    Regardless of whether a filter has been activated or not, the event is signalled as 
    completed as we have now deaklt with it (only checked for a filter if a pattern was not
    associated.
    
*/       
void LedsCheckForFilter ( LedTaskData * pLEDTask , headsetEvents_t pEvent ) ;


/****************************************************************************
NAME 
 LedsSetLedActivity

DESCRIPTION
    Sets a Led Activity to a known state.

*/
void LedsSetLedActivity ( LEDActivity_t * pLed , IndicationType_t pType , uint16 pIndex , uint16 pDimTime) ;


/****************************************************************************
NAME 
    LedsResetAllLeds

DESCRIPTION
    Resets all led pins to off and cancels all led and state indications.

*/
void LedsResetAllLeds ( LedTaskData * pLEDTask) ;


#endif

