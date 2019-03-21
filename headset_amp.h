/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_amp.h
@brief    Interface to headset functions which control the Audio Amp.
*/

#include "headset_private.h"

#ifndef _HEADSET_AMP_H_
#define _HEADSET_AMP_H_


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
     AmpOn
    
DESCRIPTION
     Switches the audio amp on.

*/
void AmpOn ( hsTaskData * pApp );


/*************************************************************************
NAME    
     AmpOff
    
DESCRIPTION
     Switches the audio amp off.

*/
void AmpOff ( hsTaskData * pApp );


/*************************************************************************
NAME    
     AmpOffLater
    
DESCRIPTION
     Switches the audio amp off after the configured delay.

*/
void AmpOffLater ( hsTaskData * pApp );


/*************************************************************************
NAME    
     AmpSetOffDelay
    
DESCRIPTION
     Sets the delay before the audio amp is switched off. So when AmpOffLater is called
	 there is this delay before it is physically switched off.

*/
void AmpSetOffDelay ( hsTaskData * pApp, uint16 delay );


/*************************************************************************
NAME    
     AmpSetPio
    
DESCRIPTION
     Sets the PIO that the audio amp is attached to.

*/
void AmpSetPio ( hsTaskData * pApp, uint16 pio );


#endif /* _HEADSET_AMP_H_ */
