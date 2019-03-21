/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_configmanager.c
@brief    Configuration manager for the headset - resoponsible for extracting user information out of the PSKEYs and initialising the configurable nature of the headset components.
*/


#include "headset_amp.h"
#include "headset_buttonmanager.h"
#include "headset_configmanager.h"
#include "headset_config.h"
#include "headset_debug.h"
#include "headset_events.h"
#include "headset_LEDmanager.h"
#include "headset_powermanager.h"
#include "headset_statemanager.h"
#include "headset_private.h"
#include "headset_tones.h"

#include <csrtypes.h>
#include <ps.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <panic.h>

#ifdef DEBUG_CONFIG
#define CONF_DEBUG(x) DEBUG(x)
#else
#define CONF_DEBUG(x) 
#endif

#define HCI_PAGESCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_PAGESCAN_WINDOW_DEFAULT   (0x12)
#define HCI_INQUIRYSCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_INQUIRYSCAN_WINDOW_DEFAULT   (0x12)


#define DEFAULT_VOLUME_MUTE_REMINDER_TIME_SEC 10


/****************************************************************************
    LOCAL FUNCTIONS
*/
static void   	configManagerButtons                ( hsTaskData* theHeadset);
static void  	configManagerLEDS                   ( hsTaskData* theHeadset);
static void  	configManagerButtonDurations        ( hsTaskData* theHeadset);
static void     configManagerEventTones             ( hsTaskData* theHeadset);
static void     configManagerButtonPatterns         ( hsTaskData* theHeadset);
static void 	configManagerPower                  ( hsTaskData* theHeadset);
static void     configManagerTimeouts               ( hsTaskData* theHeadset);
static void     configManagerAmp	                ( hsTaskData* theHeadset);
static void     configManagerFeatures               ( hsTaskData* theHeadset);
static void     configManagerSsr                    ( hsTaskData* theHeadset);


/****************************************************************************
  FUNCTIONS
*/

/*****************************************************************************/
void configManagerInit(hsTaskData* theHeadset)  
{ 
  	    /* Read and configure the button durations */
  	configManagerButtonDurations(theHeadset);
    
  	    /* Read the system event configuration and configure the buttons */
    configManagerButtons(theHeadset);

        /*configures the pattern button events*/
    configManagerButtonPatterns(theHeadset) ;

        /*Read and configure the event tones*/
    configManagerEventTones(theHeadset) ;

  	    /* Read and configure the LEDs */
    configManagerLEDS(theHeadset);
    
        /* Read and configure the power management system */
  	configManagerPower(theHeadset);
    
         /* Read and configure the timeouts */
    configManagerTimeouts(theHeadset);
	
		/* Read and configure the audio amp control */
    configManagerAmp(theHeadset);
	
		/* Read and configure the headset features */
    configManagerFeatures(theHeadset);

    /* read and configure the Sniff Subrate parameters */
    configManagerSsr(theHeadset);
}


/*****************************************************************************/ 
void configManagerSetupSupportedFeatures(hsTaskData* theHeadset)
{
	uint16 local_hfp_features;
	if(!ConfigRetrieve(PSKEY_HFP_SUPPORTED_FEATURES, &local_hfp_features, sizeof(uint16)))
    {
     	local_hfp_features = (HFP_NREC_FUNCTION | HFP_VOICE_RECOGNITION | HFP_REMOTE_VOL_CONTROL);
	}
	theHeadset->local_hfp_features = local_hfp_features;
}

/*****************************************************************************/ 
void configManagerSetupVolumeGains(uint16 *gains, uint16 size)
{
	/* ConfigRetrieve cannot fail since we have volume levels in the default config. */
	(void)ConfigRetrieve(PSKEY_VOLUME_GAINS, gains, size);
}


/*****************************************************************************/ 
void configManagerReset ( hsTaskData * pApp ) 
{
#ifdef NORMAL_ANSWER_MODE
    uint8 lnormalanswermode = 1;
#endif

    CONF_DEBUG(("CO: Reset\n")) ;

    /* Reset the Last AG */
    (void)PsStore ( PSKEY_LAST_USED_AG , 0 , 0 ) ;

    /* Reset the Last A2DP source */
    (void)PsStore ( PSKEY_LAST_USED_AV_SOURCE , 0 , 0 ) ;

    /* Reset the Last A2DP source SEP */
    (void)PsStore ( PSKEY_LAST_USED_AV_SOURCE_SEID , 0 , 0 ) ;

    /* Reset the Last paired device */
    (void)PsStore ( PSKEY_LAST_PAIRED_DEVICE , 0 , 0 ) ;

    /* Reset the Volume Levels */
    (void)PsStore ( PSKEY_VOLUME_LEVELS , 0 , 0 ) ;

#ifdef NORMAL_ANSWER_MODE
    /* Reset the Autoanswer Mode Flag */
    (void)PsStore ( PSKEY_AUTO_ANSWER , &lnormalanswermode, sizeof(uint8) ) ;
#else
    /* Reset the Autoanswer Mode Flag */
    (void)PsStore ( PSKEY_AUTO_ANSWER , 0 , 0 ) ;
#endif

#ifndef AUTO_MIC_DETECT
    /* Reset the Extmic Mode Flag */
    (void)PsStore ( PSKEY_EXTMIC_MODE , 0 , 0 ) ;
#endif

    /* Reset the Voicemanual Mode Flag */ /* v100225 */
    (void)PsStore ( PSKEY_VOICE_MANUAL_MODE , 0 , 0 ) ;

#ifdef R100
    /* Reset the Slave Mode Flag */
    (void)PsStore ( 7 , 0 , 0 ) ;
#endif

    /* Reset the Last Slave Intercom device */
    (void)PsStore ( 12 , 0 , 0 ) ;

    /* Reset the Last Master Intercom device */
    (void)PsStore ( 14 , 0 , 0 ) ;

    /* Delete the Connection Libs Paired Device List */
    ConnectionSmDeleteAllAuthDevices ( 0 );
}


/****************************************************************************
NAME 
  	configManagerButtons

DESCRIPTION
 	Read the system event configuration from persistent store and configure
  	the buttons by mapping the associated events to them.
*/  
static void configManagerButtons(hsTaskData* theHeadset)
{ 
  	uint16 no_events = 20;
 
	/* Allocate enough memory to hold event configuration */
    event_config_type* config = (event_config_type*) PanicUnlessMalloc(no_events * sizeof(event_config_type));
    
        /*read in the events for the first PSKEY*/                
    if(ConfigRetrieve(PSKEY_EVENTS_A, config, no_events * sizeof(event_config_type)))
  	{
        uint16 n;
 
    		/* Now we have the event configuration, map required events to system events */
        for(n = 0; n < no_events; n++)
        { 
            CONF_DEBUG(("Co : AddMap Ev[%x] \n", config[n].event )) ;
                    
            if ( config[n].pio_mask_0_to_15 | config[n].pio_mask_16_to_31 )
            {
                    /* Map PIO button event to system events in specified states */
                buttonManagerAddMapping (&theHeadset->theButtonTask ,
										 ((uint32)config[n].pio_mask_16_to_31 << 16) | config[n].pio_mask_0_to_15, 
                 						(config[n].event + EVENTS_EVENT_BASE) ,
                						 config[n].hfp_state_mask, 
                                         config[n].a2dp_state_mask, 
                						(ButtonsTime_t)config[n].type); 
            }                                            
    	}
  	}
		else
		{
	   CONF_DEBUG(("Co: !EvLen\n")) ;
    }
    
        /*now do the same for the second PSKEY*/
    if(ConfigRetrieve(PSKEY_EVENTS_B, config, no_events * sizeof(event_config_type)))
  	{
        uint16 n;
 
    		/* Now we have the event configuration, map required events to system events */
        for(n = 0; n < no_events; n++)
        { 
            CONF_DEBUG(("Co : AddMap Ev[%x]\n", config[n].event )) ;
                    
            if ( config[n].pio_mask_0_to_15 | config[n].pio_mask_16_to_31 )
            {
                    /* Map PIO button event to system events in specified states */
                buttonManagerAddMapping (&theHeadset->theButtonTask ,
										 ((uint32)config[n].pio_mask_16_to_31 << 16) | config[n].pio_mask_0_to_15, 
                 						(config[n].event + EVENTS_EVENT_BASE) ,
                						 config[n].hfp_state_mask, 
                                         config[n].a2dp_state_mask, 
                						(ButtonsTime_t)config[n].type); 
            }          
    	}
  	}
		else
		{
	   CONF_DEBUG(("Co: !EvLen2\n")) ;
    }        

    	/* Free up memory */
  	free(config);
}


/****************************************************************************
NAME 
  	configManagerButtonPatterns

DESCRIPTION
  	Read and configure any buttonpattern matches that exist.
    
*/
static void configManagerButtonPatterns(hsTaskData * theHeadset) 
{  
      		/* Allocate enough memory to hold event configuration */
    button_pattern_config_type* config = (button_pattern_config_type*) PanicUnlessMalloc(BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type));
   
    CONF_DEBUG(("Co: No Button Patterns - %d\n", BM_NUM_BUTTON_MATCH_PATTERNS));
   
    /* Now read in event configuration */
    if(ConfigRetrieve(PSKEY_BUTTON_PATTERN_CONFIG, config, BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type)))
    {
        uint16 n;
 
       /* Now we have the event configuration, map required events to system events */
        for(n = 0; n < BM_NUM_BUTTON_MATCH_PATTERNS ; n++)
        {	 
 	      CONF_DEBUG(("Co : AddPattern Ev[%x]\n", config[n].event )) ;
                    
      			   /* Map PIO button event to system events in specified states */
      	    buttonManagerAddPatternMapping ( &theHeadset->theButtonTask , config[n].event + EVENTS_EVENT_BASE , config[n].pattern ) ;
        }
    }
    else
	    {
	      CONF_DEBUG(("Co: !EvLen\n")) ;
    }
    free (config) ;
}


/****************************************************************************
NAME 
  	config

DESCRIPTION
  	Read the LED configuration from persistent store and configure the LEDS.
 
RETURNS
  	TRUE or FALSE
    
*/ 
static bool config(hsTaskData * theHeadset , configType type, uint16 pskey_no, uint16 pskey_config, uint16 max) 
{ 
  	bool success = FALSE;
  	uint16 no_events = 0;
 
  	/* First read the number of states/events configured */
  	if(ConfigRetrieve(pskey_no, &no_events, sizeof(uint16)))
  	{	  
		CONF_DEBUG(("Co: no_events:%d max:%d\n",no_events,max)) ;
    	/* Providing there are states to configure */
    	if((no_events > 0) && (no_events <= max))
    	{
      		/* Allocate enough memory to hold state/event configuration */
      		led_config_type* config = (led_config_type*) PanicUnlessMalloc(no_events * sizeof(led_config_type));
   
      		/* Now read in configuration */
   			if(ConfigRetrieve(pskey_config, config, no_events * sizeof(led_config_type)))
   			{
     			uint16    n;
     			LEDPattern_t  pattern;                   

     			/* Now we have the configuration, map to system states/events */
     			for(n = 0; n < no_events; n++)
     			{ 
       				pattern.LED_A           = config[n].led_a;
       				pattern.LED_B           = config[n].led_b;
         			pattern.OnTime          = config[n].on_time * 10;
       				pattern.OffTime         = config[n].off_time * 10;
       				pattern.RepeatTime      = config[n].repeat_time * 50;
       				pattern.NumFlashes      = config[n].number_flashes;           		
      				pattern.TimeOut         = config[n].timeout;
      				pattern.DimTime         = config[n].dim_time;
      				
       				pattern.Colour          = config[n].colour;
                    pattern.OverideDisable  = config[n].overide_disable;
                    
     				    
       				switch(type)
       				{
         				case led_state_pattern:
          					LEDManagerAddLEDStatePattern(&theHeadset->theLEDTask , config[n].state, config[n].a2dp_state , &pattern);
          					break;
         				case led_event_pattern:
          					LEDManagerAddLEDEventPattern(&theHeadset->theLEDTask , EVENTS_EVENT_BASE + config[n].state, &pattern);
          					break;
       				}       
     			}
     			success = TRUE;
   			}
            else
            {
                CONF_DEBUG(("Co: !LedLen\n")) ;
            }
            /* Free up memory */
   			free(config);
  		}
  	}
  	return success;
}


/****************************************************************************
NAME 
 	config_led_filter

DESCRIPTION
 	Read the LED filter configuration from persistent store and configure the
 	LED filters.
 
RETURNS
 	TRUE or FALSE
    
*/ 
static bool config_filter( hsTaskData * theHeadset , uint16 pskey_no, uint16 pskey_filter, uint16 max)
{
 	bool success = FALSE;
 	uint16 no_filters = 0;
 
  	/* First read the number of filters configured */
  	if(ConfigRetrieve(pskey_no, &no_filters, sizeof(uint16)))
  	{  
    	/* Providing there are states to configure */
    	if((no_filters > 0) && (no_filters <= max))
    	{
      		/* Allocate enough memory to hold filter configuration */
      		led_filter_config_type* config = (led_filter_config_type*) PanicUnlessMalloc(no_filters * sizeof(led_filter_config_type));
   
      		/* Now read in configuration */
   			if(ConfigRetrieve(pskey_filter, config, no_filters * sizeof(led_filter_config_type)))
   			{
     			uint16    n;
     			LEDFilter_t  filter;

     			/* Now we have the configuration, map to system states/events */
     			for(n = 0; n < no_filters; n++)
     			{ 
       				filter.Event                = EVENTS_EVENT_BASE + config[n].event;
       				filter.Speed                = config[n].speed;
       				filter.IsFilterActive       = config[n].active;
       				filter.SpeedAction          = config[n].speed_action;
       				filter.Colour               = config[n].colour;
                    filter.FilterToCancel       = config[n].filter_to_cancel ;
                    filter.OverideLED           = config[n].overide_led ;

                    filter.OverideLEDActive     = config[n].overide_led_active ;
                    filter.FollowerLEDActive    = config[n].follower_led_active ;

                    filter.FollowerLEDDelay     = config[n].follower_led_delay_50ms ;
                    filter.OverideDisable       = config[n].overide_disable;

                        /*add the filter*/
      				LEDManagerAddLEDFilter(&theHeadset->theLEDTask , &filter);
                                			} 
   			}
            else
            {
                CONF_DEBUG(("Co :!FilLen\n")) ;
            }
    		/* Free up memory */
   			free(config);

       		success = TRUE;
    	}
  	} 
  	return success;
}


/****************************************************************************
NAME 
  	configManagerLEDS

DESCRIPTION
  	Read the system LED configuration from persistent store and configure
  	the LEDS. 
    
*/ 
static void configManagerLEDS(hsTaskData* theHeadset)
{ 
  	/* 1. LED state configuration */
  	config(theHeadset , led_state_pattern, PSKEY_NO_LED_STATES_A, PSKEY_LED_STATES_A, MAX_LED_STATES/2);
 	config(theHeadset , led_state_pattern, PSKEY_NO_LED_STATES_B, PSKEY_LED_STATES_B, MAX_LED_STATES/2);	
	
  	/* 2. LED event configuration */
  	config(theHeadset , led_event_pattern, PSKEY_NO_LED_EVENTS, PSKEY_LED_EVENTS, MAX_LED_EVENTS);
 
  	/* 3. LED event filter configuration */
  	config_filter(theHeadset , PSKEY_NO_LED_FILTERS, PSKEY_LED_FILTERS, MAX_LED_FILTERS);         
  	
  	/*tri colour behaviour*/  	
  	/*ConfigRetrieve(PSKEY_TRI_COL_LEDS, &theHeadset->theLEDTask.gTriColLeds,  sizeof(uint16)) ;*/
}


/****************************************************************************
NAME 
  	configManagerButtonDurations

DESCRIPTION
  	Read the button configuration from persistent store and configure
  	the button durations.
    
*/ 
static void configManagerButtonDurations(hsTaskData* theHeadset)
{
 	button_config_type  buttons;
  
   	if(ConfigRetrieve(PSKEY_BUTTON_CONFIG, &buttons, sizeof(button_config_type)))
 	{
  		buttonManagerConfigDurations (&theHeadset->theButtonTask, buttons);
 	}                  
}
    

/****************************************************************************
NAME 
  	configManagerEventTone

DESCRIPTION
  	Configure an event tone only if one is defined.
 
*/ 
static void configManagerEventTones ( hsTaskData* theHeadset)
{ 
    uint16 no_tones = 0;
	uint16 ret_len = 0;
	uint32 data;
 
  	/* First read the number of events configured */
  	if(ConfigRetrieve(PSKEY_NO_TONES, &no_tones, sizeof(uint16)))
  	{
        /* Allocate enough memory to hold event configuration */
    	tone_config_type * config = (tone_config_type *) PanicUnlessMalloc(no_tones * sizeof(tone_config_type));
 
     	/* Now read in tones configuration */
    	if(ConfigRetrieve(PSKEY_TONES, config, no_tones * sizeof(tone_config_type)))
    	{
      		uint16 n;
   
       		/* Now we have the event configuration, map required events to system events */
        	for(n = 0; n < no_tones; n++)
        	{
                CONF_DEBUG(("CO: Ev[%x]Tone[%x] \n" , config[n].event, config[n].tone )) ;
                TonesConfigureEvent ( theHeadset , (config[n].event + EVENTS_EVENT_BASE), config[n].tone  ) ;
            }   
        }                    
        free ( config ) ;
    }    
	
 	/* Read the mixed A2DP tone volume */
 	ret_len = PsRetrieve(PSKEY_A2DP_TONE_VOLUME, &data, sizeof(uint32));
 
 	/* If no key exists then set to a default value */
 	if(!ret_len)
	{
		data = 0x3fff3fff;
		PsStore(PSKEY_A2DP_TONE_VOLUME, &data, sizeof(uint32));
	}
}


/****************************************************************************
NAME 
  	configManagerPower

DESCRIPTION
  	Read the Power Manager configuration.
 
*/ 
static void configManagerPower(hsTaskData* theHeadset)
{
    power_config_type config;
 
 	memset(&config, 0, sizeof(power_config_type));
    
    /* Read in the battery monitoring configuration */
  	ConfigRetrieve(PSKEY_BATTERY_CONFIG, &config.battery, sizeof(battery_config_type));
  
    /*  Setup the power manager */
  	powerManagerConfig(theHeadset, &config);          
}


/****************************************************************************
NAME 
    configManagerTimeouts

DESCRIPTION
    Read and configure the timeouts.
 
*/ 
static void configManagerTimeouts ( hsTaskData* theHeadset)
{	
    /* Get the timeout values */
	ConfigRetrieve(PSKEY_TIMEOUTS, &theHeadset->Timeouts, sizeof(Timeouts_t) ) ;
	
	    
    CONF_DEBUG(("Co : Pairing Timeout[%d]\n" , theHeadset->Timeouts.PairModeTimeout_s )) ;
}


/****************************************************************************
NAME 
    configManagerAmp

DESCRIPTION
    Read and configure the amp control.
 
*/ 
static void configManagerAmp ( hsTaskData* theHeadset)
{	
	Amp_t amp;
    /* Get the timeout values */
	ConfigRetrieve(PSKEY_AMP, &amp, sizeof(Amp_t) ) ;
	
	theHeadset->useAmp = amp.useAmp;
	theHeadset->ampAutoOff = amp.ampAutoOff;
	theHeadset->ampPio = amp.ampPio;
	theHeadset->ampOffDelay = amp.ampOffDelay;
	
	CONF_DEBUG(("Co : Amp config: Use Amp[%d] Auto off[%d] Pio[%d] Off delay[%d secs]\n" , theHeadset->useAmp,theHeadset->ampAutoOff,theHeadset->ampPio,theHeadset->ampOffDelay )) ;
	
	/* Configure the amp then turn it on */
	AmpSetOffDelay(theHeadset, theHeadset->ampOffDelay);
	AmpSetPio(theHeadset, theHeadset->ampPio);	
}


/****************************************************************************
NAME 
    configManagerFeatures

DESCRIPTION
    Read and configure the features block.
 
*/ 
static void configManagerFeatures ( hsTaskData* theHeadset)
{	
	Features_t features;
    /* Get the timeout values */
	ConfigRetrieve(PSKEY_FEATURES, &features, sizeof(Features_t) ) ;
	
	theHeadset->autoSendAvrcp = features.autoSendAvrcp;
	theHeadset->cvcEnabled = features.cvcEnabled;
        theHeadset->forceMitmEnabled = features.forceMitmEnabled;
        theHeadset->writeAuthEnable = features.writeAuthEnable;
        theHeadset->debugKeysEnabled = features.debugKeysEnabled;
	
	CONF_DEBUG(("Co : Features config: Auto AVRCP[%d] CVC[%d] MITM[%d] WAE[%d] DebugKeys[%d]\n" ,
                                                theHeadset->autoSendAvrcp ,
                                                theHeadset->cvcEnabled,
                                                theHeadset->forceMitmEnabled,
                                                theHeadset->writeAuthEnable,
                                                theHeadset->debugKeysEnabled)) ;
                                                                                
}

/****************************************************************************
NAME 
    configManagerSsr

DESCRIPTION
    Read and configure the Sniff Subrate parameters.
 
RETURNS
    void
*/ 
static void configManagerSsr(hsTaskData* theHeadset)
{
    subrate_data ssr_params;

    /* get the parameters either from PS or constant space */
    ConfigRetrieve(PSKEY_SSR_PARAMS, &ssr_params, sizeof(subrate_data));
    /* store them in headset for later use */
    theHeadset->ssr_data.streaming_params.max_remote_latency = ssr_params.streaming_params.max_remote_latency;
    theHeadset->ssr_data.streaming_params.min_remote_timeout = ssr_params.streaming_params.min_remote_timeout;
    theHeadset->ssr_data.streaming_params.min_local_timeout = ssr_params.streaming_params.min_local_timeout;
    theHeadset->ssr_data.signalling_params.max_remote_latency = ssr_params.signalling_params.max_remote_latency;
    theHeadset->ssr_data.signalling_params.min_remote_timeout = ssr_params.signalling_params.min_remote_timeout;
    theHeadset->ssr_data.signalling_params.min_local_timeout = ssr_params.signalling_params.min_local_timeout;

    CONF_DEBUG(("Co : SSR config: Str - MRL[%d]MRT[%d]MLT[%d] Sig - MRL[%d]MRT[%d]MLT[%d]",
                                theHeadset->ssr_data.streaming_params.max_remote_latency,
                                theHeadset->ssr_data.streaming_params.min_remote_timeout,
                                theHeadset->ssr_data.streaming_params.min_local_timeout,
                                theHeadset->ssr_data.signalling_params.max_remote_latency,
                                theHeadset->ssr_data.signalling_params.min_remote_timeout,
                                theHeadset->ssr_data.signalling_params.min_local_timeout));
}
