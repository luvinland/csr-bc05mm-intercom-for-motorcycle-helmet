/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_LEDmanager.h
@brief   Interface to LED manager functionality. 
*/

#ifndef HEADSET_LED_MANAGER_H
#define HEADSET_LED_MANAGER_H



#include "headset_private.h"
#include "headset_states.h"
#include "headset_events.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
    LEDManagerInit

DESCRIPTION
    Initialises LED manager.

*/
void LEDManagerInit ( LedTaskData * ptheLEDTask ) ;


/****************************************************************************
NAME 
    LEDManagerAddLEDStatePattern

DESCRIPTION
    Adds a state LED mapping.

*/
void LEDManagerAddLEDStatePattern (  LedTaskData * ptheLEDTask , headsetHfpState pState , headsetA2dpState pA2dpState , LEDPattern_t* pPattern ) ;  


/****************************************************************************
NAME 
    LEDManagerAddLEDFilter

DESCRIPTION
    Adds an event LED mapping.
    
*/
void LEDManagerAddLEDFilter  ( LedTaskData * ptheLEDTask , LEDFilter_t* pLedFilter ) ;  


/****************************************************************************
NAME 
    LEDManagerAddLEDEventPattern

DESCRIPTION
    Adds an event LED mapping.
    
*/
void LEDManagerAddLEDEventPattern ( LedTaskData * ptheLEDTask , headsetEvents_t pEvent , LEDPattern_t* pPattern ) ;  


/****************************************************************************
NAME 
 LEDManagerSetMicBias

DESCRIPTION
 Sets the appropriate Mic Bias pin.
    
*/
void LEDManagerSetMicBias ( hsTaskData * pApp , bool pEnable  ) ;


/****************************************************************************
NAME 
    LEDManagerIndicateEvent

DESCRIPTION
    Displays event notification.
    This function also enables / disables the event filter actions - if a normal event indication is not
    associated with the event, it checks to see if a filer is set up for the event.
    
*/
void LEDManagerIndicateEvent ( LedTaskData * pLEDTask , MessageId pEvent ) ;


/****************************************************************************
NAME	
	LEDManagerIndicateState

DESCRIPTION
	Displays state indication information.

*/
void LEDManagerIndicateState ( LedTaskData * pLEDTask , headsetHfpState pState , headsetA2dpState pA2dpState )  ;


/****************************************************************************
NAME	
	LedManagerEnableLEDS

DESCRIPTION
    Enable LED indications.
    
*/
void LedManagerEnableLEDS  ( LedTaskData * pTask ) ;


/****************************************************************************
NAME	
	LedManagerToggleLEDS

DESCRIPTION
    Toggle Enable / Disable LED indications.

*/
#if 0 /* temporarily unused function */
void LedManagerToggleLEDS  ( LedTaskData * pTask )  ;
#endif


/****************************************************************************
NAME	
	LedManagerResetLEDIndications

DESCRIPTION
    Resets the LED Indications and reverts to state indications
	Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt.
    
*/
void LedManagerResetLEDIndications ( LedTaskData * pTask ) ;


/****************************************************************************
NAME	
	LEDManagerResetStateIndNumRepeatsComplete

DESCRIPTION
    Resets the LED Number of Repeats complete for the current state indication
       This allows the time of the led indication to be reset every time an event 
       occurs.
       
*/
void LEDManagerResetStateIndNumRepeatsComplete  ( LedTaskData * pTask ) ;


/****************************************************************************
NAME 
    LMPrintPattern

DESCRIPTION
    Debug fn to output a LED pattern.
    
*/
#ifdef DEBUG_LM
void LMPrintPattern ( LEDPattern_t * pLED ) ;
#endif

#endif

