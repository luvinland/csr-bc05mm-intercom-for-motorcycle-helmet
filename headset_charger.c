/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_charger.c
@brief    This file contains the battery charging functionality for NiMH batteries.
*/


/****************************************************************************
    Header files
*/

#include "headset_charger.h"
#include "headset_debug.h"
#include "headset_private.h"

#include <charger.h>
#include <pio.h>
#include <vm.h>


#ifdef DEBUG_CHARGER
#define CH_DEBUG(x) DEBUG(x)
#else
#define CH_DEBUG(x) 
#endif   


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    charger_handler
    
DESCRIPTION
  	Messages for the charger control task arrive here
    
RETURNS
    void
*/
void chargerHandler(hsTaskData* app)
{
	charger_status lcharger_status = NOT_CHARGING ;
	
    lcharger_status = ChargerStatus() ;   
	
	CH_DEBUG(("CH: monitor status = %d\n",lcharger_status));
			
	switch (lcharger_status)
    {            
    	case CHARGING : 	
        case TRICKLE_CHARGE :
        case FAST_CHARGE :	
			MessageSend(&app->task, EventFastCharge, 0);
            break ;
            
        case DISABLED_ERROR : 	
        case STANDBY :	
        case NO_POWER :	
        case NOT_CHARGING :
        default:    
			MessageSend(&app->task, EventTrickleCharge, 0);
            break ;    
    }
			
	/* Monitoring period of 1s whilst charger is plugged in */				
	MessageSendLater(&app->task, APP_CHARGER_MONITOR, 0, D_SEC(1));				
}


/*****************************************************************************/
void chargerInit(hsTaskData* theHeadset)
{
	CH_DEBUG(("CH: Init\n"));
	
	/* Assume charger disconnected at boot time */
	theHeadset->charger_state = disconnected;
}


/*****************************************************************************/
void chargerConnected(hsTaskData* theHeadset)
{
	CH_DEBUG(("CH: Connected\n"));
	
	switch(theHeadset->charger_state)
	{
	case disconnected:
		/* LiON Handled automatically */
			
		/* Fast charge state */
		theHeadset->charger_state = fast_charge;
			
		/* Enable fast charge PIO line */
		/*setChargeEnable(TRUE);*/

		/* Start monitoring onchip LiON battery charger status */
		MessageSend(&theHeadset->task, APP_CHARGER_MONITOR, 0);
		
		/* Disable deep sleep during charging */
		VmDeepSleepEnable(FALSE);
        
        break;
		
	case trickle_charge:
	case fast_charge:
	case charge_error:
		break;
	}	
}


/*****************************************************************************/
void chargerDisconnected(hsTaskData* theHeadset)
{
	CH_DEBUG(("CH: Disconnected\n"));
	
	switch(theHeadset->charger_state)
	{
	case trickle_charge:
	case fast_charge:
	case charge_error:
		/* The charger has just been removed, move to disconnected state */
		theHeadset->charger_state = disconnected;

		/* Cancel any pending LiON battery charger status monitoring messages */
		MessageCancelAll(&theHeadset->task, APP_CHARGER_MONITOR);

		/* Cancel current LED indication */
		MessageSend(getAppTask(), EventCancelLedIndication, 0);
				
		/* Disable fast charge enable PIO line */
		/*setChargeEnable(FALSE);*/

		/* Enable deep sleep */
		VmDeepSleepEnable(TRUE);

		break;
		
	case disconnected:
		break;
	}
}

