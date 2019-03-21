/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_statemanager.h
@brief    main headset state information
*/
#ifndef _HEADSET_STATE_MANAGER_H
#define _HEADSET_STATE_MANAGER_H

#include "headset_private.h"
#include "headset_states.h"


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME	
	stateManagerGetHfpState

DESCRIPTION
	Helper function to get the current hfp headset state.

RETURNS
	The Headset State information.
    
*/
headsetHfpState stateManagerGetHfpState ( void ) ;


/****************************************************************************
NAME	
	stateManagerGetA2dpState

DESCRIPTION
	Helper function to get the current a2dp headset state.

RETURNS
	The Headset State information.
    
*/
headsetA2dpState stateManagerGetA2dpState ( void ) ;


/****************************************************************************
NAME	
	stateManagerGetCombinedState

DESCRIPTION
	Helper function to get the current combined hfp and a2dp headset states.
    Used to find the correct place in the LED state array.

RETURNS
	The resulting headset state value.
    
*/
uint16 stateManagerGetCombinedState ( void ) ;


/****************************************************************************
NAME	
	stateManagerEnterHfpConnectableState

DESCRIPTION
	Single point of entry for the HFP connectable state.

*/
void stateManagerEnterHfpConnectableState ( hsTaskData * pApp, bool req_disc ) ;


/****************************************************************************
NAME	
	stateManagerEnterA2dpConnectableState

DESCRIPTION
	Single point of entry for the A2DP connectable state.

*/
void stateManagerEnterA2dpConnectableState ( hsTaskData * pApp , bool req_disc ) ;


/****************************************************************************
NAME	
	stateManagerEnterA2dpConnectedState

DESCRIPTION
	Single point of entry for the A2DP connected state.

*/
void stateManagerEnterA2dpConnectedState (hsTaskData * pApp) ;


/****************************************************************************
NAME	
	stateManagerEnterA2dpStreamingState

DESCRIPTION
	Single point of entry for the A2DP streaming state.

*/
void stateManagerEnterA2dpStreamingState(hsTaskData * pApp) ;


/****************************************************************************
NAME	
	stateManagerEnterA2dpPausedState

DESCRIPTION
	Single point of entry for the A2DP paused state.

*/
void stateManagerEnterA2dpPausedState(hsTaskData * pApp) ;


/****************************************************************************
NAME	
	stateManagerEnterConnDiscoverableState

DESCRIPTION
	Single point of entry for the connectable / discoverable state 
    uses timeout if configured.

*/
void stateManagerEnterConnDiscoverableState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterHfpConnectedState

DESCRIPTION
	Single point of entry for the HFP connected state - disables discoverable mode.

*/
void stateManagerEnterHfpConnectedState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterIncomingCallEstablishState

DESCRIPTION
	Single point of entry for the incoming call establish state.

*/
void stateManagerEnterIncomingCallEstablishState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterOutgoingCallEstablishState

DESCRIPTION
	Single point of entry for the outgoing call establish state.

*/
void stateManagerEnterOutgoingCallEstablishState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterActiveCallState

DESCRIPTION
	Single point of entry for the active call state.

*/
void stateManagerEnterActiveCallState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterPoweringOffState

DESCRIPTION
	Single point of entry for the powering off state - enables power off.

*/
void stateManagerEnterPoweringOffState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerPowerOff

DESCRIPTION
	Actually power down the device.

*/
void stateManagerPowerOff ( hsTaskData * pApp )  ;


/****************************************************************************
NAME	
	stateManagerPowerOn

DESCRIPTION
	Power on the deviece by latching on the power regs.
    
*/
void stateManagerPowerOn ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerEnterLimboState

DESCRIPTION
    Method to provide a single point of entry to the limbo /poweringOn state.
    
*/
void stateManagerEnterLimboState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerUpdateLimboState

DESCRIPTION
    Method to update the limbo state.
    
*/
void stateManagerUpdateLimboState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerIsHfpConnected

DESCRIPTION
    Helper method to see if we are connected using the HFP profile or not.
    
RETURNS
	bool
    
*/
bool stateManagerIsHfpConnected ( void ) ;


/****************************************************************************
NAME	
	stateManagerIsA2dpConnected

DESCRIPTION
    Helper method to see if we are connected using the A2DP profile or not.
    
RETURNS
	bool
    
*/
bool stateManagerIsA2dpConnected ( void ) ;


/****************************************************************************
NAME	
	stateManagerIsAvrcpConnected

DESCRIPTION
    Helper method to see if we are connected using the AVRCP profile or not.
    
RETURNS
	bool
    
*/
bool stateManagerIsAvrcpConnected ( void ) ;


/****************************************************************************
NAME	
	stateManagerIsA2dpStreaming

DESCRIPTION
    Helper method to see if we are streaming using the A2DP profile or not.
    
RETURNS
	bool
    
*/
bool stateManagerIsA2dpStreaming(void) ;


/****************************************************************************
NAME	
	stateManagerIsA2dpSignallingActive

DESCRIPTION
    Helper method to see if we are streaming using the A2DP profile or not.
    
RETURNS
	bool
    
*/
bool stateManagerIsA2dpSignallingActive(hsTaskData * pApp) ;


/****************************************************************************
NAME	
	stateManagerEnterTestModeState

DESCRIPTION
    Method to provide a single point of entry to the test mode state.
    
*/
void stateManagerEnterTestModeState ( hsTaskData * pApp ) ;


/****************************************************************************
NAME	
	stateManagerSetAvrcpState

DESCRIPTION
    Updates the AVRCP state.
    
*/
void stateManagerSetAvrcpState ( hsTaskData* pApp, headsetAvrcpState state );


/****************************************************************************
NAME	
	stateManagerGetAvrcpState

DESCRIPTION
    Return the current AVRCP state.
	
RETURNS
	headsetAvrcpState
    
*/ 
headsetAvrcpState stateManagerGetAvrcpState ( void );


#endif

