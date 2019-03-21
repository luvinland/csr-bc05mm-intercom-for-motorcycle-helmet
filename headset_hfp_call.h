/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_hfp_call.h
@brief    Handles HFP call functionality.
*/
#ifndef HEADSET_HFP_CALL_H
#define HEADSET_HFP_CALL_H


#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/


/****************************************************************************
NAME    
    hfpCallInitiateLNR
    
DESCRIPTION
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends button press

*/
void hfpCallInitiateLNR ( hsTaskData * pApp );


/****************************************************************************
NAME    
    hfpCallAnswer
    
DESCRIPTION
    Answer an incoming call from the headset.

*/
void hfpCallAnswer ( hsTaskData *pApp );


/****************************************************************************
NAME    
    hfpCallHangUp
    
DESCRIPTION
    Hang up the call from the headset.

*/
void hfpCallHangUp ( const hsTaskData *pApp );


/****************************************************************************
NAME    
    hfpCallRecallQueuedEvent
    
DESCRIPTION
    Checks to see if an event was Queued and issues it.

*/
void hfpCallRecallQueuedEvent ( hsTaskData * pApp );


/****************************************************************************
NAME    
    hfpCallClearQueuedEvent
    
DESCRIPTION
    Clears the QUEUE - used on failure to connect / power on / off etc.

*/
void hfpCallClearQueuedEvent ( hsTaskData * pApp );


#endif
