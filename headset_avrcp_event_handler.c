/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_avrcp_event_handler.c
@brief    Implementation of AVRCP event handlers.
*/

#include "headset_a2dp_connection.h"
#include "headset_a2dp_stream_control.h"
#include "headset_avrcp_event_handler.h"
#include "headset_debug.h"
#include "headset_statemanager.h"

#include <panic.h>


#ifdef DEBUG_AVRCP_EVENT
#define AVRCP_EVENT_DEBUG(x) DEBUG(x)
#else
#define AVRCP_EVENT_DEBUG(x) 
#endif


/****************************************************************************
  LOCAL FUNCTIONS
*/


/**************************************************************************/
static void sendAVRCP(hsTaskData* app, avc_operation_id op_id, uint8 state)
{
    app->avrcp_data.pending = TRUE;
    
    /* Send a key press */
    AvrcpPassthrough(app->avrcp, subunit_panel, 0, state, op_id, 0, 0);   
}


/**************************************************************************/
static void avrcpSendControlMessage(hsTaskData *app, avrcp_controls control)
{
	if ( stateManagerIsAvrcpConnected() )
    {
		APP_AVRCP_CONTROLS_T *message = PanicUnlessNew(APP_AVRCP_CONTROLS_T);
		message->control = control;
		MessageSendConditionally(&app->task, APP_AVRCP_CONTROLS, message, &app->avrcp_data.pending);
	}
}


/**************************************************************************/
static void avrcpStopPress(hsTaskData* app)
{
    /* see controls_handler description */
    avrcpSendControlMessage(app, AVRCP_CTRL_STOP_PRESS);
}


/**************************************************************************/
static void avrcpStopRelease(hsTaskData* app)
{
    /* see button_handler message queue description */
    avrcpSendControlMessage(app, AVRCP_CTRL_STOP_RELEASE);
}


/**************************************************************************/
static void avrcpPausePress(hsTaskData* app)
{
    /* see controls_handler description */
    avrcpSendControlMessage(app, AVRCP_CTRL_PAUSE_PRESS);
}


/**************************************************************************/
static void avrcpPauseRelease(hsTaskData* app)
{
    /* see button_handler message queue description */
    avrcpSendControlMessage(app, AVRCP_CTRL_PAUSE_RELEASE);
}


/**************************************************************************/
static void avrcpPlayPress(hsTaskData* app)
{
    /* see controls_handler description */
    avrcpSendControlMessage(app, AVRCP_CTRL_PLAY_PRESS);
}


/**************************************************************************/
static void avrcpPlayRelease(hsTaskData* app)
{
    /* see button_handler message queue description */
    avrcpSendControlMessage(app, AVRCP_CTRL_PLAY_RELEASE);
}


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************/
void avrcpEventHandleControls(hsTaskData *app, APP_AVRCP_CONTROLS_T *msg)
{
	AVRCP_EVENT_DEBUG(("APP_AVRCP_CONTROLS : "));
    switch (msg->control)
    {
        case AVRCP_CTRL_PAUSE_PRESS:
	        AVRCP_EVENT_DEBUG(("Sending Pause Pressed\n"));
            sendAVRCP(app, opid_pause, 0);
            break;
        case AVRCP_CTRL_PAUSE_RELEASE:
            AVRCP_EVENT_DEBUG(("Sending Pause Released\n"));
            sendAVRCP(app, opid_pause, 1);
            break;
        case AVRCP_CTRL_PLAY_PRESS:
            AVRCP_EVENT_DEBUG(("Sending Play Pressed\n"));
            sendAVRCP(app, opid_play, 0);
            break;
        case AVRCP_CTRL_PLAY_RELEASE:
            AVRCP_EVENT_DEBUG(("Sending Play Released\n"));
            sendAVRCP(app, opid_play, 1);
            break;
        case AVRCP_CTRL_STOP_PRESS:
            AVRCP_EVENT_DEBUG(("Sending Stop Pressed\n"));
            sendAVRCP(app, opid_stop, 0);
            break;
        case AVRCP_CTRL_STOP_RELEASE:
            AVRCP_EVENT_DEBUG(("Sending Stop Released\n"));
            sendAVRCP(app, opid_stop, 1);
            break;
		default:
			break;
    }
}


/**************************************************************************/
void avrcpEventPlay(hsTaskData* app)
{
	AVRCP_EVENT_DEBUG(("avrcpEventPlay\n"));
	
	if ( stateManagerIsAvrcpConnected () )
	{
		/* There is an AVRCP connection */
		
		/* If no media channel connected, then connect it now */
		if (!A2dpGetMediaSink(app->a2dp))
		{
			a2dpConnectRequest(app, TRUE);
		}
		else
		{		
			/* Don't try to auto send AVRCP as user has intervened */
			app->sendPlayOnConnection = FALSE;
			
			/* AVRCP is connected so send AVRCP Play. */
			avrcpSendPlay(app);
		
			/* Change to streaming state if currently paused. */
			if ( stateManagerGetA2dpState () == headsetA2dpPaused )
				stateManagerEnterA2dpStreamingState(app);
			else if (!stateManagerIsA2dpStreaming())
			{
				/* There is a media connection but we aren't streaming, so send A2dpStart */
				streamControlStartA2dp(app);
			}
		}
	}	
	else
	{
		/* Toggle streaming state */
		if ( stateManagerGetA2dpState () == headsetA2dpConnected )
		{
			if ( A2dpGetMediaSink(app->a2dp) )
			{
				/* There is a media connection but we aren't streaming, so send A2dpStart */
				streamControlBeginA2dpStreaming(app);
			}
			else
			{
				/* No media connection so connect it now. */
				a2dpConnectRequest(app, TRUE);	
			}
		}
		else if ( (stateManagerGetA2dpState () == headsetA2dpStreaming ) || (stateManagerGetA2dpState () == headsetA2dpPaused) )
		{
			streamControlCeaseA2dpStreaming(app, TRUE);
		}
	}
}


/**************************************************************************/
void avrcpEventStop(hsTaskData* app)
{
	AVRCP_EVENT_DEBUG(("avrcpEventStop\n"));
	if ( stateManagerIsAvrcpConnected () )
	{					
		/* Don't try to auto send AVRCP as user has intervened */
		app->sendPlayOnConnection = FALSE;
		
		/* AVRCP is connected so send AVRCP Stop. */
		avrcpSendStop(app);
		
		/* Change to paused state if currently streaming . */
		if ( stateManagerGetA2dpState () == headsetA2dpStreaming )
			stateManagerEnterA2dpPausedState(app);
	}	
	else
	{
		/* No AVRCP connection, but try and stop streaming. */
		if ( (stateManagerGetA2dpState () == headsetA2dpStreaming ) || (stateManagerGetA2dpState () == headsetA2dpPaused) )
		{
			streamControlCeaseA2dpStreaming(app, TRUE);
		}
	}
}


/**************************************************************************/
void avrcpSendPause(hsTaskData* app)
{
    if ( stateManagerIsAvrcpConnected() )
    {
		/* If the A2DP state is streaming or paused then this tracks what it thinks is
		   the playing status of the media. If the A2DP state is connected, then it needs to 
		   track the AVRCP commands independently of the A2DP state.
		*/
		if ( !stateManagerIsA2dpStreaming() )
			app->PlayingState = 1 - app->PlayingState;
		else
			app->PlayingState = 0;
		
        avrcpPausePress(app);
        avrcpPauseRelease(app);
    }
}


/**************************************************************************/
void avrcpSendPlay(hsTaskData* app)
{
    if ( stateManagerIsAvrcpConnected() )
    {
		/* If the A2DP state is streaming or paused then this tracks what it thinks is
		   the playing status of the media. If the A2DP state is connected, then it needs to 
		   track the AVRCP commands independently of the A2DP state.
		*/
		if ( !stateManagerIsA2dpStreaming() )
			app->PlayingState = 1 - app->PlayingState;
		else
			app->PlayingState = 1;
					
        avrcpPlayPress(app);
        avrcpPlayRelease(app);
    }
}


/**************************************************************************/
void avrcpSendStop(hsTaskData* app)
{
    if ( stateManagerIsAvrcpConnected() )
    {
		/* The media should now be stopped */
		app->PlayingState = 0;
		
        avrcpStopPress(app);
        avrcpStopRelease(app);
    }
}


/*************************************************************************/
void handleAVRCPConnectReq(hsTaskData *app, APP_AVRCP_CONNECT_REQ_T *msg)
{
    if ((stateManagerGetAvrcpState() == avrcpReady) && (stateManagerGetHfpState() != headsetPoweringOn))
    {
        MessageCancelAll(&app->task, APP_AVRCP_CONNECT_REQ);
        /* Change to connecting state */
	    stateManagerSetAvrcpState(app, avrcpConnecting);
        /* Establish AVRCP connection */
	    AvrcpConnect(app->avrcp, &msg->addr);
    }
}


/**************************************************************************/
void avrcpConnectReq(hsTaskData *app, bdaddr addr, bool delay_request)
{
	APP_AVRCP_CONNECT_REQ_T *message = (APP_AVRCP_CONNECT_REQ_T*)PanicUnlessMalloc(sizeof(APP_AVRCP_CONNECT_REQ_T));
	message->addr = addr;
	if (delay_request)
		MessageSendLater(&app->task, APP_AVRCP_CONNECT_REQ, message, 3000);
	else
    	MessageSend(&app->task, APP_AVRCP_CONNECT_REQ, message);
}


/**************************************************************************/
void avrcpDisconnectReq(hsTaskData *app)
{
    if ( stateManagerIsAvrcpConnected() )
    {
        /* Disconnect AVRCP connection */
	    AvrcpDisconnect(app->avrcp);
    }
}

