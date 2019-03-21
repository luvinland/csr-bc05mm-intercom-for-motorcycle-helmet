/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1

FILE NAME
    headset_intercom_inquire.h

DESCRIPTION
    Handles inquiry procedures of Intercom application

*/

/****************************************************************************
    Header files
*/
#include <connection.h>
#include <stdio.h>
#include <bdaddr.h>
#include <aghfp.h>

#include "headset_private.h"
#include "headset_intercom_inquire.h"

#define CLASS_OF_DEVICE (AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS)


/****************************************************************************
NAME    
    intercomInquire

DESCRIPTION
    Start Intercom inquiry process

RETURNS
    void
*/
void intercomInquire(hsTaskData* app)
{
    /* Turn off security */
    ConnectionSmRegisterIncomingService(0x0000, 0x0001, 0x0000);
    /* Write class of device */
    ConnectionWriteClassOfDevice(CLASS_OF_DEVICE);
    /* Inquire to look for devB devices in inquiry scan mode */
    ConnectionInquire(getAppTask(), 0x9E8B33, 0x9, 0x30, CLASS_OF_DEVICE);
}

/****************************************************************************
NAME    
    intercomInquiryComplete

DESCRIPTION
    Intercom Inquiry complete handler

RETURNS
    void
*/
void intercomInquiryComplete(hsTaskData* app)
{
    if(BdaddrIsZero(&app->ag_bd_addr))
    {
        /* No remote device found, so must decide what to do now */
        /* Restart Inquiry */
        intercomInquire(app);
    }
    else
    {
        /* Remote device found, so will now be trying to connect */
    }
}

/****************************************************************************
NAME    
    intercomInquiryResult

DESCRIPTION
    Intercom Inquiry result handler

RETURNS
    void
*/
void intercomInquiryResult(hsTaskData* app, const CL_DM_INQUIRE_RESULT_T* res)
{
    /*  make sure device class returned is correct,
    and that we currently aren't trying to connect to another device  */
    if((res->dev_class & CLASS_OF_DEVICE) && (BdaddrIsZero(&app->ag_bd_addr)))
    {
        app->ag_bd_addr = res->bd_addr;

        /* Cancel the inquiry */
        ConnectionInquireCancel(getAppTask());

        /* Now try and connect to this device */
        AghfpSlcConnect(app->aghfp, &app->ag_bd_addr);
    }
}
