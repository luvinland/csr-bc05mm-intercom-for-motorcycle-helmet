/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_event_handler.h
@brief    Handles user events
*/
#ifndef HEADSET_EVENT_HANDLER_H
#define HEADSET_EVENT_HANDLER_H

#include <message.h>


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    handleUEMessage
    
DESCRIPTION
    Handles messages from the User Events.
    
*/
void handleUEMessage( Task task, MessageId id, Message message );
        
        
#endif
