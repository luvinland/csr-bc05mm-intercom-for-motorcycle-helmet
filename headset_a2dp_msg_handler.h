/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_a2dp_msg_handler.h
@brief    Handles a2dp library messages.
*/
#ifndef HEADSET_A2DP_MSG_HANDLER_H
#define HEADSET_A2DP_MSG_HANDLER_H

#include <message.h>
#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    Handles messages from the A2DP library.
    
*/
void handleA2DPMessage( Task task, MessageId id, Message message );


#endif
