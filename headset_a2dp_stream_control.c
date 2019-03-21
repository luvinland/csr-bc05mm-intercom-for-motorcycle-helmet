/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_a2dp_stream_control.c
@brief    Implementation of A2DP streaming controls.
*/

/****************************************************************************
    Header files
*/


#include "headset_a2dp_stream_control.h"
#include "headset_amp.h"
#include "headset_avrcp_event_handler.h"
#include "headset_debug.h"
#include "headset_hfp_slc.h"
#include "headset_init.h"
#include "headset_statemanager.h"
#include "headset_volume.h"

#include <audio.h>
#include <bdaddr.h>

#ifdef DEBUG_A2DP_STREAM_CONTROL
    #define STREAM_DEBUG(x) DEBUG(x)    
#else
    #define STREAM_DEBUG(x) 
#endif   


#ifdef R100 /* v091221 */
#include <pio.h>
#ifdef Z100_CLASS1
#define AMP_GAIN_MASK ((uint16)1 << 13)
#else
#define AMP_GAIN_MASK ((uint16)1 << 0)
#endif
#endif


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/



/****************************************************************************
  FUNCTIONS
*/


/**************************************************************************/
void streamControlCeaseA2dpStreaming(hsTaskData *app, bool send_suspend)
{				  
	STREAM_DEBUG(("streamControlCeaseA2dpStreaming %d\n",send_suspend));
	
	MessageCancelAll(&app->task, APP_RESUME_A2DP);

	if (app->dsp_process == dsp_process_a2dp)
	{		
		AudioDisconnect();
		app->dsp_process = dsp_process_none;
		STREAM_DEBUG(("CeaseStreaming - disconnect audio\n"));

		/* Turn the audio amp off after a delay */
		AmpOffLater(app);
	
    	if (A2dpGetMediaSink(app->a2dp) && send_suspend && (stateManagerIsA2dpStreaming()))
    	{
			STREAM_DEBUG(("CeaseStreaming - suspend audio\n"));
        	A2dpSuspend(app->a2dp);

			if (!IsA2dpSourceAnAg(app))
			{
				if (stateManagerGetA2dpState() == headsetA2dpPaused)
				{
					/* Ensure music does not resume erroneously when the call ends */
					app->sendPlayOnConnection = FALSE;
				}
				else
				{
					/* Only send Pause if headset is currently playing music */
					if (app->autoSendAvrcp)
     					avrcpSendPause(app);
     				/* Ensure music is resumed when the call ends */
     				app->sendPlayOnConnection = TRUE;
 				}
				stateManagerEnterA2dpPausedState(app);
			}
    	}
	}
}


/**************************************************************************/
void streamControlConnectA2dpAudio(hsTaskData *app)
{	
	bool result;
#ifdef R100 /* v091221 */
	AUDIO_MODE_T mode = AUDIO_MODE_CONNECTED;
#else
    AUDIO_MODE_T mode = 0;
#endif
	Task audio_plugin;
	uint32 rate = 0;

	STREAM_DEBUG(("streamControlConnectA2dpAudio vol index[%d] vol gain[%d]\n",app->gAvVolumeLevel, VolumeRetrieveGain(app->gAvVolumeLevel, TRUE)));
	/* Turn the audio amp on */
	AmpOn(app);
	
	audio_plugin = InitA2dpPlugin(app->seid);
	
	switch (app->a2dp_rate)
	{
	case a2dp_rate_48_000k:
		rate = 48000;
		break;
	case a2dp_rate_44_100k:
		rate = 44100;
		break;
	case a2dp_rate_32_000k:
		rate = 32000;
		break;
	case a2dp_rate_24_000k:
		rate = 24000;
		break;
	case a2dp_rate_22_050k:
		rate = 22050;
		break;
	case a2dp_rate_16_000k:
	default:
		rate = 16000;
		break;
	}

#if R100 /* v091221 */
    if(app->gAvVolumeLevel > 4)
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

    /* v091111 Release */
    result = AudioConnect(audio_plugin,
                            A2dpGetMediaSink(app->a2dp),
                            AUDIO_SINK_AV,
                            app->theCodecTask,
                            VolumeRetrieveGain(app->gAvVolumeLevel, TRUE),
                            rate,
                            TRUE,
                            mode,
                            (sink_codec_data_type *)&app->a2dp_data.codecData,
                            &app->task); /* Jace_Test */

	app->dsp_process = dsp_process_a2dp;
}


/**************************************************************************/
void streamControlResumeA2dpStreaming(hsTaskData *app, uint32 user_delay)
{				  
	STREAM_DEBUG(("streamControlResumeA2dpStreaming\n"));
	MessageCancelAll(&app->task, APP_RESUME_A2DP);
	if (!user_delay)
   		MessageSendLater(&app->task, APP_RESUME_A2DP, 0, A2DP_RESTART_DELAY);           
	else
		MessageSendLater(&app->task, APP_RESUME_A2DP, 0, user_delay);           
}


/**************************************************************************/
void streamControlCancelResumeA2dpStreaming(hsTaskData *app)
{
	STREAM_DEBUG(("streamControlCancelResumeA2dpStreaming\n"));
	MessageCancelAll(&app->task, APP_RESUME_A2DP);
}


/**************************************************************************/
void streamControlBeginA2dpStreaming(hsTaskData *app)
{				  
	if (HfpGetAudioSink(app->hfp_hsp) || (!A2dpGetMediaSink(app->a2dp)))
		return;

	STREAM_DEBUG(("streamControlBeginA2dpStreaming\n"));
	if (!stateManagerIsA2dpStreaming() && app->a2dpSourceSuspended)
	{
		STREAM_DEBUG(("Begin Streaming - start A2DP\n"));
		streamControlStartA2dp(app);			
	}
	
	if (stateManagerIsA2dpStreaming() && (app->dsp_process != dsp_process_a2dp))
	{
		STREAM_DEBUG(("Begin Streaming - connect audio\n"));
		streamControlConnectA2dpAudio(app);
	}
}


/**************************************************************************/
void streamControlStartA2dp(hsTaskData *app)
{
	if (A2dpGetMediaSink(app->a2dp) && app->a2dpSourceSuspended && !HfpGetAudioSink(app->hfp_hsp))
    {
		STREAM_DEBUG(("streamControlStartA2dp\n"));

        A2dpStart(app->a2dp);
		if (!IsA2dpSourceAnAg(app) && (app->sendPlayOnConnection))
		{
			if (app->autoSendAvrcp)
     			avrcpSendPlay(app);
     		app->sendPlayOnConnection = FALSE;
 		}
		app->a2dpSourceSuspended = FALSE;
	}
}


/*****************************************************************************/
bool IsA2dpSourceAnAg(hsTaskData *app)
{
    bdaddr bdaddr_sig, bdaddr_slc;
    
    if (!SinkGetBdAddr(A2dpGetSignallingSink(app->a2dp), &bdaddr_sig))
		return FALSE;
    if (!SinkGetBdAddr(HfpGetSlcSink(app->hfp_hsp), &bdaddr_slc))
		return FALSE;
    if (BdaddrIsSame(&bdaddr_sig, &bdaddr_slc))
        return TRUE;
    
    return FALSE;
}

