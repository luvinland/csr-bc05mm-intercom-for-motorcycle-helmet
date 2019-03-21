/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_avrcp_event_handler.h
@brief    Interface to AVRCP event handlers.
*/
#ifndef HEADSET_AVRCP_EVENT_HANDLER_H
#define HEADSET_AVRCP_EVENT_HANDLER_H


#include "headset_private.h"


/*************************************************************************
NAME    
     avrcpEventHandleControls
    
DESCRIPTION
     Handles the Avrcp control messages and sends the AVRCP command to the remote end.
*/
void avrcpEventHandleControls(hsTaskData *app, APP_AVRCP_CONTROLS_T *msg);


/*************************************************************************
NAME    
     avrcpEventPlay
    
DESCRIPTION
     Handles the Avrcp Play event.
*/
void avrcpEventPlay(hsTaskData* app);


/*************************************************************************
NAME    
     avrcpEventStop
    
DESCRIPTION
     Handles the Avrcp Stop event.
*/
void avrcpEventStop(hsTaskData* app);       


/*************************************************************************
NAME
    avrcpSendStop

DESCRIPTUION
    Sends an AVRCP Stop.
*/
void avrcpSendStop(hsTaskData* app);


/*************************************************************************
NAME
    avrcpSendPlay

DESCRIPTUION
    Sends an AVRCP Play.
*/
void avrcpSendPlay(hsTaskData* app);


/*************************************************************************
NAME
    avrcpSendPause

DESCRIPTUION
    Sends an AVRCP Pause.
*/
void avrcpSendPause(hsTaskData* app);


/*************************************************************************
NAME    
     handleAVRCPConnectReq
    
DESCRIPTION
     This function is called to create an AVRCP connection.     
*/
void handleAVRCPConnectReq(hsTaskData *app, APP_AVRCP_CONNECT_REQ_T *msg);


/*************************************************************************
NAME    
     avrcpConnectReq
    
DESCRIPTION
     This function sends an internal message to create an AVRCP connection.
	 If delay_request is set to TRUE then the AVRCP connection is delayed by a fixed time.
*/
void avrcpConnectReq(hsTaskData *app, bdaddr addr, bool delay_request);


/*************************************************************************
NAME    
     avrcpDisconnectReq
    
DESCRIPTION
     This function is called to disconnect an AVRCP connection     
*/
void avrcpDisconnectReq(hsTaskData *app);


#endif
