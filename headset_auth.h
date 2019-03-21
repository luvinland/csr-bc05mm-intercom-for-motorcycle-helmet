/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_auth.h
@brief    Interface to remote device authorisation functionality.
*/

#include "headset_private.h"

#ifndef _HEADSET_AUTH_PRIVATE_H_
#define _HEADSET_AUTH_PRIVATE_H_


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
     headsetHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The AV Headset default pin code is sent back.

*/
void headsetHandlePinCodeInd(hsTaskData * pApp , const CL_SM_PIN_CODE_IND_T* ind);


/****************************************************************************
NAME    
    headsetHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

*/
void headsetHandleAuthoriseInd(hsTaskData * pApp , const CL_SM_AUTHORISE_IND_T *ind);


/****************************************************************************
NAME    
    headsetHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

*/
void headsetHandleAuthenticateCfm(hsTaskData * pApp , const CL_SM_AUTHENTICATE_CFM_T *cfm);


/*****************************************************************************
NAME    
     headsetHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_REQ_IND

RETURNS
     void
*/
void headsetHandleUserConfirmationInd(hsTaskData* pApp, const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind);


/*****************************************************************************
NAME    
     headsetHandleUserPasskeyInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS
     void
*/
void headsetHandleUserPasskeyInd(hsTaskData* pApp, const CL_SM_USER_PASSKEY_REQ_IND_T* ind);


/*****************************************************************************
NAME    
     headsetHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     void
*/
void headsetHandleUserPasskeyNotificationInd(hsTaskData* pApp, const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind);


/*****************************************************************************
NAME    
     headsetHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND
 
RETURNS
     void 
*/
void headsetHandleRemoteIoCapabilityInd(hsTaskData* pApp, const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind);


/*****************************************************************************
NAME    
     headsetHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     void 
*/
void headsetHandleIoCapabilityInd(hsTaskData* pApp, const CL_SM_IO_CAPABILITY_REQ_IND_T* ind);


/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS
     
*/
void AuthResetConfirmationFlags(hsTaskData* pApp);

#endif /* _HEADSET_AUTH_PRIVATE_H_ */
