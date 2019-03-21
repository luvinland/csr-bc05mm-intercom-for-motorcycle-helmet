/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_hfp_slc.h
@brief    Handles HFP SLC
*/
#ifndef HEADSET_HFP_SLC_H
#define HEADSET_HFP_SLC_H


#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME    
    hfpSlcReset
    
DESCRIPTION
    Reset the SLC connecting states

*/
void hfpSlcReset( hsTaskData * pApp );


/****************************************************************************
NAME    
    hfpSlcIsConnecting
    
DESCRIPTION
    Method to tell if the headset is currently connecting or not.

RETURNS
    bool
*/
bool hfpSlcIsConnecting ( hsTaskData * pApp );


/****************************************************************************
NAME    
    hfpSlcConnectSuccess
    
DESCRIPTION
    Indicate that the HFP/HSP profile has been connected. 
    
*/
void hfpSlcConnectSuccess (hsTaskData * pApp , HFP * pProfile, Sink sink);


/****************************************************************************
NAME    
    hfpSlcConnectFail
    
DESCRIPTION
    SLC failed to connect.

*/
void hfpSlcConnectFail( hsTaskData *pApp );
        

/****************************************************************************
NAME    
    hfpSlcConnectRequest
    
DESCRIPTION
    Request to create a connection to a remote AG.

*/
void hfpSlcConnectRequest( hsTaskData *pApp, hfp_profile pProfile );


/****************************************************************************
NAME    
    hfpSlcAttemptConnect
    
DESCRIPTION
    Attempt connection to a remote AG.

*/
void hfpSlcAttemptConnect( hsTaskData *pApp, hfp_profile pProfile , bdaddr * pAddr );


/****************************************************************************
NAME    
    hfpSlcDisconnect
    
DESCRIPTION
    Disconnect the SLC associated with this profile instance.

*/
void hfpSlcDisconnect( hsTaskData *pApp );


/****************************************************************************
NAME    
    hfpSlcStoreBdaddr
    
DESCRIPTION
    Store the bluetooth address of an AG device that we know about.

*/
void hfpSlcStoreBdaddr ( const bdaddr * pAddr );


/****************************************************************************
NAME    
    hfpSlcGetLastUsedAG
    
DESCRIPTION
    Retrieve the bdaddr of the last used AG (paired or connected). 
	
RETURNS
	If the return value is TRUE then the function will have returned a valid bdaddr. 
	If the return value is FALSE then there is no last used AG.
*/
bool hfpSlcGetLastUsedAG(bdaddr *addr);
		

/****************************************************************************
NAME    
    hfpSlcGetLastConnectedAG
    
DESCRIPTION
    Retrieve the bdaddr of the last connected AG. 
	
RETURNS
	If the return value is TRUE then the function will have returned a valid bdaddr. 
	If the return value is FALSE then there is no last connected AG.
*/
bool hfpSlcGetLastConnectedAG(bdaddr *addr);


#endif
