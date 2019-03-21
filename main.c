/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    main.c
@brief    Main entry point for the application.
*/


#include "headset_a2dp_msg_handler.h"
#include "headset_a2dp_stream_control.h"
#include "headset_amp.h"
#include "headset_avrcp_event_handler.h"
#include "headset_avrcp_msg_handler.h"
#include "headset_charger.h"
#include "headset_cl_msg_handler.h"
#include "headset_codec_msg_handler.h"
#include "headset_debug.h"
#include "headset_event_handler.h"
#include "headset_events.h"
#include "headset_hfp_msg_handler.h"
#include "headset_init.h"
#include "headset_LEDmanager.h"
#include "headset_private.h"
#include "headset_volume.h" /* Natural Volume Increase */

/* Insert code for Intercom by Jace */
#include "headset_intercom_msg_handler.h"
#include "headset_statemanager.h"

#include <string.h>
#include <a2dp.h>
#include <avrcp.h>
#include <boot.h>
#include <codec.h>
#include <connection.h>
#include <hfp.h>
#include <panic.h>
#include <pio.h>
#include <stdlib.h>

#ifdef TEST_HARNESS
#include "test_bc5_stereo.h"
#endif

#ifdef DEBUG_MAIN
#define MAIN_DEBUG(x) DEBUG(x)
#else
#define MAIN_DEBUG(x) 
#endif


/* Single instance of the Headset state */
hsTaskData *theHeadset;


/****************************************************************************
  FUNCTIONS
*/


/* Insert code for Intercom by Jace */
static void IntercomMode(hsTaskData *app)
{
    MessageCancelAll(&app->task, APP_INTERCOM_MODE);

    if(app->aghfp_connect && app->audio_connect)
    {
        AghfpAudioDisconnect(app->aghfp);
    }
    else if(app->aghfp_initial == FALSE && app->ag_bd_addr.nap == 0 && app->ag_bd_addr.uap == 0 && app->ag_bd_addr.lap == 0)
    {
        AghfpInit(&app->task, aghfp_headset_profile, 0);
    }
    else
    {
        if(app->intercom_button) MessageSend(&app->task, EventSkipForward, 0);

        if(app->ag_bd_addr.nap == 0x24 && app->ag_bd_addr.uap == 0xbc && (app->ag_bd_addr.lap >= 0x100000 && app->ag_bd_addr.lap < 0x200000))
            MessageSend(&app->task, EventSkipForward, 0); /* F100 È£È¯¼º */

        if(app->aghfp_connect)
        {
#ifndef DUAL_STREAM
            if((stateManagerGetA2dpState() == headsetA2dpStreaming) || (stateManagerGetA2dpState() == headsetA2dpPaused)) /* SUSPEND */
            {
                streamControlCeaseA2dpStreaming(app, TRUE);
            }
#endif
            AghfpAudioConnect(app->aghfp, sync_all_sco, 0);
        }
        else
        {
            if(app->intercom_pairing_mode) app->intercom_init = TRUE; /* v100817 v100617 remodify */
            AghfpSlcConnect(app->aghfp, &app->ag_bd_addr);
        }

        MessageSendLater(&app->task, AGHFP_CONNECT_FAIL_TIMEOUT, 0, D_SEC(10));
    }
}


/* Handle any application messages */
static void handleAppMessage( Task task, MessageId id, Message message )
{
	hsTaskData * lApp = (hsTaskData *) getAppTask() ;
	
	switch (id)
    {        
    case APP_RESUME_A2DP:
        MAIN_DEBUG(("APP_RESUME_A2DP\n"));		
        streamControlBeginA2dpStreaming( lApp );
        break;
	case APP_AVRCP_CONTROLS:
		MAIN_DEBUG(("APP_AVRCP_CONTROLS\n"));
		avrcpEventHandleControls(lApp, (APP_AVRCP_CONTROLS_T*)message);
		break;
	case APP_AVRCP_CONNECT_REQ:
		MAIN_DEBUG(("APP_AVRCP_CONNECT_REQ\n"));
		handleAVRCPConnectReq(lApp, (APP_AVRCP_CONNECT_REQ_T*)message);
		break;
	case APP_AMP_OFF:
		MAIN_DEBUG(("APP_AMP_OFF\n"));
		AmpOff(lApp);
		break;
	case APP_SEND_PLAY:
		MAIN_DEBUG(("APP_SEND_PLAY\n"));
		if (lApp->autoSendAvrcp)
			avrcpSendPlay(lApp);
		break;
	case APP_CHARGER_MONITOR:
		MAIN_DEBUG(("APP_CHARGER_MONITOR\n"));
		chargerHandler(lApp);
		break;
    /* Insert code for Intercom by Jace */
    case APP_INTERCOM_MODE: 
        MAIN_DEBUG(("APP_INTERCOM_MODE\n"));
		IntercomMode(lApp);
		break;
	default:
		MAIN_DEBUG(("APP UNHANDLED MSG: 0x%x\n",id));
		break;
	}
}


/* Pass the message to the test harness if running in test mode. */
#ifdef TEST_HARNESS
	#define TEST_HANDLE_LIB_MESSAGE(t,i,m) test_handle_lib_message(t, i,  m);
#else
	#define TEST_HANDLE_LIB_MESSAGE(t,i,m)
#endif

/*************************************************************************
NAME    
    app_handler
    
DESCRIPTION
    This is the main message handler for the Headset Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
static void app_handler(Task task, MessageId id, Message message)
{
    /* Determine the message type based on base and offset */
    if ( ( id >= EVENTS_EVENT_BASE ) && ( id <= EVENTS_LAST_EVENT ) )
    {
        /* Message is a User Generated Event */
        handleUEMessage(task, id,  message);
    }
    else if ( (id >= CL_MESSAGE_BASE) && (id <= CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);
        TEST_HANDLE_LIB_MESSAGE(task, id,  message);
    }
    else if ( (id >= CODEC_MESSAGE_BASE ) && (id <= CODEC_MESSAGE_TOP) )
    {     
        handleCodecMessage(task, id,  message);
    }
    else if ( (id >= HFP_MESSAGE_BASE ) && (id <= HFP_MESSAGE_TOP) )
    {     
        handleHFPMessage(task, id,  message);
        TEST_HANDLE_LIB_MESSAGE(task, id,  message);
    }      
    else if ( (id >= A2DP_MESSAGE_BASE ) && (id <= A2DP_MESSAGE_TOP) )
    {     
        handleA2DPMessage(task, id,  message);
        TEST_HANDLE_LIB_MESSAGE(task, id,  message);
    }      
    else if ( (id >= AVRCP_MESSAGE_BASE ) && (id <= AVRCP_MESSAGE_TOP) )
    {     
        handleAVRCPMessage(task, id,  message);
        TEST_HANDLE_LIB_MESSAGE(task, id,  message);
    }      
    /* Insert code for Intercom by Jace */
    else if ( (id >= AGHFP_MESSAGE_BASE ) && (id <= AGHFP_MESSAGE_TOP) )
    {
        handleINTERCOMMessage(task, id,  message);
    }
	else if ( (id >= HEADSET_MSG_BASE ) && (id <= HEADSET_MSG_TOP) )
    {     
        handleAppMessage(task, id,  message);
    }  
    else /* This message is not one of the above */
    {
        /* Pass this message to default handler */
        MAIN_DEBUG(("MSGTYPE ? [%x]\n", id)) ;
    }
}


Task getAppTask(void)
{
    return &theHeadset->task;
}


int main(void)
{
    MAIN_DEBUG(("Main entered\n")); 
    
	PROFILE_MEMORY(("InitStart"))
	PROFILE_TIME(("InitStart"))

	PioSetPsuRegulator ( TRUE ) ;
    
    theHeadset = malloc(sizeof(hsTaskData));
    memset(theHeadset, 0, sizeof(hsTaskData));

    /* Set up the Application task handler */
    theHeadset->task.handler = app_handler;

    /* Initialise the data contained in the hsTaskData structure */
    InitHeadsetData(theHeadset);

    /* Initialise the Codec Library */
    InitCodec();

    /* Insert code for Intercom by Jace */
    InitIntercomData(theHeadset);

    /* Start the message scheduler loop */
    MessageLoop();
    
    /* Never get here...*/
    return 0;
}
