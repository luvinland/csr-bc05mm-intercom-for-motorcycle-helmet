/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_codec_msg_handler.c
@brief    Handle codec library messages arriving at the app.
*/


#include "headset_debug.h"
#include "headset_codec_msg_handler.h"
#include "headset_init.h"
#include "headset_private.h"

#include <codec.h>
#include <panic.h>


#ifdef DEBUG_CODEC_MSG
#define CODEC_MSG_DEBUG(x) DEBUG(x)
#else
#define CODEC_MSG_DEBUG(x) 
#endif


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************/
void handleCodecMessage( Task task, MessageId id, Message message )
{
    hsTaskData * lApp = (hsTaskData *) getAppTask() ;
    
    switch (id)
    {
    case CODEC_INIT_CFM:
        CODEC_MSG_DEBUG(("CODEC_INIT_CFM\n"));
        if(((CODEC_INIT_CFM_T *)message)->status == success)
        {          
            lApp->theCodecTask = ((CODEC_INIT_CFM_T*)message)->codecTask ;
            /* Initialise the connection library */
            InitConnection();
        }
        else
            Panic();
        break;
   
    default:   
        CODEC_MSG_DEBUG(("CODEC UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}
