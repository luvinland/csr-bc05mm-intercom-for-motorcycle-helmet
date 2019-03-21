/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_avrcp_msg_handler.c
@brief    Implementation of AVRCP library message handlers.
*/


#include "headset_avrcp_event_handler.h"
#include "headset_avrcp_msg_handler.h"
#include "headset_debug.h"
#include "headset_init.h"
#include "headset_link_policy.h"
#include "headset_private.h"
#include "headset_statemanager.h"
#include "headset_volume.h"


#include <avrcp.h>
#include <panic.h>

#ifdef DEBUG_AVRCP_MSG
#define AVRCP_MSG_DEBUG(x) DEBUG(x)
#else
#define AVRCP_MSG_DEBUG(x) 
#endif


/****************************************************************************
  LOCAL MESSAGE HANDLING FUNCTIONS
*/

static void handleAVRCPInitCfm(hsTaskData *app, AVRCP_INIT_CFM_T *msg)
{
	AVRCP_MSG_DEBUG(("AVRCP_INIT_CFM : "));
	if (msg->status == avrcp_success)
	{
		AVRCP_MSG_DEBUG(("Success\n"));
	    app->avrcp = msg->avrcp;
	    app->avrcp_data.pending = FALSE;
	    stateManagerSetAvrcpState(app, avrcpReady);
    }
    else
    {
	    AVRCP_MSG_DEBUG(("Failure [status 0x%x]\n",msg->status));
    }
}

/****************************************************************************/
static void handleAVRCPConnectInd(hsTaskData *app, AVRCP_CONNECT_IND_T *msg)
{
	bool accept = FALSE;
	AVRCP_MSG_DEBUG(("AVRCP_CONNECT_IND : "));
	
    MessageCancelAll(&app->task, APP_AVRCP_CONNECT_REQ);
    
	if ((stateManagerGetAvrcpState() == avrcpReady) && (stateManagerGetHfpState() != headsetPoweringOn))
	{ /* Accept connection */
		AVRCP_MSG_DEBUG(("Accepting connection\n"));
		accept = TRUE;
		stateManagerSetAvrcpState(app, avrcpConnecting);
	}
	else
	{ /* Reject connection due to incorrect state */
		AVRCP_MSG_DEBUG(("Rejecting connection [state = %d]\n", stateManagerGetAvrcpState()));
	}
	AvrcpConnectResponse(msg->avrcp, msg->connection_id, accept);
}

/****************************************************************************/
static void handleAVRCPConnectCfm(hsTaskData *app, AVRCP_CONNECT_CFM_T *msg)
{
	AVRCP_MSG_DEBUG(("AVRCP_CONNECT_CFM : \n"));
	if (stateManagerGetAvrcpState() == avrcpConnecting)
	{
	    if(msg->status == avrcp_success)
	    {	
			stateManagerSetAvrcpState(app, avrcpConnected);
			
			/* Ensure the underlying ACL is encrypted */       
		    ConnectionSmEncrypt( &app->task , msg->sink , TRUE );
			
			/* If a request was made to send an avrcp_play when AVRCP was connected
			   then do it now. 
			*/
			app->avrcp_data.send_play = 0;
				
			/* Assume music won't be playing */
			app->PlayingState = 0;
			
			if (!app->a2dp) 
			{
				/* Disconnect AVRCP if no signalling channel. */
				avrcpDisconnectReq(app);
			}
			else
			{
				/* Update Link Policy as AVRCP has connected. */
				linkPolicyAvrcpStateChange(TRUE);	
			}
	    }
	    else
	    {
			MessageCancelAll( &app->task , APP_SEND_PLAY );
			stateManagerSetAvrcpState(app, avrcpReady);
			app->PlayingState = 1;
			if ( stateManagerGetA2dpState () == headsetA2dpPaused )
				stateManagerEnterA2dpStreamingState(app);
				
	    }
	    
		PROFILE_MEMORY(("AVRCPConnect"))
	}
	else
	{
		AVRCP_MSG_DEBUG(("Ignoring as in wrong state [state = %d]\n", stateManagerGetAvrcpState()));
	}
}

/****************************************************************************/
static void handleAVRCPDisconnectInd(hsTaskData *app, AVRCP_DISCONNECT_IND_T *msg)
{
	PROFILE_MEMORY(("AVRCPDisco"))
	AVRCP_MSG_DEBUG(("AVRCP_DISCONNECT_IND : "));

	if ( stateManagerIsAvrcpConnected() )
	{
		AVRCP_MSG_DEBUG(("Disconnect Result = %d\n", msg->status));
		stateManagerSetAvrcpState(app, avrcpReady);
		
	    /* Reset pending state as we won't get a CFM back from any sent AVRCP commands,
	       now that that connection is closed.
	    */
	    MessageCancelAll(&app->task, APP_AVRCP_CONTROLS);
	    app->avrcp_data.pending = FALSE;
	}
	else
	{
		AVRCP_MSG_DEBUG(("Ignoring as in wrong state [state = %d]\n", stateManagerGetAvrcpState()));
	}
}

/****************************************************************************/
static void handleAVRCPPassthroughCfm(hsTaskData *app)
{
    /* 
        Clearing the pending flag should allow another
        pending event to be delivered to controls_handler 
    */
    app->avrcp_data.pending = FALSE;
}

/****************************************************************************/
static void handleAVRCPPassthroughInd(hsTaskData *app, AVRCP_PASSTHROUGH_IND_T *msg)
{
    /* Acknowledge the request */
	
	if (msg->opid == opid_volume_up)
	{
		/* The headset should accept volume up commands as it supports AVRCP TG category 2. */
		AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);
	
		/* Adjust the local volume only if it is a press command and the A2DP is active. */
		if (!msg->state && stateManagerIsA2dpStreaming())
			VolumeUp(app);
	}
	else if (msg->opid == opid_volume_down)
	{
		/* The headset should accept volume down commands as it supports AVRCP TG category 2. */
		AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);	
		
		/* Adjust the local volume only if it is a press command and the A2DP is active. */
		if (!msg->state && stateManagerIsA2dpStreaming())
			VolumeDown(app);
	}
    else
		/* The headset won't accept any other commands. */
    	AvrcpPassthroughResponse(msg->avrcp, avctp_response_not_implemented);
}

/****************************************************************************/
static void handleAVRCPUnitInfoInd(AVRCP_UNITINFO_IND_T *msg)
{
    /*
        We are not a target so reject UnitInfo requests
    */
    AvrcpUnitInfoResponse(msg->avrcp, FALSE, subunit_monitor, 0, (uint32) 0);
}

/****************************************************************************/
static void handleAVRCPSubUnitInfoInd(AVRCP_SUBUNITINFO_IND_T *msg)
{
    /*
        We are not a target so reject SubUnitInfo requests
    */
    AvrcpSubUnitInfoResponse(msg->avrcp, FALSE, 0);
}

/****************************************************************************/
static void handleAVRCPVendorDependentInd(AVRCP_VENDORDEPENDENT_IND_T *msg)
{
    /*
        We are not a target so reject vendor requests
    */
	AvrcpVendorDependentResponse(msg->avrcp, avctp_response_not_implemented);
}


/****************************************************************************
  INTERFACE FUNCTIONS
*/

/****************************************************************************/
void handleAVRCPMessage( Task task, MessageId id, Message message )
{
    hsTaskData *app = (hsTaskData *) getAppTask() ;
    
    /*AVRCP_MSG_DEBUG(("AVRCP msg: 0x%x\n",id));*/
    
    switch (id)
    {
    case AVRCP_INIT_CFM:
    	handleAVRCPInitCfm(app, (AVRCP_INIT_CFM_T*)message);
		InitUserFeatures(app);
        break;
    case AVRCP_CONNECT_IND:
    	handleAVRCPConnectInd(app, (AVRCP_CONNECT_IND_T*)message);
        break;
    case AVRCP_CONNECT_CFM:
    	handleAVRCPConnectCfm(app, (AVRCP_CONNECT_CFM_T*)message);
        break;
    case AVRCP_DISCONNECT_IND:
    	handleAVRCPDisconnectInd(app, (AVRCP_DISCONNECT_IND_T*)message);
        break;

    case AVRCP_PASSTHROUGH_CFM:
    	handleAVRCPPassthroughCfm(app);
        break;

    case AVRCP_PASSTHROUGH_IND:
    	handleAVRCPPassthroughInd(app, (AVRCP_PASSTHROUGH_IND_T*)message);
        break;
    case AVRCP_UNITINFO_IND:
    	handleAVRCPUnitInfoInd((AVRCP_UNITINFO_IND_T*)message);
        break;
    case AVRCP_SUBUNITINFO_IND:
    	handleAVRCPSubUnitInfoInd((AVRCP_SUBUNITINFO_IND_T*)message);
        break;
    case AVRCP_VENDORDEPENDENT_IND:
    	handleAVRCPVendorDependentInd((AVRCP_VENDORDEPENDENT_IND_T*)message);
        break;

    default:
    	AVRCP_MSG_DEBUG(("UNKNOWN MESSAGE\n"));
        break;
    }    
}



