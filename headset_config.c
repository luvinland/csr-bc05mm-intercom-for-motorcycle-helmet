/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_config.c
@brief   Implementation of headset configuration functionality. 
*/


#include "headset_config.h"

#include <ps.h>
#include <stdlib.h>
#include <string.h>


/****************************************************************************/
/* Reference the default configuartion definition files */

extern const config_type csr_pioneer_default_config;

 
/****************************************************************************/

/* Key defintion structure */
typedef struct
{
 	uint16    length;
 	const uint16* data;
}key_type;


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
 	getkey

DESCRIPTION
 	Access specified key in constant space.
 
*/
static key_type getkey(uint16 key_id)
{
 	key_type key = {0, 0};
 
 	switch(key_id)
 	{
        case PSKEY_BATTERY_CONFIG:
      		key.length = csr_pioneer_default_config.battery_config->length;
      		key.data = (uint16*)csr_pioneer_default_config.battery_config->value;
      		break;
            
  		case PSKEY_BUTTON_CONFIG:
        	key.length = csr_pioneer_default_config.button_config->length;
        	key.data = (uint16*)csr_pioneer_default_config.button_config->value;
        	break;
            
        case PSKEY_BUTTON_PATTERN_CONFIG:
        	key.length = csr_pioneer_default_config.button_pattern_config->length;
        	key.data = (uint16*)csr_pioneer_default_config.button_pattern_config->value;
        	break;
  
  		case PSKEY_NO_LED_FILTERS:
   			key.length = csr_pioneer_default_config.no_led_filters->length;
   			key.data = (uint16*)csr_pioneer_default_config.no_led_filters->value;
   			break;
   
  		case PSKEY_LED_FILTERS:
   			key.length = csr_pioneer_default_config.led_filters->length;
   			key.data = (uint16*)csr_pioneer_default_config.led_filters->value;
   			break;
   
  		case PSKEY_NO_LED_STATES_A:
   			key.length = csr_pioneer_default_config.no_led_states_a->length;
   			key.data = (uint16*)csr_pioneer_default_config.no_led_states_a->value;
   			break;
   
  		case PSKEY_LED_STATES_A:
   			key.length = csr_pioneer_default_config.led_states_a->length;
   			key.data = (uint16*)csr_pioneer_default_config.led_states_a->value;
   			break;
			
		case PSKEY_NO_LED_STATES_B:
   			key.length = csr_pioneer_default_config.no_led_states_b->length;
   			key.data = (uint16*)csr_pioneer_default_config.no_led_states_b->value;
   			break;
   
  		case PSKEY_LED_STATES_B:
   			key.length = csr_pioneer_default_config.led_states_b->length;
   			key.data = (uint16*)csr_pioneer_default_config.led_states_b->value;
   			break;
   
  		case PSKEY_NO_LED_EVENTS:
   			key.length = csr_pioneer_default_config.no_led_events->length;
   			key.data = (uint16*)csr_pioneer_default_config.no_led_events->value;
   			break;
   
  		case PSKEY_LED_EVENTS:
   			key.length = csr_pioneer_default_config.led_events->length;
   			key.data = (uint16*)csr_pioneer_default_config.led_events->value;
   			break;
   
  		case PSKEY_EVENTS_A:
   			key.length = csr_pioneer_default_config.events_a->length;
   			key.data = (uint16*)csr_pioneer_default_config.events_a->value;
   			break;
   
  		case PSKEY_EVENTS_B:
   			key.length = csr_pioneer_default_config.events_b->length;
   			key.data = (uint16*)csr_pioneer_default_config.events_b->value;
   			break;
   
  		case PSKEY_NO_TONES:
   			key.length = csr_pioneer_default_config.no_tone_events->length;
   			key.data = (uint16*)csr_pioneer_default_config.no_tone_events->value;
   			break;
   
  		case PSKEY_TONES:
   			key.length = csr_pioneer_default_config.tones->length;
   			key.data = (uint16*)csr_pioneer_default_config.tones->value;
   			break;
            
        case PSKEY_TIMEOUTS:
        	key.length = csr_pioneer_default_config.timeouts_config->length;
   			key.data = (uint16*)csr_pioneer_default_config.timeouts_config->value;
   			break; 
			
		case PSKEY_AMP:
        	key.length = csr_pioneer_default_config.amp_config->length;
   			key.data = (uint16*)csr_pioneer_default_config.amp_config->value;
   			break; 

		case PSKEY_VOLUME_GAINS:
        	key.length = csr_pioneer_default_config.vol_gains->length;
   			key.data = (uint16*)csr_pioneer_default_config.vol_gains->value;
   			break; 
			
		case PSKEY_FEATURES:
			key.length = csr_pioneer_default_config.features->length;
   			key.data = (uint16*)csr_pioneer_default_config.features->value;
   			break;
                
        case PSKEY_SSR_PARAMS:
            key.length = csr_pioneer_default_config.ssr_config->length;
            key.data = (uint16*)csr_pioneer_default_config.ssr_config->value;
            break;

  		default:
   			break;
 	}

 	return key;
}


/*****************************************************************************/
uint16 ConfigRetrieve(uint16 key_id, void* data, uint16 len)
{
 	uint16 ret_len;
 
 	 	/* Read requested key from PS if it exists */
 	ret_len = PsRetrieve(key_id, data, len);
 
 	/* If no key exists then read the parameters from the default configuration
       held in constant space */
 	if(!ret_len)
 	{
		/* Read the key */
		key_type key = getkey(key_id);
		
		/* Providing the requested length matches the entry in constant space */
		if(key.length == len)
		{
			/* Copy from constant space */
			memcpy(data, key.data, len);
			ret_len = len;
		}
 	}
 
 	return ret_len;
}
