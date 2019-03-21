/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_states.h
@brief    The Headset States.
*/

#ifndef _HEADSET_STATES_H
#define _HEADSET_STATES_H


/* HFP states */
typedef enum
{
    headsetPoweringOn,    
    headsetConnDiscoverable,
    headsetHfpConnectable,
    headsetHfpConnected,
    headsetOutgoingCallEstablish,
    headsetIncomingCallEstablish,
    headsetActiveCall,
    headsetTestMode 
} headsetHfpState;

#define HEADSET_NUM_HFP_STATES (headsetTestMode + 1) 


/* A2DP states */
typedef enum
{
    headsetA2dpConnectable,
    headsetA2dpConnected,
    headsetA2dpStreaming,
    headsetA2dpPaused
} headsetA2dpState;

#define HEADSET_NUM_A2DP_STATES (headsetA2dpPaused + 1) 

/* AVRCP states */
typedef enum
{
    avrcpInitialising,
    avrcpReady,
    avrcpConnecting,
    avrcpConnected
} headsetAvrcpState;


#endif

