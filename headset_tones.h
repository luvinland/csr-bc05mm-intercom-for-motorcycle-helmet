/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_tones.h
@brief    Header file which defines all of the tones which can be used in the headset.
*/  

#ifndef HEADSET_TONES_H
#define HEADSET_TONES_H


#include "headset_private.h"



/****************************************************************************
DESCRIPTION
  	Identifiers for standard tones.
*/
enum
{
	tone_id_power = 1,
	tone_id_pairing,
	tone_id_inactive,
	tone_id_active,
	tone_id_battery,
	tone_id_vol,
	tone_id_connection, 
	tone_id_error,
	tone_id_short,
	tone_id_long,
	ring_id_twilight,
	ring_id_greensleeves,
	ring_id_major_scale
};    


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    TonesInit
    
DESCRIPTION
  	Init the tones.
    
RETURNS
	Size of allocation used for tones.
*/
uint16 TonesInit ( hsTaskData * pApp ) ;


/****************************************************************************
NAME    
    TonesConfigureEvent
    
DESCRIPTION
  	Configures a tones event mapping - used to playback a tone.
    
*/
void TonesConfigureEvent ( hsTaskData * pApp ,headsetEvents_t pEvent , HeadsetTone_t pTone );


/****************************************************************************
NAME 
    TonesPlayEvent

DESCRIPTION
    Function to indaicate an event by playing its associated tone uses underlying
    tones playback.

*/
void TonesPlayEvent ( hsTaskData * pApp,  headsetEvents_t pEvent );


/****************************************************************************
NAME    
    TonesPlayTone
    
DESCRIPTION
  	Playback the tone given by the headsetTone_t index.
    
*/
void TonesPlayTone ( hsTaskData * pApp , HeadsetTone_t pTone , bool pCanQueue );


/****************************************************************************
NAME    
    ToneTerminate
    
DESCRIPTION
  	Function to terminate a ring tone prematurely.
    
*/
void ToneTerminate ( hsTaskData * pApp );


#endif
