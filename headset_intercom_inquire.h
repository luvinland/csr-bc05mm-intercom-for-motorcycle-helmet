/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1

FILE NAME
    headset_intercom_inquire.h
    
DESCRIPTION
    Handles inquiry procedures of Intercom application
    
*/

#ifndef _HEADSET_INTERCOM_INQUIRE_H_
#define _HEADSET_INTERCOM_INQUIRE_H_


/****************************************************************************
NAME    
    intercomInquire
    
DESCRIPTION
    Start Intercom inquiry process

RETURNS
    void
*/
void intercomInquire(hsTaskData* app);


/****************************************************************************
NAME    
    intercomInquiryComplete
    
DESCRIPTION
    Intercom Inquiry complete handler

RETURNS
    void
*/
void intercomInquiryComplete(hsTaskData* app);


/****************************************************************************
NAME    
    intercomInquiryResult
    
DESCRIPTION
    Intercom Inquiry result handler

RETURNS
    void
*/
void intercomInquiryResult(hsTaskData* app, const CL_DM_INQUIRE_RESULT_T* res);

#endif /* _HEADSET_INTERCOM_INQUIRE_H_ */

