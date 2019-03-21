/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_volume.h
@brief  Interface to volume controls.
*/

#ifndef HEADSET_VOLUME_H
#define HEADSET_VOLUME_H


#include "headset_private.h"


#define VOL_DEFAULT_VOLUME_LEVEL (0x03) /* v091111 Release */
#define VOL_MAX_VOLUME_LEVEL (0x06) /* v091111 Release */
 

/****************************************************************************
NAME 
    VolumeInit

DESCRIPTION
    Initialises the volume.

*/
void VolumeInit ( hsTaskData * pApp );


/****************************************************************************
NAME 
    VolumeInitHfp

DESCRIPTION
    Initialises the HFP volume.

*/
void VolumeInitHfp ( hsTaskData * pApp );


/****************************************************************************
NAME 
    VolumeUp

DESCRIPTION
    Increments volume.
    
*/
void VolumeUp ( hsTaskData * pApp ) ;


/****************************************************************************
NAME 
    VolumeDown

DESCRIPTION
    Decrements volume.

*/
void VolumeDown ( hsTaskData * pApp ) ;


/****************************************************************************
NAME 
    VolumeStoreLevels

DESCRIPTION
    Store the volume levels in PS.

*/
void VolumeStoreLevels ( hsTaskData * pApp ) ;


/****************************************************************************
NAME 
    VolumeGetHeadsetVolume

DESCRIPTION
    Retrieve the current headset volume. 

RETURNS
	Returns FALSE if no volume was retrieved.

*/
bool VolumeGetHeadsetVolume(hsTaskData * pApp, uint16 * actVol, bool * avAudio);


/****************************************************************************
NAME 
    VolumeSetHeadsetVolume

DESCRIPTION
    Sets the current headset volume. This volume can also be sent to the connected AG. 

RETURNS
	Returns FALSE if no volume was retrieved.

*/
void VolumeSetHeadsetVolume(hsTaskData * pApp, uint16 actVol, bool avAudio);


/****************************************************************************
NAME 
    VolumeRetrieveGain

DESCRIPTION
    Retrieve the gain to be set based on the index table used by the application. 

RETURNS
	Returns the gain to be set.

*/
uint16 VolumeRetrieveGain( uint16 index , bool avAudio );


#endif

