/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_hfp_call.c
@brief    Implementation of HFP call functionality.
*/


#include "headset_a2dp_stream_control.h"
#include "headset_debug.h"
#include "headset_hfp_call.h"
#include "headset_statemanager.h"
#include "headset_tones.h"

#include <hfp.h>
#include <panic.h>
#include <ps.h>


#ifdef DEBUG_HFP_CALL
#define HFP_CALL_DEBUG(x) DEBUG(x)
#else
#define HFP_CALL_DEBUG(x) 
#endif


static void headsetQueueEvent ( hsTaskData * pApp , headsetEvents_t pEvent) ;

static headsetEvents_t gMessageQueued = EventInvalid ;


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************/


/****************************************************************************/
void hfpCallInitiateLNR ( hsTaskData * pApp )
{
    HFP_CALL_DEBUG(("CM: LNR\n")) ;

    if (!stateManagerIsHfpConnected() || !HfpGetSlcSink(pApp->hfp_hsp)) /* v100129 */
    {
        MessageSend ( &pApp->task , EventEstablishSLC , 0 ) ;
        headsetQueueEvent(pApp , EventLastNumberRedial) ;
    }
    else
    {
        
        HFP_CALL_DEBUG(("CM: LNR Connected\n")) ;

#ifdef BNFON
        HfpSendHsButtonPress(pApp->hsp);
#else
        if ( pApp->profile_connected == hfp_handsfree_profile )
        {
            HfpLastNumberRedial( pApp->hfp );    
        }
#endif
    }    
}


/****************************************************************************/
void hfpCallAnswer ( hsTaskData *pApp )
{
    /* 
        Call the HFP lib function, this will determine the AT cmd to send
        depending on whether the profile instance is HSP or HFP compliant.
    */ 
#ifdef BNFON
    HfpSendHsButtonPress(pApp->hsp);
#else
    if ( pApp->profile_connected == hfp_handsfree_profile )
    {
        HfpAnswerCall ( pApp->hfp );
        
        /* Terminate the ring tone */
        ToneTerminate ( pApp ) ;
    }
#endif
}


/****************************************************************************/
void hfpCallHangUp ( const hsTaskData *pApp )
{
    /* Terminate the current ongoing call process */
#ifdef BNFON
    HfpSendHsButtonPress(pApp->hsp);
#else
    if ( pApp->profile_connected == hfp_handsfree_profile )
    {
        HfpTerminateCall ( pApp->hfp );
    }
#endif
}


/*****************************************************************************/
void hfpCallRecallQueuedEvent ( hsTaskData * pApp )
{
        /*this is currently only applicable to LNR and voice Dial but does not care */
    if (gMessageQueued != EventInvalid)
    {
        switch (stateManagerGetHfpState() )
        {
            case headsetIncomingCallEstablish:
            case headsetOutgoingCallEstablish:
            case headsetActiveCall:            
                /* Do Nothing Message Gets ignored*/
            break ;
            default:
                if ( pApp->profile_connected == hfp_handsfree_profile )
                {
                    MessageSend ( &pApp->task , gMessageQueued , 0 ) ; 
                
                    HFP_CALL_DEBUG(("CM: Queued Message ? [%x] Sent\n" , gMessageQueued))    
                }
            break;
        }
    }    
    
    /*reset the queued event*/
    gMessageQueued = EventInvalid ;
}


/*****************************************************************************/
void hfpCallClearQueuedEvent ( hsTaskData * pApp )
{
    /* this resets the queue - on a conenction fail / power off etc */
    gMessageQueued = EventInvalid ;
}


/****************************************************************************
NAME    
    headsetQueueEvent
    
DESCRIPTION
    Queues an event to be sent once the headset is connected.

*/
static void headsetQueueEvent ( hsTaskData * pApp , headsetEvents_t pEvent)
{
    HFP_CALL_DEBUG(("CM: QQ Ev[%x]\n", pEvent)) ;
    gMessageQueued = pEvent ;
}
