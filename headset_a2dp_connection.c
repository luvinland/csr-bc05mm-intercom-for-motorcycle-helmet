/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_a2dp_connection.c
@brief    Handles a2dp connection.
*/


#include "headset_a2dp_connection.h"
#include "headset_a2dp_stream_control.h"
#include "headset_avrcp_event_handler.h"
#include "headset_configmanager.h"
#include "headset_hfp_slc.h"
#include "headset_init.h"
#include "headset_statemanager.h"

#include <bdaddr.h>
#include <ps.h>


#ifdef DEBUG_A2DP_CONNECTION
#define A2DP_CONNECTION_DEBUG(x) DEBUG(x)
#else
#define A2DP_CONNECTION_DEBUG(x) 
#endif


/****************************************************************************
  FUNCTIONS
*/
bool a2dpGetLastUsedSource(hsTaskData *app, bdaddr *addr, uint8 *seid)
{
	bool ret = TRUE;
	
    if (!PsRetrieve(PSKEY_LAST_USED_AV_SOURCE, addr, sizeof(bdaddr)))
    {
		if (!PsRetrieve(PSKEY_LAST_PAIRED_DEVICE, addr, sizeof(bdaddr)))
		{
		    ret = FALSE;
	    }
	}
	
	if ((ret) && (!PsRetrieve(PSKEY_LAST_USED_AV_SOURCE_SEID, seid, sizeof(uint8))))
	{
#if 0 /* TTA & A2DP conn */
        /* No remote device has connected to a SEP on this device, so no record of which SEP to use.  */
        if (app->a2dpCodecsEnabled & (1<<MP3_CODEC_BIT))
            /* If MP3 is enabled always try this first, it may be supported at the remote end */
            *seid = MP3_SEID;
        else
#endif
			/* Default to using SBC */
			*seid = SBC_SEID;
	}
	
	return ret;
}


/****************************************************************************/
void a2dpReconnectProcedure(hsTaskData *app)
{
	bdaddr a2dp_addr, ag_addr;	
	uint8 seid;
	
	/* 	
		If this A2DP source is also an AG then only connect signalling channel. 
		If the A2DP source and AG are different devices then connect signalling and media channels.	   
	*/
	if (!a2dpGetLastUsedSource(app, &a2dp_addr, &seid))
		return;
	
	if (!hfpSlcGetLastConnectedAG(&ag_addr))
	{
		a2dpConnectRequest(app, TRUE);
		return;	
	}
	
	if (BdaddrIsSame(&a2dp_addr, &ag_addr) && app->slcConnectFromPowerOn)
	{
		if (!A2dpGetSignallingSink(app->a2dp))
			a2dpConnectRequest(app, TRUE); /* TTA & A2DP conn */
	}
	else
	{
		a2dpConnectRequest(app, TRUE);
	}
}


/****************************************************************************/
void a2dpConnectRequest(hsTaskData *app, bool connect_media)
{
    bdaddr addr;
	uint8 seid = SBC_SEID;
	uint8 seids[2];
	uint16 size_seids = 1;
	
	if (stateManagerGetHfpState() == headsetPoweringOn)
	{
		A2DP_CONNECTION_DEBUG(("A2DP_CONNECTION: Already powered off\n"));
		return;
	}
    
	if (!a2dpGetLastUsedSource(app, &addr, &seid))
	{
		MessageSend(&app->task, EventA2dpReconnectFailed, 0);
		return;
	}
				
    app->a2dpConnecting = TRUE;
	
	/* Send an avrcp_play once media has connected and entered
	   the correct state. This is so A2DP sources (standalone and AGs)
	   will start playing music straight away.
	*/
	if (connect_media)
	{
		app->sendPlayOnConnection = TRUE;
		
		size_seids = InitSeidConnectPriority(app, seid, seids);
			
		/* Open media channel. Call the correct API depending on whether signalling connected or not. */
        if (A2dpGetSignallingSink(app->a2dp))
    		A2dpOpen(app->a2dp, size_seids, seids);
        else
            A2dpConnectOpen(&app->task, &addr, size_seids, seids, app->a2dp_data.sep_entries);
	}
	else
	{
		/* Connect signalling channel only */
		A2dpConnectSignallingChannel(&app->task, &addr, app->a2dp_data.sep_entries);
	}

}


/****************************************************************************/
void a2dpDisconnectRequest(hsTaskData *app)
{
	/* Don't resume A2DP streaming */
	streamControlCancelResumeA2dpStreaming( app );
	
	/* Disconnect AVRCP first. */
	avrcpDisconnectReq(app);
	
	/* Close all A2DP signalling and media channels */
	A2dpDisconnectAll(app->a2dp);
}


/****************************************************************************/
bool a2dpIsConnecting ( hsTaskData * pApp )
{
	return pApp->a2dpConnecting;	
}

