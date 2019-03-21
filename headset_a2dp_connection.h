/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_a2dp_connection.h
@brief    Handles a2dp connection.
*/
#ifndef HEADSET_A2DP_CONNECTION_H
#define HEADSET_A2DP_CONNECTION_H

#include <message.h>
#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/


/*************************************************************************
NAME    
    a2dpGetLastUsedSource
    
DESCRIPTION
    Retrieve bdaddr and sep of last used A2DP source. 
	Return value of FALSE indicates no such device.
*/
bool a2dpGetLastUsedSource(hsTaskData *app, bdaddr *addr, uint8 *seid);


/*************************************************************************
NAME    
     a2dpReconnectProcedure
    
DESCRIPTION
     Connect to the last A2DP source. Need to see if media has to be connected
	 as well as the signalling.
*/
void a2dpReconnectProcedure(hsTaskData *app);


/*************************************************************************
NAME    
     a2dpConnectRequest
    
DESCRIPTION
     Connect to the last AV source the headset was connected to.
*/
void a2dpConnectRequest(hsTaskData *app, bool connect_media);


/*************************************************************************
NAME    
     a2dpDisconnectRequest
    
DESCRIPTION
     Disconnect from the AV source.
*/
void a2dpDisconnectRequest(hsTaskData *app);


/*************************************************************************
NAME    
     a2dpIsConnecting
    
DESCRIPTION
     Is the headset currently connecting A2DP.
*/
bool a2dpIsConnecting ( hsTaskData * pApp );
		

#endif
