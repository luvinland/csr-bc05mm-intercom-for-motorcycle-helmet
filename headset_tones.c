/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_tones.c
@brief    Module responsible for tone generation and playback.
*/

#include "headset_amp.h"
#include "headset_debug.h"
#include "headset_tones.h"

#include <audio.h>
#if 0 /* Jace_Test */
#include <csr_cvsd_8k_cvc_1mic_headset_plugin.h>
#endif
#include <stream.h>
#include <panic.h>

#ifdef DEBUG_TONES
    #define TONE_DEBUG(x) DEBUG(x)
#else
    #define TONE_DEBUG(x) 
#endif

#define TONE_TYPE_RING (0x60FF)


/****************************************************************/
/*
    SIMPLE TONES
 */
/****************************************************************/

/* eg. power tone */
static const audio_note tone_power[] =
{
    AUDIO_TEMPO(120), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, HEMIDEMISEMIQUAVER),        
    AUDIO_NOTE(G7,   CROTCHET), 
    
    AUDIO_END
};

/* eg. pairing tone */
static const audio_note tone_pairing[] =
{
    AUDIO_TEMPO(2400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G5 , SEMIBREVE),
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(REST, QUAVER),
    AUDIO_NOTE(G5 , SEMIBREVE),
    
    AUDIO_END
};

/* eg. mute off */
static const audio_note tone_inactive[] =
{
    AUDIO_TEMPO(2400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G6 , SEMIBREVE),
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(C7 , SEMIBREVE),
    
    AUDIO_END
};

/* eg. mute on */
static const audio_note tone_active[] =
{
    AUDIO_TEMPO(2400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G6 , SEMIBREVE),
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G5 , SEMIBREVE),
    
    AUDIO_END
};

/* eg. battery low */
static const audio_note tone_battery[] =
{
    AUDIO_TEMPO(120), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, HEMIDEMISEMIQUAVER),
    AUDIO_NOTE(G6 , CROTCHET),
    AUDIO_NOTE(REST, HEMIDEMISEMIQUAVER),
    AUDIO_NOTE(G6 , CROTCHET),
    AUDIO_NOTE(REST, HEMIDEMISEMIQUAVER),
    AUDIO_NOTE(G6 , CROTCHET),
    
    AUDIO_END
};

/* eg. vol limit */
static const audio_note tone_vol[] =
{
    AUDIO_TEMPO(600), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    AUDIO_NOTE(REST, SEMIQUAVER),
    AUDIO_TEMPO(200), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    AUDIO_NOTE(G7 , CROTCHET),
    
    AUDIO_END
};

/* eg. connection */
static const audio_note tone_connection[] =
{
    AUDIO_TEMPO(2400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G7 , SEMIBREVE),
    
    AUDIO_END
};

/* error tone */
static const audio_note tone_error[] =
{
    AUDIO_TEMPO(120), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, HEMIDEMISEMIQUAVER),
    AUDIO_NOTE(G5 , CROTCHET),
    
    AUDIO_END
};

/* short confirmation */
static const audio_note tone_short[] =
{
    AUDIO_TEMPO(2400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G6 , SEMIBREVE),
    
    AUDIO_END
};

/* long confirmation */
static const audio_note tone_long[] =
{
    AUDIO_TEMPO(1200), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),  
    AUDIO_NOTE(REST, QUAVER),
    AUDIO_TEMPO(150), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine), 
    AUDIO_NOTE(G6 , MINIM),
    
    AUDIO_END
};

#ifdef FAVORITES_CALL
static const audio_note tone_mute_reminder[] =
{
    AUDIO_TEMPO(1), AUDIO_VOLUME(128), AUDIO_TIMBRE(sine),
    AUDIO_NOTE(G7 , SEMIBREVE), 

    AUDIO_END
};
#else
static const audio_note tone_mute_reminder[] =
{
    AUDIO_TEMPO(600), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    AUDIO_NOTE(REST, SEMIQUAVER),
    AUDIO_TEMPO(120), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    AUDIO_NOTE(G5, CROTCHET),
    AUDIO_NOTE(REST, CROTCHET),
    AUDIO_NOTE(G5, CROTCHET),
    AUDIO_END
};
#endif

/* ringtone 1 */
static const audio_note ring_twilight[] =
{
    AUDIO_TEMPO(180), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
    
    AUDIO_NOTE(E7, QUAVER),
    AUDIO_NOTE(F7, QUAVER),
    AUDIO_NOTE(E7, QUAVER),
    AUDIO_NOTE(C7, QUAVER),
    AUDIO_NOTE(E7, QUAVER),
    AUDIO_NOTE(F7, QUAVER),
    AUDIO_NOTE(E7, QUAVER),
    AUDIO_NOTE(C7, QUAVER),

    AUDIO_END
};

/* ringtone 2 */
static const audio_note ring_greensleeves[] =
{
    AUDIO_TEMPO(400), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),
              
    AUDIO_NOTE(F6,CROTCHET),                                  
    AUDIO_NOTE(AF6,MINIM),                                            
    AUDIO_NOTE(BF6,CROTCHET),                         
    AUDIO_NOTE(C7,CROTCHET),                          
    AUDIO_NOTE_TIE(C7,QUAVER),                                            
    AUDIO_NOTE(DF7,QUAVER),                           
    AUDIO_NOTE(C7,CROTCHET),                                          
    AUDIO_NOTE(BF6,MINIM),                            
    AUDIO_NOTE(G6,CROTCHET),          
    AUDIO_NOTE(EF6,CROTCHET), 
    AUDIO_NOTE_TIE(EF6,QUAVER),
       
    AUDIO_END
};

/* ringtone 3 */
static const audio_note ring_major_scale[] =
{
    AUDIO_TEMPO(300), AUDIO_VOLUME(64), AUDIO_TIMBRE(sine),

    AUDIO_NOTE(E6,QUAVER),                                    
    AUDIO_NOTE(FS6,QUAVER),                                           
    AUDIO_NOTE(GS6,QUAVER),                           
    AUDIO_NOTE(A6,QUAVER),                            
    AUDIO_NOTE(B6,QUAVER),                                            
    AUDIO_NOTE(CS7,QUAVER),                           
    AUDIO_NOTE(DS7,QUAVER),                                           
    AUDIO_NOTE(E7,QUAVER),    

    AUDIO_END
};

/***************************************************************************/
/*
    The Tone Array
*/
/*************************************************************************/
#define NUM_FIXED_TONES (14)


/* This must make use of all of the defined tones - requires the extra space first */
static const audio_note * const gFixedTones [ NUM_FIXED_TONES ] = 
{
/*1*/    tone_power,
/*2*/    tone_pairing,
/*3*/    tone_inactive,
/*4*/    tone_active,
/*5*/    tone_battery,
/*6*/    tone_vol,
/*7*/    tone_connection, 
/*8*/    tone_error,
/*9*/    tone_short,
/*a*/    tone_long,		
/*b*/	 tone_mute_reminder,
/*c*/    ring_twilight,
/*d*/    ring_greensleeves,
/*e*/    ring_major_scale
};    

/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    IsToneDefined
    
DESCRIPTION
  	Helper fn to determine if a tone has been defined or not.
    
RETURNS
    bool 
*/
static bool IsToneDefined ( HeadsetTone_t pTone )
{
    bool lResult = TRUE ;

    if ( pTone == TONE_NOT_DEFINED )
    {
        lResult = FALSE ;
    }
    
    if (  ! gFixedTones [ (pTone - 1) ] )
    {	    /*the tone is also not defined if no entry exists for it*/
        lResult = FALSE ; 
    }
    
    return lResult ;
}


/*****************************************************************************/
uint16 TonesInit ( hsTaskData * pApp ) 
{
    uint16 lEvent = 0 ;
    
    uint16 lSize = 0 ;
    
    TONE_DEBUG (("TONE Init :\n")) ;  
    
    /*need to create the space for the event mapping and the vol tone mapping*/
    lSize = (EVENTS_MAX_EVENTS * sizeof ( HeadsetTone_t ) ) ;
    
    TONE_DEBUG(("TONE: sz[%x] \n",lSize)) ;
    
        /*allocate the total space*/
    pApp->gEventTones = ( HeadsetTone_t * ) PanicUnlessMalloc ( lSize ) ;
    
    for ( lEvent =  0 ; lEvent  < EVENTS_MAX_EVENTS ; lEvent ++ )
    {
        pApp->gEventTones[ lEvent ] = TONE_NOT_DEFINED ;
    }
    
    return lSize;
}


/*****************************************************************************/
void TonesConfigureEvent ( hsTaskData * pApp ,headsetEvents_t pEvent , HeadsetTone_t pTone ) 
{
    uint16 lEventIndex = pEvent - EVENTS_EVENT_BASE ;   

    if ( IsToneDefined ( pTone ) ) 
    {
        if (pEvent == TONE_TYPE_RING )
        {   
            TONE_DEBUG(("TONE: ConfRingTone [%x]\n" , pTone)) ;
            pApp->RingTone = pTone ;
        }
        else
        {
                /* gEventTones is an array of indexes to the tones*/
            TONE_DEBUG(("Add Tone[%x][%x]\n" , pEvent , pTone ));
   
            pApp->gEventTones [ lEventIndex ] = pTone  ;
    
            TONE_DEBUG(("TONE; Add Ev[%x][%x]Tone[%x][%x]\n", pEvent , lEventIndex , pTone 
                                 , (int) pApp->gEventTones [ lEventIndex] )) ;
        }
    }    
}


/*****************************************************************************/
void TonesPlayEvent ( hsTaskData * pApp,  headsetEvents_t pEvent )
{    
    uint16 lEventIndex = pEvent - EVENTS_EVENT_BASE ;
    
        /*if the tone is present*/
    if (pApp->gEventTones [ lEventIndex ] != TONE_NOT_DEFINED )
    {
        TONE_DEBUG(("TONE: EvPl[%x][%x][%x]\n", pEvent, lEventIndex , (int) pApp->gEventTones [ lEventIndex]  )) ;
        
	    TonesPlayTone (pApp ,  pApp->gEventTones [ lEventIndex] ,TRUE ) ;
    }
    else
    {
        TONE_DEBUG(("TONE: NoPl[%x]\n", pEvent)) ;
    }        
} 


/*****************************************************************************/
void TonesPlayTone ( hsTaskData * pApp , HeadsetTone_t pTone , bool pCanQueue )
{
    if ( IsToneDefined(pTone) )			
    {   
        uint16 lToneVolume;
		uint16 ampOffDelay = pApp->ampOffDelay;
		uint16 newDelay = ampOffDelay;
		
		/* Turn the audio amp on */
		AmpOn(pApp);

		/* The audio off delay must be greater than all tone lengths, so if it's
		   set very low then increase it slightly while a tone is played as 
		   otherwise the amp could be turned off before a tone has complete.
		*/
		if (pApp->dsp_process == dsp_process_none)
		{
			/* Only switch amp off if SCO and A2DP not active */
    		if (pApp->ampAutoOff && (ampOffDelay < 3))
    		{
    			newDelay += 3; 
    			AmpSetOffDelay(pApp, newDelay);
    			AmpOffLater(pApp);
    			AmpSetOffDelay(pApp, ampOffDelay);
    		}
    		else
    		{
    			AmpOffLater(pApp);
    		}
		}

#ifndef AUTO_MIC_DETECT
        /* v091111 Release */
        if(pApp->extmic_evt && pApp->extmic_mode)
        {
            lToneVolume = 30;
        }
        else
#endif
        if(pApp->dsp_process)
        {
            lToneVolume = 10; /* ±èÁ¤¹Î 10->3 */
        }
        else
        {
            lToneVolume = 16; /* ±èÁ¤¹Î 16->8 */
        }

#if 0 /* GOLDWING_v101020 */
        if(pTone == 12 || pTone == 13)
        {
            if(pApp->dsp_process == dsp_process_sco) lToneVolume = 22;
            else lToneVolume = 30;
        }
#endif

#ifndef AUTO_MIC_DETECT
        pApp->extmic_evt = FALSE;
#endif

        AudioPlayTone ( gFixedTones [ pTone  - 1 ] , pCanQueue ,pApp->theCodecTask, lToneVolume , TRUE ) ;
    }    
}


/*****************************************************************************/
void ToneTerminate ( hsTaskData * pApp )
{
    AudioStopTone() ;
}  

