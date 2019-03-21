/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_amp.c        
@brief    Implementation of the audio amp control functionality.
*/

/****************************************************************************
    Header files
*/

#include "headset_amp.h"
#include "headset_debug.h"

#include <pio.h>


#ifdef DEBUG_AMP
    #define AMP_DEBUG(x) DEBUG(x)    
#else
    #define AMP_DEBUG(x) 
#endif   


/****************************************************************************
 	Enable or disable the audio amp.
*/
static void SET_AMP ( hsTaskData * pApp, bool on )
{		
	uint32 pio = (uint32)1 << pApp->ampPio;
	
	PioSetDir32(pio, pio); 
	
	if (on)
		PioSet32(pio, pio);
	else
		PioSet32(pio, 0);
}


/****************************************************************************
  FUNCTIONS
*/

/**************************************************************************/
void AmpOn ( hsTaskData * pApp )
{
	if (pApp->useAmp)
	{
		AMP_DEBUG(("Amp: Cancel switch off\n"));
		MessageCancelAll(&pApp->task , APP_AMP_OFF);
					 
		if (!pApp->ampOn)
		{
			AMP_DEBUG(("Amp: Switch on\n"));
			SET_AMP(pApp, TRUE);
			pApp->ampOn = TRUE;
		}
	}
}


/**************************************************************************/
void AmpOff ( hsTaskData * pApp )
{
	if (pApp->useAmp & pApp->ampOn)
	{
		AMP_DEBUG(("Amp: Switch off\n"));
		SET_AMP(pApp, FALSE);
		pApp->ampOn = FALSE;
	}
}


/**************************************************************************/
void AmpOffLater ( hsTaskData * pApp )
{
	if (pApp->useAmp & pApp->ampAutoOff & pApp->ampOn)
	{
		AMP_DEBUG(("Amp: Switch off later\n"));
		MessageSendLater(&pApp->task , APP_AMP_OFF , 0 , D_SEC(pApp->ampOffDelay));
	}
}		


/**************************************************************************/
void AmpSetOffDelay ( hsTaskData * pApp, uint16 delay )
{
	AMP_DEBUG(("Amp: Set off delay %d secs\n",delay));
	pApp->ampOffDelay = delay;
}


/**************************************************************************/
void AmpSetPio ( hsTaskData * pApp, uint16 pio )
{
	AMP_DEBUG(("Amp: PIO used for amp: %d\n",pApp->useAmp));
	AMP_DEBUG(("Amp: Set PIO %d for amp\n",pio));
	pApp->ampPio = pio;
}
