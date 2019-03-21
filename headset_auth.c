/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_auth.c        
@brief    Implementation of the authentication functionality for the stereo headset application
*/

/****************************************************************************
    Header files
*/

#include "headset_a2dp_connection.h"
#include "headset_auth.h"
#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_hfp_slc.h"
#include "headset_statemanager.h"

#include "panic.h"
#include <connection.h>
#include <ps.h>
#include <stdlib.h>

#ifdef DEBUG_AUTH
    #define AUTH_DEBUG(x) DEBUG(x)    
#else
    #define AUTH_DEBUG(x) 
#endif   


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/

static bool AuthCanHeadsetConnect ( hsTaskData * pApp ) ;

static bool AuthCanHeadsetPair ( hsTaskData * pApp ) ;


/****************************************************************************
  FUNCTIONS
*/

/**************************************************************************/
void headsetHandlePinCodeInd(hsTaskData * pApp , const CL_SM_PIN_CODE_IND_T* ind)
{
    uint16 pin_length = 0;
    uint8 pin[16];
    
    if ( AuthCanHeadsetPair (pApp) )
    {
	    
		AUTH_DEBUG(("auth: Can Pin\n")) ;
			
    	/* Do we have a fixed pin in PS, if not reject pairing */
    	if ((pin_length = PsFullRetrieve(PSKEY_FIXED_PIN, pin, 16)) == 0 || pin_length > 16)
    	{
        	/* Set length to 0 indicating we're rejecting the PIN request */
        	AUTH_DEBUG(("auth : failed to get pin\n")) ;
        	pin_length = 0; 
    	}	
	} 
    /* Respond to the PIN code request */
    ConnectionSmPinCodeResponse(&ind->bd_addr, pin_length, pin); 
}


/*****************************************************************************/
void headsetHandleAuthoriseInd(hsTaskData * pApp , const CL_SM_AUTHORISE_IND_T *ind)
{
	
	bool lAuthorised = FALSE ;
	
	if ( AuthCanHeadsetConnect(pApp) )
	{
		lAuthorised = TRUE ;
	}
	
	AUTH_DEBUG(("auth: Authorised [%d]\n" , lAuthorised)) ;
	    
    /*complete the authentication with the authorised or not flag*/		
    ConnectionSmAuthoriseResponse(&ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming, lAuthorised);
}


/*****************************************************************************/
void headsetHandleAuthenticateCfm(hsTaskData * pApp , const CL_SM_AUTHENTICATE_CFM_T *cfm)
{
    /* Leave bondable mode if successful unless we got a debug key */
    if (cfm->status == auth_status_success && cfm->key_type != cl_sm_link_key_debug)
    {
        (void)PsStore(PSKEY_LAST_PAIRED_DEVICE, &cfm->bd_addr, sizeof(bdaddr)); 
        /* Send a user event to the app for indication purposes */
        MessageSend (&pApp->task , EventPairingSuccessful , 0 );
    }

    /* reset pairing info if we timed out on confirmation */
    AuthResetConfirmationFlags(pApp);
}


/*****************************************************************************
NAME    
     headsetHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_REQ_IND

RETURNS
     void
*/
void headsetHandleUserConfirmationInd(hsTaskData* pApp, const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind)
{
    if (AuthCanHeadsetPair(pApp) && pApp->forceMitmEnabled)
    {
        pApp->confirmation = TRUE;
        AUTH_DEBUG(("auth: can confirm %ld\n", ind->numeric_value));
        /* should use text to speech here */
        pApp->confirmation_addr = (bdaddr*)PanicUnlessMalloc(sizeof(bdaddr));
        *pApp->confirmation_addr = ind->bd_addr;
    }
    else
    {
        /* reject the confirmation request */
        AUTH_DEBUG(("auth: rejecting confirmation req\n"));
        ConnectionSmUserConfirmationResponse(&ind->bd_addr, FALSE);
    }
}


/*****************************************************************************
NAME    
     headsetHandleUserPasskeyInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS
     void
*/
void headsetHandleUserPasskeyInd(hsTaskData* pApp, const CL_SM_USER_PASSKEY_REQ_IND_T* ind)
{
    /* reject the passkey request */
    AUTH_DEBUG(("auth: rejecting passkey req\n"));
    ConnectionSmUserPasskeyResponse(&ind->bd_addr, TRUE, 0);
}


/*****************************************************************************
NAME    
     headsetHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     void
*/
void headsetHandleUserPasskeyNotificationInd(hsTaskData* pApp, const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind)
{
    AUTH_DEBUG(("auth: Passkey [%ld]\n", ind->passkey));
    /* should use text to speech here */
}


/*****************************************************************************
NAME    
     headsetHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     void 
*/
void headsetHandleIoCapabilityInd(hsTaskData* pApp, const CL_SM_IO_CAPABILITY_REQ_IND_T* ind)
{
    /* Only send IO capabilities if we are pairable */
    if (AuthCanHeadsetPair(pApp))
    {
        cl_sm_io_capability local_io_caps = pApp->forceMitmEnabled ? cl_sm_io_cap_display_yes_no : cl_sm_io_cap_no_input_no_output;
        AUTH_DEBUG(("auth: sending IO capability\n"));
        ConnectionSmIoCapabilityResponse(&ind->bd_addr, local_io_caps, pApp->forceMitmEnabled, TRUE, FALSE, 0, 0);
    }
    else /* send a reject response */
    {
        AUTH_DEBUG(("auth: rejecting IO capability req\n"));
        ConnectionSmIoCapabilityResponse(&ind->bd_addr, cl_sm_reject_request, FALSE, FALSE, FALSE, 0, 0);
    }
}


/*****************************************************************************
NAME    
     headsetHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND
 
RETURNS
     void 
*/
void headsetHandleRemoteIoCapabilityInd(hsTaskData* pApp, const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind)
{
    AUTH_DEBUG(("auth: Incoming Authentication Request\n"));
}


/****************************************************************************
NAME    
    AuthCanHeadsetPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed.

RETURNS
    bool
*/
static bool AuthCanHeadsetPair ( hsTaskData * pApp )
{
	bool lCanPair = FALSE ;
	
	/*if we are in pairing mode*/
	if (stateManagerGetHfpState() == headsetConnDiscoverable || pApp->intercom_pairing_mode)
	{
	    lCanPair = TRUE ;
		AUTH_DEBUG(("auth: is ConnDisco\n")) ;
	}	    		
    
	/*or if we initiated the connection*/
	if ( hfpSlcIsConnecting( pApp ) || a2dpIsConnecting( pApp) )
	{
		lCanPair = TRUE ;
		AUTH_DEBUG(("auth: is Initiated\n")) ;
	}
 
    return lCanPair ;
}


/****************************************************************************
NAME    
    AuthCanHeadsetConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed.

RETURNS
    bool
*/
static bool AuthCanHeadsetConnect ( hsTaskData * pApp )
{
	bool lCanConnect = FALSE ;
	
	/* If the headset is on then authorise */
	if (stateManagerGetHfpState() != headsetPoweringOn)
	{
		lCanConnect = TRUE ;
		AUTH_DEBUG(("auth: headset is on\n")) ;
	}
		
    return lCanConnect ;
}


/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS
    void 
*/
void AuthResetConfirmationFlags(hsTaskData* pApp)
{
    AUTH_DEBUG(("auth: reset confirmation flags\n"));
    if (pApp->confirmation_addr != NULL)
    {
        AUTH_DEBUG(("auth: free confirmation addr\n"));
        free(pApp->confirmation_addr);
    }
    pApp->confirmation_addr = NULL;
    pApp->confirmation = FALSE;
}

