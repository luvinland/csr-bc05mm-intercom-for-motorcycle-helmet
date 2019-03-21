/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file     headset_intercom_msg_handler.h
@brief    Handles intercom library messages.
*/
#ifndef HEADSET_INTERCOM_MSG_HANDLER_H
#define HEADSET_INTERCOM_MSG_HANDLER_H

#include <message.h>


/****************************************************************************
    FUNCTIONS
*/

/*************************************************************************
NAME
    handleINTERCOMMessage

DESCRIPTION
    Handles messages from the INTERCOM library.

*/
void handleINTERCOMMessage(Task task, MessageId id, Message message);

#endif
