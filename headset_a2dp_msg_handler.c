/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_a2dp_msg_handler.c
@brief    Handle a2dp library messages arriving at the app.
*/

#include "headset_a2dp_connection.h"
#include "headset_a2dp_msg_handler.h"
#include "headset_a2dp_stream_control.h"
#include "headset_avrcp_event_handler.h"
#include "headset_avrcp_msg_handler.h"
#include "headset_debug.h"
#include "headset_hfp_slc.h"
#include "headset_init.h"
#include "headset_link_policy.h"
#include "headset_private.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_configmanager.h"
#include "headset_volume.h" /* R100 */

#include <a2dp.h>
#include <bdaddr.h>
#include <panic.h>
#include <ps.h>

#ifdef DEBUG_A2DP_MSG
#define A2DP_MSG_DEBUG(x) DEBUG(x)
#else
#define A2DP_MSG_DEBUG(x) 
#endif


/****************************************************************************
  ENUM DEFINITIONS
*/


/****************************************************************************
  MESSAGE DEFINITIONS
*/


/****************************************************************************
  LOCAL FUNCTIONS
*/

/****************************************************************************/
static void sendPlayOnAvrcpConnection(hsTaskData *app)
{
	if ( stateManagerIsAvrcpConnected() )
	{
		if (app->autoSendAvrcp)
			avrcpSendPlay(app);
	}
	else
	{
		/* AVRCP is not connected yet, so send an avrcp_play once it is connected */
		if (stateManagerGetAvrcpState() == avrcpConnecting)
		{
			app->avrcp_data.send_play = 1;
			MessageSendConditionally( &app->task , APP_SEND_PLAY, 0, &app->avrcp_data.send_play );
		}
	}
}


/****************************************************************************
  LOCAL MESSAGE HANDLING FUNCTIONS
*/

static void handleA2DPInitCfm(hsTaskData *app, const A2DP_INIT_CFM_T *msg)
{   
    if(msg->status == a2dp_success)
    {
        A2DP_MSG_DEBUG(("Init Success\n"));
        /* A2DP Library initialisation was a success */     
        /* Keep a record of the A2DP SEP pointer. */
		app->a2dp_data.sep_entries = msg->sep_list;
		
		/* Initialise AVRCP */
		InitAvrcp();
    }
    else
    {
	    A2DP_MSG_DEBUG(("Init Failed [Status %d]\n", msg->status));
        Panic();
    }
}


static void handleA2DPOpenInd(hsTaskData *app, Sink sink, uint8 seid)
{
    bdaddr bdaddr_ind;
    
	if (SinkGetBdAddr(sink, &bdaddr_ind))
	{
		avrcpConnectReq(app, bdaddr_ind, FALSE);
	
		/* Store the last used SEP on media connection */
		(void)PsStore(PSKEY_LAST_USED_AV_SOURCE_SEID, &seid, sizeof(uint8));
	    
		app->seid = seid;
		A2DP_MSG_DEBUG(("    Selected SEID = %d\n", seid));
		stateManagerEnterA2dpConnectedState(app);
		PROFILE_MEMORY(("A2DPOpen"))
	}
	else
	{
		A2DP_MSG_DEBUG(("    Can't find BDA associated with sink 0x%x\n", (uint16)sink));
	}
}

static void handleA2DPOpenCfm(hsTaskData *app, a2dp_status_code status, Sink sink, uint8 seid)
{
#ifdef LABRADOR
    bdaddr addr;
#endif
	app->a2dpConnecting = FALSE;
	
	if (status == a2dp_success)
	{
		A2DP_MSG_DEBUG(("Open Success\n"));
		handleA2DPOpenInd(app, sink, seid);

        /* Start the Streaming */
        /*A2dpStart(app->a2dp);*/ /* TTA & A2DP conn */
#ifdef LABRADOR
        if (!SinkGetBdAddr(sink, &addr))
            return;

        if(addr.nap == 0x1f && addr.uap == 0x47) /* Labrador addr */
        {
            /* Start the Streaming */
            A2dpStart(app->a2dp); /* TTA & A2DP conn */
        }
#endif
	}
	else
	{
		A2DP_MSG_DEBUG(("Open Failure [result = %d]\n", status));
		
		/* reset flag as media connection failed */
		app->sendPlayOnConnection = FALSE;
	}
}

static void handleA2DPStartInd(hsTaskData *app, A2DP_START_IND_T *msg)
{
	if (!stateManagerIsA2dpConnected())
	{
		A2DP_MSG_DEBUG(("    Not Connected - Ignoring\n"));
		return;
	}
	
    if (!A2dpGetMediaSink(app->a2dp) || (stateManagerIsA2dpStreaming()))
        return;

    if((app->aghfp_connect && app->audio_connect) || HfpGetAudioSink(app->intercom_hsp)) /* Unknown power off when play smart phone button tone sound during intercom. v110519 */
        return;

#if 0  /* Unknown power off when play smart phone button tone sound during intercom. v110519 */
    if (HfpGetAudioSink(app->hfp_hsp) || hfpSlcIsConnecting(app) || (app->aghfp_connect && app->audio_connect))
#else
    if (HfpGetAudioSink(app->hfp_hsp) || hfpSlcIsConnecting(app))
#endif
    {
        /* SCO is active or currently connecting SLC so don't start AV */
        A2dpSuspend(app->a2dp);
		return;
    }

	streamControlConnectA2dpAudio(app);
	
	/* The A2DP state needs to be set based on what the headset thinks is the playing status of the media */ 
    if (app->PlayingState || IsA2dpSourceAnAg(app))       
		stateManagerEnterA2dpStreamingState(app);
	else
		stateManagerEnterA2dpPausedState(app);
}

static void handleA2DPStartCfm(hsTaskData *app, A2DP_START_CFM_T *msg)
{
    if (msg->status == a2dp_success)
    {   
        A2DP_MSG_DEBUG(("Start Success\n"));
        /* start Kalimba decoding if it isn't already */
        if (!stateManagerIsA2dpStreaming())
        {			
            if (HfpGetAudioSink(app->hfp_hsp) || hfpSlcIsConnecting(app) || (app->aghfp_connect && app->audio_connect))
            {
                /* 
                    SCO has become active while we were waiting for a START_CFM (or SLC
					is connecting).
				    AV doesn't want to be streaming now, so we must try to 
				    suspend the source again.
				*/
                A2dpSuspend(app->a2dp);
                return;
            }

			streamControlConnectA2dpAudio(app);
        } 
		
		/* The A2DP state needs to be set based on what the headset thinks is the playing status of the media */
		if (app->PlayingState || app->sendPlayOnConnection || IsA2dpSourceAnAg(app))       
		    stateManagerEnterA2dpStreamingState(app);
		else
			stateManagerEnterA2dpPausedState(app);
		
		if (app->sendPlayOnConnection)
		{
			sendPlayOnAvrcpConnection(app);
		}
    }
    else
    {
        A2DP_MSG_DEBUG(("Start Failed [Status %d]\n", msg->status));
		
		/* Workaraound for Samsung phones to start audio playing once media is connected.
 		   The phone sends an avdtp_start as soon as it receives the avdtp_open, so the
		   avdtp_start sent from the headset end will fail. But even though it is in the streaming
		   state we still need to send an avrcp_play here to start the music.
		*/
		if (stateManagerIsA2dpStreaming())
		{
			sendPlayOnAvrcpConnection(app);
		}
    }
	/* reset flag regardless of result code */
	app->sendPlayOnConnection = FALSE;
}

static void handleA2DPSuspendInd(hsTaskData *app, A2DP_SUSPEND_IND_T *msg)
{
	if (!stateManagerIsA2dpStreaming())
	{
		A2DP_MSG_DEBUG(("    Not Streaming - Ignoring\n"));
		return;
	}
    stateManagerEnterA2dpConnectedState(app);

    streamControlCeaseA2dpStreaming(app, FALSE);
}

static void handleA2DPSuspendCfm(hsTaskData *app, A2DP_SUSPEND_CFM_T *msg)
{
	if (!stateManagerIsA2dpConnected())
	{
		A2DP_MSG_DEBUG(("    Not Connected - Ignoring\n"));
		return;
	}
    if (msg->status == a2dp_success)
    {
        A2DP_MSG_DEBUG(("Suspend Success\n"));
        
	    if (app->dsp_process == dsp_process_a2dp)
        {
            /* We must have had a stream restart at this end occuring so restart AV source */
            A2dpStart(app->a2dp);
			if (app->autoSendAvrcp)
            	avrcpSendPlay(app);
			return;
        }
		else
        {
            /* We have suspended the A2DP source. */
			app->a2dpSourceSuspended = TRUE;
        }
		
        stateManagerEnterA2dpConnectedState(app);
    }
    else
    {
        A2DP_MSG_DEBUG(("Suspend Failed [Status %d]\n", msg->status));   
    }
}

static void handleA2DPClose(hsTaskData *app)
{
    PROFILE_MEMORY(("A2DPClose"))

#if 0 /* TTA & A2DP conn */
    if (!stateManagerIsA2dpConnected())
    {
        A2DP_MSG_DEBUG(("    Not Connected - Ignoring\n"));
        return;
    }
#endif
	
    streamControlCeaseA2dpStreaming(app,FALSE);
	
	app->a2dpSourceSuspended = FALSE;
	
    /* Change state */	
	if (A2dpGetSignallingSink(app->a2dp))
		stateManagerEnterA2dpConnectedState(app);
	else
		stateManagerEnterA2dpConnectableState(app, FALSE);
    
	app->seid = 0;
}

static void handleA2DPCodecSettingsInd(hsTaskData *app, A2DP_CODEC_SETTINGS_IND_T *msg)
{
	codec_data_type	codecData;
	
	app->a2dp_channel_mode = msg->channel_mode;
	
	switch (msg->rate)
	{
	case 48000:
		app->a2dp_rate = a2dp_rate_48_000k;
		break;
	case 44100:
		app->a2dp_rate = a2dp_rate_44_100k;
		break;
	case 32000:
		app->a2dp_rate = a2dp_rate_32_000k;
		break;
	case 24000:
		app->a2dp_rate = a2dp_rate_24_000k;
		break;
	case 22050:
		app->a2dp_rate = a2dp_rate_22_050k;
		break;
	case 16000:
	default:
		app->a2dp_rate = a2dp_rate_16_000k;
		break;
	}
	
	codecData = msg->codecData;
	
	/* Get rid of the packet_size as it isn't needed for the sink, and we're short of global space */
	app->a2dp_data.codecData.content_protection = codecData.content_protection;
	app->a2dp_data.codecData.voice_rate = codecData.voice_rate;
	app->a2dp_data.codecData.bitpool = codecData.bitpool;
	app->a2dp_data.codecData.format = codecData.format;
	
	A2DP_MSG_DEBUG(("	chn_mode=%d rate=0x%x\n",app->a2dp_channel_mode,app->a2dp_rate));
	A2DP_MSG_DEBUG(("	content_protection=%d voice_rate=0x%lx bitpool=0x%x format=0x%x\n",
																	codecData.content_protection,
																	codecData.voice_rate,
																	codecData.bitpool,
																	codecData.format
																	));

}

static void handleA2DPSignallingConnected(hsTaskData *app, a2dp_status_code status, A2DP *a2dp, Sink sink)
{
	bdaddr bdaddr_ind;
	bool bdaddr_retrieved = FALSE;
    
    app->a2dpConnecting = FALSE;
    
    if (status != a2dp_success)
    {
        A2DP_MSG_DEBUG(("Signalling Failed [Status %d]\n", status));
        /* Send event to signify that reconnection attempt failed */
		MessageSend(&app->task, EventA2dpReconnectFailed, 0);
        return;
    }
	
	/* If there is already a signalling channel connected, then disconnect this new connection */
	if (app->a2dp)
    {      
       A2dpDisconnectAll(a2dp);
	   return;
    }

#if 0 /* v100617 Master & Slave power on at the same time problem */
    A2DP_MSG_DEBUG(("Signalling Success\n"));
    
    app->a2dp = a2dp;
    
    bdaddr_retrieved = SinkGetBdAddr(sink, &bdaddr_ind);
#else
    bdaddr_retrieved = SinkGetBdAddr(sink, &bdaddr_ind);

    if(bdaddr_ind.nap == 0x24 && bdaddr_ind.uap == 0xbc)
    {
        A2DP_MSG_DEBUG(("Signalling Failed [Status %d]\n", status));
        /* Send event to signify that reconnection attempt failed */
        MessageSend(&app->task, EventA2dpReconnectFailed, 0);
        return;
    }

    A2DP_MSG_DEBUG(("Signalling Success\n"));

    app->a2dp = a2dp;
#endif
    
    /* We are now connected */
    if (!stateManagerIsA2dpStreaming())
        stateManagerEnterA2dpConnectedState(app); 	
	
	if (bdaddr_retrieved)
		(void)PsStore(PSKEY_LAST_USED_AV_SOURCE, &bdaddr_ind, sizeof(bdaddr));
	
	/* Ensure the underlying ACL is encrypted */       
    ConnectionSmEncrypt( &app->task , sink , TRUE );
	
	/* If we were in pairing mode then update HFP state also */
	if (stateManagerGetHfpState() == headsetConnDiscoverable)
		stateManagerEnterHfpConnectableState(app, FALSE);
	
	/* If the headset is off then disconnect */
	if (stateManagerGetHfpState() == headsetPoweringOn)
    {      
       a2dpDisconnectRequest( app );
    }
	else
	{
    	/* Establish an AVRCP connection if required */
		if (bdaddr_retrieved)
			avrcpConnectReq(app, bdaddr_ind, TRUE);

		MessageSend ( &app->task , EventA2dpConnected , 0 );
		
		/* Update Link Policy as A2DP has connected. */
		linkPolicyA2dpStateChange(TRUE);
	}
}

static void handleA2DPSignallingDisconnected(hsTaskData *app, A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND_T *msg)
{   
    /* Change to ready state if no media channel open */
    if (!A2dpGetMediaSink(app->a2dp))
	    stateManagerEnterA2dpConnectableState(app, FALSE);
		
	app->a2dp = 0; 
	
	avrcpDisconnectReq(app);
		    
	if (msg->status == a2dp_disconnect_link_loss)
	{
		/* Reconnect on link loss */
		bdaddr ag_addr, a2dp_addr;
		
		A2DP_MSG_DEBUG(("A2DP: Link Loss Detect\n")) ;
               
        MessageSend( &app->task , EventLinkLoss , 0 ) ;
		
		if (app->combined_link_loss)
		{
			/* HFP on this device already suffered link loss so connect HFP first */
			app->slcConnectFromPowerOn = TRUE;
        	MessageSend ( &app->task , EventEstablishSLC , 0 ) ;
		}
		else if (HfpGetSlcSink(app->hfp_hsp) && PsRetrieve(PSKEY_LAST_USED_AG, &ag_addr, sizeof(bdaddr)) && PsRetrieve(PSKEY_LAST_USED_AV_SOURCE, &a2dp_addr, sizeof(bdaddr)))
		{
			if (BdaddrIsSame(&ag_addr, &a2dp_addr))
				/* Flag to indicate that HFP should be reconnected first. It's still connected but should suffer link loss soon. */
				app->combined_link_loss = TRUE;
		}
		else	
        {      
			/* Standard reconnect procedure for A2DP */
            /* for Lock Up side of Stereo Dongle that attemp auto connection */
			/*a2dpReconnectProcedure(app);*/
        }
	}
	else
	{
		app->combined_link_loss = FALSE;
	}
	
	MessageSend ( &app->task , EventA2dpDisconnected , 0 );
	
	/* Update Link Policy as A2DP has disconnected. */
	linkPolicyA2dpStateChange(FALSE);
		
	PROFILE_MEMORY(("A2DPSigClose"))
}

/****************************************************************************
  INTERFACE FUNCTIONS
*/
void handleA2DPMessage( Task task, MessageId id, Message message )
{
    hsTaskData * app = (hsTaskData *) getAppTask() ;

    switch (id)
    {
    case A2DP_INIT_CFM:
        A2DP_MSG_DEBUG(("A2DP_INIT_CFM : \n"));
        handleA2DPInitCfm(app, (A2DP_INIT_CFM_T *) message);
        break;
		
	case A2DP_SIGNALLING_CHANNEL_CONNECT_IND:
        A2DP_MSG_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_IND : \n"));

        if(app->aghfp_connect && app->audio_connect) /* Power OFF occur during Intercom */
        {
            A2DP_MSG_DEBUG(("Reject\n"));
            A2dpConnectSignallingChannelResponse(((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->a2dp,
                                            FALSE,
                                            ((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->connection_id,
                                            app->a2dp_data.sep_entries);
            break;
        }

		if (!stateManagerIsA2dpConnected() && (stateManagerGetHfpState() != headsetPoweringOn))
        {
            A2DP_MSG_DEBUG(("Accept\n"));
			A2dpConnectSignallingChannelResponse(((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->a2dp,
											 TRUE,
											 ((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->connection_id,
											 app->a2dp_data.sep_entries);
        }
		else
        {
            A2DP_MSG_DEBUG(("Reject\n"));
			A2dpConnectSignallingChannelResponse(((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->a2dp,
											 FALSE,
											 ((A2DP_SIGNALLING_CHANNEL_CONNECT_IND_T *)message)->connection_id,
											 app->a2dp_data.sep_entries);
        }
		break;
		
	case A2DP_SIGNALLING_CHANNEL_CONNECT_CFM:
        A2DP_MSG_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_CFM : \n"));
		handleA2DPSignallingConnected(app, ((A2DP_SIGNALLING_CHANNEL_CONNECT_CFM_T*)message)->status, ((A2DP_SIGNALLING_CHANNEL_CONNECT_CFM_T*)message)->a2dp, ((A2DP_SIGNALLING_CHANNEL_CONNECT_CFM_T*)message)->sink);
		break;
        
    case A2DP_OPEN_IND:
        A2DP_MSG_DEBUG(("A2DP_OPEN_IND : \n"));
    	handleA2DPOpenInd(app, ((A2DP_OPEN_IND_T*)message)->media_sink, ((A2DP_OPEN_IND_T*)message)->seid);
        break;
    case A2DP_OPEN_CFM:
        A2DP_MSG_DEBUG(("A2DP_OPEN_CFM : \n"));
    	handleA2DPOpenCfm(app, ((A2DP_OPEN_CFM_T*)message)->status, ((A2DP_OPEN_CFM_T*)message)->media_sink, ((A2DP_OPEN_CFM_T*)message)->seid);
        break;
        
    case A2DP_CONNECT_OPEN_CFM:
        A2DP_MSG_DEBUG(("A2DP_CONNECT_OPEN_CFM : \n"));
        handleA2DPSignallingConnected(app, ((A2DP_CONNECT_OPEN_CFM_T*)message)->status, ((A2DP_CONNECT_OPEN_CFM_T*)message)->a2dp, ((A2DP_CONNECT_OPEN_CFM_T*)message)->signalling_sink);
    	handleA2DPOpenCfm(app, ((A2DP_CONNECT_OPEN_CFM_T*)message)->status, ((A2DP_CONNECT_OPEN_CFM_T*)message)->media_sink, ((A2DP_CONNECT_OPEN_CFM_T*)message)->seid);
        break;
                    
    case A2DP_START_IND:
        A2DP_MSG_DEBUG(("A2DP_START_IND : \n"));
    	handleA2DPStartInd(app, (A2DP_START_IND_T*)message);
        break;
    case A2DP_START_CFM:
        A2DP_MSG_DEBUG(("A2DP_START_CFM : \n"));
    	handleA2DPStartCfm(app, (A2DP_START_CFM_T*)message);
        break;
        
    case A2DP_SUSPEND_IND:
        A2DP_MSG_DEBUG(("A2DP_SUSPEND_IND : \n"));
    	handleA2DPSuspendInd(app, (A2DP_SUSPEND_IND_T*)message);
        break;
    case A2DP_SUSPEND_CFM:
        A2DP_MSG_DEBUG(("A2DP_SUSPEND_CFM : \n"));
    	handleA2DPSuspendCfm(app, (A2DP_SUSPEND_CFM_T*)message);
        break;
        
    case A2DP_CLOSE_IND:
        A2DP_MSG_DEBUG(("A2DP_CLOSE_IND : \n"));
        handleA2DPClose(app);
        break;
        
    case A2DP_CLOSE_CFM:
        A2DP_MSG_DEBUG(("A2DP_CLOSE_CFM : \n"));
    	handleA2DPClose(app);
        break;
        
    case A2DP_CODEC_SETTINGS_IND:
        A2DP_MSG_DEBUG(("A2DP_CODEC_SETTINGS_IND : \n"));
    	handleA2DPCodecSettingsInd(app, (A2DP_CODEC_SETTINGS_IND_T*)message);
        break;
            
	case A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND:
        A2DP_MSG_DEBUG(("A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND : \n"));
    	handleA2DPSignallingDisconnected(app, (A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND_T*)message);
		break;
		
	case A2DP_ENCRYPTION_CHANGE_IND:
        A2DP_MSG_DEBUG(("A2DP_ENCRYPTION_CHANGE_IND : \n"));
		break;
			
    default:       
		A2DP_MSG_DEBUG(("A2DP UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}
