/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_volume.c
@brief    Module responsible for Vol control 
*/

#include "headset_debug.h"
#include "headset_LEDmanager.h"
#include "headset_volume.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_configmanager.h"

#include <stdlib.h>
#include <audio.h>
#include <ps.h>

#ifdef R100 /* v091221 */
#include <pio.h>
#ifdef Z100_CLASS1
#define AMP_GAIN_MASK ((uint16)1 << 13)
#else
#define AMP_GAIN_MASK ((uint16)1 << 0)
#endif
#endif

#ifdef DEBUG_VOLUME
#define VOL_DEBUG(x) DEBUG(x)
#else
#define VOL_DEBUG(x) 
#endif

typedef struct
{
	unsigned int hfpVol:8;
	unsigned int avVol:8;
} vol_table_t;

vol_table_t *gVolLevels = NULL;

/*****************************************************************************/
void VolumeInit ( hsTaskData * pApp ) 
{
	uint16 psVolume = 0;
	VOL_DEBUG(("VOL: Init Volume\n"));

	if (PsRetrieve(PSKEY_VOLUME_LEVELS, &psVolume, sizeof(uint16)))
	{
		pApp->gHfpVolumeLevel = psVolume & 0x1f; /* Field is 5 Bits long */
		pApp->gAvVolumeLevel = (psVolume >> 8) & 0x1f; /* Field is 5 Bits long */
		
		if (pApp->gHfpVolumeLevel > VOL_MAX_VOLUME_LEVEL)
		{
		    pApp->gHfpVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
		}
		if (pApp->gAvVolumeLevel > VOL_MAX_VOLUME_LEVEL)
		{
		    pApp->gAvVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
		}
	}
	else
	{
	    pApp->gHfpVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
	    pApp->gAvVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
	}
	pApp->gMuted = FALSE;
	
	gVolLevels = (vol_table_t*)PanicUnlessMalloc(sizeof(vol_table_t) * (VOL_MAX_VOLUME_LEVEL+1));
	configManagerSetupVolumeGains((uint16*)gVolLevels, VOL_MAX_VOLUME_LEVEL+1);
}


/*****************************************************************************/
void VolumeInitHfp ( hsTaskData * pApp ) 
{
	uint16 psVolume = 0;
	VOL_DEBUG(("VOL: Init HFP Volume\n"));

	if (PsRetrieve(PSKEY_VOLUME_LEVELS, &psVolume, sizeof(uint16)))
	{
		pApp->gHfpVolumeLevel = psVolume & 0x5f; /* Field is 5 Bits long */
		
		if (pApp->gHfpVolumeLevel > VOL_MAX_VOLUME_LEVEL)
		{
		    pApp->gHfpVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
		}
	}
	else
	{
	    pApp->gHfpVolumeLevel = VOL_DEFAULT_VOLUME_LEVEL ; 
	}
	pApp->gMuted = FALSE;
}


/*****************************************************************************/
void VolumeUp ( hsTaskData * pApp ) /* R100 */
{
	uint16 actVol = 0;
	bool avAudio = FALSE;

    VOL_DEBUG(("VOL: VolUp\n"));

    if (!VolumeGetHeadsetVolume( pApp, &actVol, &avAudio ))
	    return;

    /* Volume change's beep occur */
    MessageSend(&pApp->task, EventVolumeUp, 0);

    if (actVol < VOL_MAX_VOLUME_LEVEL)
    {
        actVol++;
        VolumeSetHeadsetVolume(  pApp, actVol, avAudio );
    }
    else
    {
        MessageSend ( &pApp->task , EventVolumeMax , 0 );
        VOL_DEBUG(("VOL:      Max. volume reached\n"));
    }
}


/*****************************************************************************/
void VolumeDown ( hsTaskData * pApp ) /* R100 */
{
	uint16 actVol = 0;
	bool avAudio = FALSE;
	
    VOL_DEBUG(("VOL: VolDwn\n"))  ;
    
    if (!VolumeGetHeadsetVolume( pApp, &actVol, &avAudio ))
	    return;

    /* Volume change's beep occur */ /* R100 */
    MessageSend(&pApp->task, EventVolumeDown, 0);

    if (actVol > 0)
    {
        actVol--;
	
	    VolumeSetHeadsetVolume(  pApp, actVol, avAudio );
	}
    else
    {
	    MessageSend ( &pApp->task , EventVolumeMin , 0 );
	    VOL_DEBUG(("VOL:      Min. Volume reached\n"));
    }
}


/*****************************************************************************/
void VolumeStoreLevels ( hsTaskData * pApp )
{
	uint16 psVolume = 0;
	
	VOL_DEBUG(("VOL: Store Volume Levels\n"));

	psVolume |= pApp->gHfpVolumeLevel; /* Field is 5 Bits long */
	psVolume |= pApp->gAvVolumeLevel << 8; /* Field is 5 Bits long */
	
	if (!PsStore(PSKEY_VOLUME_LEVELS, &psVolume, sizeof(uint16)))
	{
		VOL_DEBUG(("VOL:    Can not store volume\n"));
	}
}


/*****************************************************************************/
bool VolumeGetHeadsetVolume(hsTaskData * pApp, uint16 * actVol, bool * avAudio)
{
	if (stateManagerIsA2dpStreaming())
    {
	    *actVol = pApp->gAvVolumeLevel;
	    *avAudio = TRUE;
    }
    else if (stateManagerIsHfpConnected())
    {
	    *actVol = pApp->gHfpVolumeLevel;
    }
    /* Insert code for Intercom by Jace */
    else if(pApp->aghfp_connect || pApp->audio_connect)
    {
        *actVol = pApp->gHfpVolumeLevel;
    }
    else
    {
	    VOL_DEBUG(("VOL:      No Active audio\n"));
	    return FALSE;
    }
	
	return TRUE;
}


/*****************************************************************************/
void VolumeSetHeadsetVolume(hsTaskData * pApp, uint16 actVol, bool avAudio)
{
#ifdef R100 /* v091221 */
    if(actVol > 4)
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, AMP_GAIN_MASK);
    }
    else
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, 0);
    }
#endif

	if (avAudio)
	{
	    pApp->gAvVolumeLevel = actVol;
        AudioSetVolume( gVolLevels[actVol].avVol , pApp->theCodecTask ) ; /* v091111 Release */
	}
	else
	{
	    pApp->gHfpVolumeLevel = actVol;
        AudioSetVolume( gVolLevels[actVol].hfpVol, pApp->theCodecTask ) ; /* v091111 Release */
	}
}


/*****************************************************************************/
uint16 VolumeRetrieveGain( uint16 index , bool avAudio )
{
	if (avAudio)
		return gVolLevels[index].avVol;
	else
		return gVolLevels[index].hfpVol;
		
}


