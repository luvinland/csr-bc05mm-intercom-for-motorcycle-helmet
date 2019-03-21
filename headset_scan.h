/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_scan.h
@brief   Interface to device access functionality. 
*/

#ifndef _HEADSET_SCAN_H_
#define _HEADSET_SCAN_H_


#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    headsetEnableConnectable
    
DESCRIPTION
    Make the device connectable. 

*/
void headsetEnableConnectable(hsTaskData *app);


/****************************************************************************
NAME    
    headsetDisableConnectable
    
DESCRIPTION
    Take device out of connectable mode.

*/
void headsetDisableConnectable(hsTaskData *app);


/****************************************************************************
NAME    
    headsetEnableDiscoverable
    
DESCRIPTION
    Make the device discoverable. 

*/
void headsetEnableDiscoverable(hsTaskData *app);


/****************************************************************************
NAME    
    headsetDisableDiscoverable
    
DESCRIPTION
    Make the device non-discoverable. 

RETURNS
    void
*/
void headsetDisableDiscoverable(hsTaskData *app);


/****************************************************************************
NAME    
    headsetWriteEirData
    
DESCRIPTION
    Writes the local name and device UUIDs into device EIR data, local name 
        is shortened to fit into a DH1 packet if necessary

RETURNS
    void
*/
void headsetWriteEirData(hsTaskData *app, CL_DM_LOCAL_NAME_COMPLETE_T *message);

#endif /* _HEADSET_SCAN_H_ */
