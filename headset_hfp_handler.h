/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file    headset_hfp_handler.h
@brief    Functions which handle the HFP library messages.
*/
#ifndef HEADSET_HFP_HANDLER_H
#define HEADSET_HFP_HANDLER_H


#include "headset_private.h"


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    hfpHandlerInitCfm
    
DESCRIPTION
    Handles the HFP_INIT_CFM message from the HFP library.
    
*/
void hfpHandlerInitCfm( hsTaskData * pApp , const HFP_INIT_CFM_T *cfm );
        

/*************************************************************************
NAME    
    hfpHandlerConnectInd
    
DESCRIPTION
    Handles the HFP_SLC_CONNECT_IND message from the HFP library.
    
*/
void hfpHandlerConnectInd( hsTaskData * pApp , const HFP_SLC_CONNECT_IND_T *cfm );


/*************************************************************************
NAME    
    hfpHandlerConnectCfm
    
DESCRIPTION
    Handles the HFP_SLC_CONNECT_CFM message from the HFP library.
    
*/
void hfpHandlerConnectCfm( hsTaskData * pApp , const HFP_SLC_CONNECT_CFM_T *cfm );
 

/*************************************************************************
NAME    
    hfpHandlerDisconnectInd
    
DESCRIPTION
    Handles the HFP_SLC_DISCONNECT_IND message from the HFP library.
    
*/
void hfpHandlerDisconnectInd(hsTaskData * pApp, const HFP_SLC_DISCONNECT_IND_T *ind);


/*************************************************************************
NAME    
    hfpHandlerInbandRingInd
    
DESCRIPTION
    Handles the HFP_IN_BAND_RING_IND message from the HFP library.
    
*/
void hfpHandlerInbandRingInd( hsTaskData * pApp, const HFP_IN_BAND_RING_IND_T * ind );


/*************************************************************************
NAME    
    hfpHandlerCallInd
    
DESCRIPTION
    Handles the HFP_CALL_IND message from the HFP library.
    
*/
void hfpHandlerCallInd ( hsTaskData *pApp,  const HFP_CALL_IND_T * pInd );


/*************************************************************************
NAME    
    hfpHandlerCallSetupInd
    
DESCRIPTION
    Handles the HFP_CALL_SETUP_IND message from the HFP library.
    
*/
void hfpHandlerCallSetupInd ( hsTaskData *pApp,  const HFP_CALL_SETUP_IND_T * pInd );


/*************************************************************************
NAME    
    hfpHandlerRingInd
    
DESCRIPTION
    Handles the HFP_RING_IND message from the HFP library.
    
*/
void hfpHandlerRingInd ( hsTaskData *pApp );
        

/*************************************************************************
NAME    
    hfpHandlerLastNoRedialCfm
    
DESCRIPTION
    Handles the HFP_LAST_NUMBER_REDIAL_CFM message from the HFP library.
    
*/
void hfpHandlerLastNoRedialCfm( hsTaskData *pApp, const HFP_LAST_NUMBER_REDIAL_CFM_T *cfm );


/*************************************************************************
NAME    
    hfpHandlerEncryptionChangeInd
    
DESCRIPTION
    Handles the HFP_ENCRYPTION_CHANGE_IND message from the HFP library.
    
*/
void hfpHandlerEncryptionChangeInd( hsTaskData *pApp, const HFP_ENCRYPTION_CHANGE_IND_T *ind );


/*************************************************************************
NAME    
    hfpHandlerSpeakerVolumeInd
    
DESCRIPTION
    Handles the HFP_SPEAKER_VOLUME_IND message from the HFP library.
    
*/
void hfpHandlerSpeakerVolumeInd( hsTaskData *pApp, const HFP_SPEAKER_VOLUME_IND_T *ind );


/*************************************************************************
NAME    
    hfpHandlerAudioConnectInd
    
DESCRIPTION
    Handles the HFP_AUDIO_CONNECT_IND message from the HFP library.
    
*/
void hfpHandlerAudioConnectInd( hsTaskData *pApp, const HFP_AUDIO_CONNECT_IND_T *ind );


/*************************************************************************
NAME    
    hfpHandlerAudioConnectCfm
    
DESCRIPTION
    Handles the HFP_AUDIO_CONNECT_CFM message from the HFP library.
    
*/
void hfpHandlerAudioConnectCfm( hsTaskData *pApp, const HFP_AUDIO_CONNECT_CFM_T *cfm );


/*************************************************************************
NAME    
    hfpHandlerAudioDisconnectInd
    
DESCRIPTION
    Handles the HFP_AUDIO_DISCONNECT_IND message from the HFP library.
    
*/
void hfpHandlerAudioDisconnectInd( hsTaskData *pApp, const HFP_AUDIO_DISCONNECT_IND_T *ind );


#endif
