/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_configmanager.h
@brief    Configuration manager for the headset - resoponsible for extracting user information out of the PSKEYs and initialising the configurable nature of the headset components.
*/
#ifndef HEADSET_CONFIG_MANAGER_H
#define HEADSET_CONFIG_MANAGER_H

#include "headset_private.h"

/* Persistent store key allocation */
#define PSKEY_BASE  (0)
enum
{
    PSKEY_BATTERY_CONFIG           = PSKEY_BASE,
    PSKEY_BUTTON_CONFIG            = 1,
    PSKEY_BUTTON_PATTERN_CONFIG    = 2,
    PSKEY_HFP_SUPPORTED_FEATURES   = 3,
    PSKEY_VOICE_MANUAL_MODE        = 4, /* v100225 */
    PSKEY_AUTO_ANSWER              = 5,
    PSKEY_TIMEOUTS                 = 6,
#ifndef AUTO_MIC_DETECT
    PSKEY_EXTMIC_MODE              = 8, /* Software Version v110422 */
#endif
    PSKEY_AMP                      = 9,
    PSKEY_USED1                    = 10, /*used for CVC key*/
    PSKEY_USED2                    = 11, /*used for CVC key*/
    PSKEY_NO_LED_FILTERS           = 15,
    PSKEY_LED_FILTERS              = 16,
    PSKEY_NO_LED_STATES_A          = 17,
    PSKEY_LED_STATES_A             = 18,
    PSKEY_NO_LED_STATES_B          = 19,
    PSKEY_LED_STATES_B             = 20,
    PSKEY_NO_LED_EVENTS            = 21,
    PSKEY_LED_EVENTS               = 22,
    PSKEY_EVENTS_A                 = 23,
    PSKEY_EVENTS_B                 = 24,
    PSKEY_NO_TONES                 = 25,
    PSKEY_TONES                    = 26,
    PSKEY_USED3                    = 28, /*used for CVC key*/
    PSKEY_LAST_USED_AG             = 29,
    PSKEY_CONFIGURATION_ID         = 30,
    PSKEY_LAST_USED_AV_SOURCE      = 31,
    PSKEY_LAST_USED_AV_SOURCE_SEID = 32,
    PSKEY_CODEC_ENABLED            = 33,
    PSKEY_LAST_PAIRED_DEVICE       = 34,
    PSKEY_VOLUME_LEVELS            = 35,
    PSKEY_VOLUME_GAINS             = 36,
    PSKEY_FEATURES                 = 37,
    PSKEY_A2DP_TONE_VOLUME         = 38, /*used for A2DP tone mixing volume*/
    PSKEY_SSR_PARAMS               = 40  /* Sniff Subrate parameters */
};

/* Bit field define for CODEC Enabled key */
#define MASK_MP3_ENABLED (1<<0)
#define MASK_AAC_ENABLED (1<<1)


/* Persistent store event configuration definition */
typedef struct
{
 	unsigned    event:8;
 	unsigned 	type:8;
    uint16  	pio_mask_16_to_31;
 	uint16  	pio_mask_0_to_15;
	unsigned  	hfp_state_mask:8;
	unsigned  	a2dp_state_mask:8;
}event_config_type;

#define MAX_EVENTS ( EVENTS_MAX_EVENTS )


/* Persistent store LED configuration definition */
typedef struct
{
 	unsigned 	state:8;
    unsigned 	a2dp_state:8;
 	unsigned 	on_time:8;
 	unsigned 	off_time:8;
 	unsigned  	repeat_time:8;
 	unsigned  	dim_time:8;
 	unsigned  	timeout:8;
 	unsigned 	number_flashes:4;
 	unsigned 	led_a:4;
 	unsigned 	led_b:4;
    unsigned    overide_disable:1;
 	unsigned 	colour:3;
    unsigned    unused:8;
}led_config_type;

typedef struct
{
 	unsigned 	event:8;
 	unsigned 	speed:8;
     
    unsigned 	active:1;
    unsigned    dummy:1 ;
 	unsigned 	speed_action:2;
 	unsigned  	colour:4;
    unsigned    filter_to_cancel:4 ;
    unsigned    overide_led:4 ;   
    
    unsigned    overide_led_active:1 ;
    unsigned    dummy2:2;
    unsigned    follower_led_active:1 ;
    unsigned    follower_led_delay_50ms:4;
    unsigned    overide_disable:1 ;
    unsigned    dummy3:7 ;
    
    
}led_filter_config_type;

#define MAX_STATES (HEADSET_NUM_HFP_STATES)
#define MAX_LED_EVENTS (20)
#define MAX_LED_STATES (HEADSET_NUM_HFP_STATES * HEADSET_NUM_A2DP_STATES)
#define MAX_LED_FILTERS (LM_NUM_FILTER_EVENTS)

/* LED patterns */
typedef enum
{
 	led_state_pattern,
 	led_event_pattern
}configType;


typedef struct 
{
    unsigned event:8;
    unsigned tone:8;
}tone_config_type ;


typedef struct
{
    uint16 event;
    button_pattern_type pattern[6];
}button_pattern_config_type ;


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
  	configManagerInit

DESCRIPTION
  	The Configuration Manager is responsible for reading the user configuration
  	from the persistent store are setting up the system.  Each system component
  	is initialised in order.  Where appropriate, each configuration parameter
  	is limit checked and a default assigned if found to be out of range.

*/
void configManagerInit (hsTaskData* theHeadset);


/***************************************************************************
NAME 
  	configManagerSetupSupportedFeatures

DESCRIPTION
  	Setup the local supported features.
    
*/ 
void configManagerSetupSupportedFeatures(hsTaskData* theHeadset);

/***************************************************************************
NAME 
  	configManagerSetupVolumeGains

DESCRIPTION
  	Setup the volume gain levels.
    
*/ 
void configManagerSetupVolumeGains(uint16 *gains, uint16 size);

/****************************************************************************
NAME 
  	configManagerReset

DESCRIPTION
    Resets the Paired Device List - Reboots if PSKEY says to do so.
    
*/ 
void configManagerReset ( hsTaskData * pApp ) ;     

#endif   
