/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_config.h
@brief    Interface to functions for configing the headset.
*/

#ifndef _HEADSET_CONFIG_H_
#define _HEADSET_CONFIG_H_


#include "headset_configmanager.h"
#include "headset_states.h"


typedef struct
{
 	uint16     length;
 	uint16     value[1];
}config_uint16_type;


typedef struct
{
  	uint16         length;
  	uint16         value[sizeof(battery_config_type)];
}config_battery_type;


typedef struct
{
   	uint16         length;
   	uint16         value[sizeof(button_config_type)];
}config_button_type;

typedef struct
{
    uint16         length ;
    uint16         value[sizeof(button_pattern_config_type) * BM_NUM_BUTTON_MATCH_PATTERNS]; 
}config_button_pattern_type ;

typedef struct
{
	uint16	blank;
}radio_config_type;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(radio_config_type)];
}config_radio_type;


typedef struct
{
    uint16 blank;   
}HFP_1_5_features_type;

typedef struct
{
    uint16 length ;
    uint16      value[sizeof(HFP_1_5_features_type)];
}config_HFP_1_5_type ;


#define VOL_NUM_VOL_SETTINGS     (0x10)
typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(uint16) * VOL_NUM_VOL_SETTINGS];
}config_volume_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_filter_config_type) * MAX_LED_FILTERS];
}config_led_filters_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_config_type) * MAX_LED_STATES];
}config_led_states_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_config_type) * MAX_LED_EVENTS];
}config_led_events_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(event_config_type) * MAX_EVENTS]; 
}config_events_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(tone_config_type) * MAX_EVENTS]; 
}config_tone_events_type;

typedef struct
{
    uint16 blank;   
}PIO_block_t;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(PIO_block_t)];
}config_pio_block_type;

typedef struct
{
	uint16     length;
	uint16     value[sizeof(Timeouts_t)] ; 
}config_timeouts ;

typedef struct
{
	uint16     length;
	uint16     value[sizeof(Amp_t)] ; 
}config_amp ;

typedef struct
{
	uint16     length;
	uint16     value[sizeof(Features_t)] ; 
}config_features ;

typedef struct
{
        uint16     length;
        uint16     value[sizeof(subrate_data)];
}config_ssr_params_type;

typedef struct
{

    const config_battery_type*  	battery_config;     	    /* PSKEY_USR_0 - Battery configuration */
 	const config_button_type*  		button_config;     		    /* PSKEY_USR_1 - Button configuration */
 	const config_button_pattern_type*   button_pattern_config;  /* PSKEY_USR_2 - Button Sequence Patterns*/
    const config_timeouts*  		timeouts_config;  		    /* PSKEY_USR_6 - timeouts */
	const config_amp* 				amp_config;   		    	/* PSKEY_USR_9 - Amp configuration */
 	const config_uint16_type*  		no_led_filters;         	/* PSKEY_USR_15 - Number of LED filters */
 	const config_led_filters_type* 	led_filters;     	    	/* PSKEY_USR_16 - LED filter configuration */
 	const config_uint16_type*  		no_led_states_a;   	    	/* PSKEY_USR_17 - Number of LED states */
 	const config_led_states_type* 	led_states_a;      	    	/* PSKEY_USR_18 - LED state configuration */
	const config_uint16_type*  		no_led_states_b;   	    	/* PSKEY_USR_19 - Number of LED states */
 	const config_led_states_type* 	led_states_b;      	    	/* PSKEY_USR_20 - LED state configuration */
 	const config_uint16_type*  		no_led_events;     		    /* PSKEY_USR_21 - Number of LED events */
 	const config_led_events_type* 	led_events;      		    /* PSKEY_USR_22 - LED event configuration */
 	const config_events_type*  		events_a;     			    /* PSKEY_USR_23 - System event configuration */
 	const config_events_type*  		events_b;       	        /* PSKEY_USR_24 - System event configuration */
 	const config_uint16_type*  		no_tone_events;         	/* PSKEY_USR_25 - Number of tone events */
 	const config_tone_events_type* 	tones;       		    	/* PSKEY_USR_26 - Tone event configuration */
 	const config_volume_type* 		vol_gains;       		    /* PSKEY_USR_35 - Volume Gains */
	const config_features* 			features;       		    /* PSKEY_USR_37 - Features */
        const config_ssr_params_type*   ssr_config;                     /* PSKEY_USR_40 - Sniff Subrate parameters */

}config_type;


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
 	ConfigRetrieve

DESCRIPTION
 	This function is called to read a configuration key.  If the key exists
 	in persistent store it is read from there.  If it does not exist then
 	the default is read from constant space.
 
RETURNS
 	0 if no data was read otherwise the length of data.
    
*/
uint16 ConfigRetrieve(uint16 key, void* data, uint16 len);


#endif /* _HEADSET_CONFIG_H_ */
