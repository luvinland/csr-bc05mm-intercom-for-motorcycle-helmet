/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_link_policy.h
@brief  Interface to the link policy controls.
*/

#ifndef HEADSET_LINK_POLICY_H_
#define HEADSET_LINK_POLICY_H_


#include "headset_states.h"


/*****************************************************************************/
void linkPolicyHfpStateChange(bool hfpConnected, bool scoStateChange);


/*****************************************************************************/
void linkPolicyA2dpStateChange(bool a2dpConnected);


/*****************************************************************************/
void linkPolicyAvrcpStateChange(bool avrcpConnected);

#endif /* HEADSET_LINK_POLICY_H_ */
