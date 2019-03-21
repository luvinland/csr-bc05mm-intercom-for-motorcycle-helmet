/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005
*/

/*!
@file    headset_battery.c
@brief    This file contains the battery specific functionality.  This includes battery voltage and temperature monitoring

@note
    Certain assumptions have been made on the assignment of the analog input signals:
@par
    AIO_0 is connected to VBAT via a potential divider
@par
    VBAT = AIO_0 * (Ra \ (Ra + Rb))
@par
    Where the divisor ratio of Ra and Rb is configured from persistent store
@par
    AIO_1 is connected to a thermister used to measure the battery temperature 
    fed from an PIO output through a 10k resistor
*/


/****************************************************************************
    Header files
*/

#include "headset_battery.h"
#include "headset_debug.h"
#include "headset_private.h"
#include "headset_statemanager.h"

/*#include <pio.h>*/
#include <battery.h>




#ifdef DEBUG_BAT
#define BT_DEBUG(x) DEBUG(x)
#else
#define BT_DEBUG(x) 
#endif   


/* Local static functions */
static void batteryNormal(Task pTask , hsTaskData* theHeadset );
static void batteryLow(Task pTask, hsTaskData* theHeadset);
static void batteryShutdown(Task pTask, hsTaskData* theHeadset);
static void aio_handler(Task task, MessageId id, Message message);
static void handleBatteryVoltageReading(hsTaskData* theHeadset, uint32 reading , Task pTask);


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    batteryNormal
    
DESCRIPTION
  	Called when the battery voltage is detected to be in a Normal state
    
RETURNS
    void
*/
static void batteryNormal(Task pTask, hsTaskData* theHeadset )
{
    BT_DEBUG(("PM Normal\n"));

    if (theHeadset->charger_state != disconnected) 
    	MessageSend(getAppTask(), EventOkBattery, 0);


}


/****************************************************************************
NAME    
    batteryLow
    
DESCRIPTION
  	Called when the battery voltage is detected to be in a Low state
    
RETURNS
    void
*/
static void batteryLow(Task pTask , hsTaskData* theHeadset)
{
   
        /*we only want low battery reminders if the headset is ON and not charging*/
    if ( (theHeadset->charger_state == disconnected) && (stateManagerGetHfpState() != headsetPoweringOn) )
	{    
    	MessageSend(getAppTask(), EventLowBattery, 0); 

    }
}


/****************************************************************************
NAME    
    batteryShutdown
    
DESCRIPTION
  	Called when the battery voltage is detected to be in a Shutdown state
    
RETURNS
    void
*/
static void batteryShutdown(Task pTask, hsTaskData* theHeadset)
{
	BT_DEBUG(("PM Shutdown\n"));
        /*we only want low battery reminders if the headset is ON and not charging*/
    if ( stateManagerGetHfpState() != headsetPoweringOn )
	{    
    	theHeadset->buttons_locked = FALSE;
    	MessageSend(getAppTask(), EventLowBattery, 0); 
		MessageSend(getAppTask(), EventPowerOff, 0);
    	
	}	
}


/****************************************************************************
NAME    
    handleBatteryVoltageReading
    
DESCRIPTION
  	Calculate current battery voltage and check to determine if the level
	has fallen below either the low or critical thresholds.  If the voltage
	has fallen below the low threshold generate a low battery system event.
	If the level has fallen below the critical threshold level then initiate
	a headset power down sequence.
    
RETURNS
    void
*/
static void handleBatteryVoltageReading(hsTaskData* theHeadset, uint32 reading , Task pTask)
{	
	power_type* power = theHeadset->power;
	/* Calculate the current battery voltage in mV */
	uint32 vb = ((reading * 1000) / power->config.battery.divisor_ratio);

	BT_DEBUG(("VBAT: %lumV\n", vb));
#ifdef READ_VOL
    theHeadset->battery_mv = (int)vb;
#endif

	/* Store current battery reading */
	power->vbat_task.current_reading = (int16)vb;
	
	BT_DEBUG(("BAT [%d][%d][%d]\n", 	
	    (uint16)(power->config.battery.high_threshold  * 20) ,
        (uint16)(power->config.battery.low_threshold      * 20) ,
    	(uint16)(power->config.battery.shutdown_threshold * 20) 
	)) ;
	
	
	/* Check current voltage level against configured thresholds */
	
	if (vb > (uint16)(power->config.battery.high_threshold  * 20) )
		batteryNormal( pTask, theHeadset );	
	else if(vb < (uint16)(power->config.battery.shutdown_threshold * 20) )
		batteryShutdown( pTask, theHeadset );
	else if(vb < (uint16)(power->config.battery.low_threshold      * 20) )
		batteryLow(pTask, theHeadset);
}


/****************************************************************************
NAME    
    aio_handler
    
DESCRIPTION
  	AIO readings arrive here for processing
    
RETURNS
    void
*/
static void aio_handler(Task task, MessageId id, Message message)
{
	uint32	reading;
	
	/* Get Power configuration data */
	power_type* power = ((hsTaskData *)getAppTask())->power;
	hsTaskData * lApp = (hsTaskData *) getAppTask() ;
	
	/* This function receives messages from the battery library */
	battery_reading_source source = BATTERY_INTERNAL;
	
	switch(id)
	{
		case BATTERY_READING_MESSAGE :		
			/* New reading, extract reading in mV and handle accordingly */
			reading = (*((uint32*)message));
			
			/* Readings can either be AIO0 (Battery Voltage) or AIO1 (Battery Temperature) */
			switch(source)
			{
                case BATTERY_INTERNAL:
					/* Battery Voltage */
					handleBatteryVoltageReading(lApp, reading , task);
					break;
			
                case AIO0:
				case AIO1:			
				case VDD:
				case AIO2:
				case AIO3:
				default:
					break;
			}	
            /* If initial reading, revert back to default battery reading period */
            if (lApp->initial_battery_reading)
            {
				BT_DEBUG(("BATT : Initial reading\n")) ;
                lApp->initial_battery_reading = FALSE;
            	BatteryInit(&power->vbat_task.state, &power->vbat_task.task, BATTERY_INTERNAL, D_SEC(power->config.battery.monitoring_period));	           
            }
			break;

        
		default:
			break;
	}
}


/*****************************************************************************/
void batteryInit(hsTaskData* theHeadset)
{
	power_type* power = theHeadset->power;
	
	/* Initialise the default battery readings */
    theHeadset->initial_battery_reading = TRUE;
	   
	/* --- Battery Voltage --- */
	/* The battery voltage is monitored at all times.  Initialise the battery
	   library to read the battery voltage via BATTERY_INTERNAL */
	power->vbat_task.task.handler = aio_handler;
		
    /* Read battery now */
    batteryRead(theHeadset);
}


/*****************************************************************************/
void batteryRead(hsTaskData* theHeadset)
{
	power_type* power = theHeadset->power;
	
	theHeadset->initial_battery_reading = TRUE;
	BatteryInit(&power->vbat_task.state, &power->vbat_task.task, BATTERY_INTERNAL, 0);	
}

