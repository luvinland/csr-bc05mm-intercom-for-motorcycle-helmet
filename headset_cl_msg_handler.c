/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_cl_msg_handler.c
@brief    Handle connection library messages arriving at the app.
*/


#include "headset_auth.h"
#include "headset_debug.h"
#include "headset_cl_msg_handler.h"
#include "headset_init.h"
#include "headset_private.h"
#include "headset_scan.h"
/* For AG inquire */
#include "headset_intercom_inquire.h"

#include <connection.h>
#include <panic.h>
#include <pio.h>

#ifdef DEBUG_CL_MSG
#define CL_MSG_DEBUG(x) DEBUG(x)
#else
#define CL_MSG_DEBUG(x) 
#endif


/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************/

#ifndef S100A /* v101201 Repairing fail for same addr */
/* For AG inquire */
/* Keep a list of addresses we've tried before without success */
enum { FAILED_SIZE = 8 };
static bdaddr failed_addr[FAILED_SIZE];

static uint16 tried_and_failed(const bdaddr *addr)
{
    static uint16 failed_next;
    uint16 i;

    for(i = 0; i < FAILED_SIZE; ++i)
        if(failed_addr[i].lap == addr->lap && failed_addr[i].nap == addr->nap && failed_addr[i].uap == addr->uap)
            return 1;
    failed_addr[failed_next] = *addr;
    ++failed_next;
    if(failed_next == FAILED_SIZE) failed_next = 0;
    return 0;
}
#endif

void handleCLMessage( Task task, MessageId id, Message message )
{
    hsTaskData * lApp = (hsTaskData *) getAppTask() ;
    
    switch (id)
    {
    case CL_INIT_CFM:
        CL_MSG_DEBUG(("CL_INIT_CFM\n"));
        if(((CL_INIT_CFM_T *)message)->status == success)
        {
            if (((CL_INIT_CFM_T*)message)->version == bluetooth2_1)
            {
                CL_MSG_DEBUG(("BLUETOOTH 2.1 MODE\n"));
                /* set EIR inquiry mode */
                ConnectionWriteInquiryMode(&lApp->task, inquiry_mode_eir);
            }
            else
            {
                /* no 2.1 EIR init to do -> initialise the HFP library now */
                InitHfp(lApp);
            }
        }
        else
            Panic();
        break;
    case CL_DM_INQUIRE_RESULT: /* For AG inquire */
        CL_MSG_DEBUG(("CL_DM_INQUIRE_RESULT\n"));
        if (((CL_DM_INQUIRE_RESULT_T*)message)->status == inquiry_status_ready)
        {
            CL_MSG_DEBUG(("Inquiry complete\n"));
            /* Inquiry complete. End of inquiry */
            intercomInquiryComplete(lApp);
        }
        else
        {
            CL_MSG_DEBUG(("Found device\n"));
            /* Inquiry Result. We found a device! */
#ifdef S100A /* v101201 Repairing fail for same addr */
            intercomInquiryResult(lApp, (CL_DM_INQUIRE_RESULT_T*)message);
#else
            if(!tried_and_failed(&((CL_DM_INQUIRE_RESULT_T*)message)->bd_addr))
            {
                CL_MSG_DEBUG(("Not already tried to connect\n"));
                intercomInquiryResult(lApp, (CL_DM_INQUIRE_RESULT_T*)message);
            }
            else
                CL_MSG_DEBUG(("Already tried to connect\n"));
#endif
        }
        break;
    case CL_DM_WRITE_INQUIRY_MODE_CFM:
        CL_MSG_DEBUG(("CL_DM_WRITE_INQUIRY_MODE_CFM\n"));
        /* Read the local name to put in our EIR data */
        ConnectionReadLocalName(&lApp->task);
        break;
    case CL_DM_LOCAL_NAME_COMPLETE:
        CL_MSG_DEBUG(("CL_DM_LOCAL_NAME_COMPLETE\n"));
        /* write the EIR data and then initialise HFP */
        headsetWriteEirData(lApp, (CL_DM_LOCAL_NAME_COMPLETE_T*)message);
        /* done 2.1 EIR init -> now initialise HFP */
        InitHfp(lApp);
        break;
    case CL_SM_USER_CONFIRMATION_REQ_IND:
        CL_MSG_DEBUG(("CL_SM_USER_CONFIRMATION_REQ_IND\n"));
        headsetHandleUserConfirmationInd(lApp, (CL_SM_USER_CONFIRMATION_REQ_IND_T*)message);
        break;
    case CL_SM_USER_PASSKEY_REQ_IND:
        CL_MSG_DEBUG(("CL_SM_USER_PASSKEY_REQ_IND\n"));
        headsetHandleUserPasskeyInd(lApp, (CL_SM_USER_PASSKEY_REQ_IND_T*)message);
        break;
    case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
        CL_MSG_DEBUG(("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n"));
        headsetHandleUserPasskeyNotificationInd(lApp, (CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*)message);
        break;
    case CL_SM_KEYPRESS_NOTIFICATION_IND:
        CL_MSG_DEBUG(("CL_SM_KEYPRESS_NOTIFICATION_IND\n"));
        /* no action required? */
        break;
    case CL_SM_REMOTE_IO_CAPABILITY_IND: 
        CL_MSG_DEBUG(("CL_SM_REMOTE_IO_CAPABILITY_IND\n"));
        headsetHandleRemoteIoCapabilityInd(lApp, (CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
        break;
    case CL_SM_IO_CAPABILITY_REQ_IND:
        CL_MSG_DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
        headsetHandleIoCapabilityInd(lApp, (CL_SM_IO_CAPABILITY_REQ_IND_T*)message);
        break;
    case CL_SM_SEC_MODE_CONFIG_CFM:
        CL_MSG_DEBUG(("CL_SM_SEC_MODE_CONFIG_CFM\n"));
        /* remember if debug keys are on or off */
        lApp->debugKeysInUse = ((CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
        break;
    case CL_SM_PIN_CODE_IND:  
        CL_MSG_DEBUG(("CL_SM_PIN_CODE_IND\n"));
        headsetHandlePinCodeInd(lApp, (CL_SM_PIN_CODE_IND_T*) message);
        break;
    case CL_SM_AUTHORISE_IND:  
        CL_MSG_DEBUG(("CL_SM_AUTHORISE_IND\n"));
        headsetHandleAuthoriseInd(lApp , (CL_SM_AUTHORISE_IND_T*) message);
        break;            
    case CL_SM_AUTHENTICATE_CFM:
        CL_MSG_DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
        headsetHandleAuthenticateCfm(lApp, (CL_SM_AUTHENTICATE_CFM_T*) message);
        break;     
        
    /* Ignored messages */
    case CL_DM_ACL_OPENED_IND:
    case CL_DM_ACL_CLOSED_IND:
    case CL_SM_ENCRYPT_CFM:
    case CL_DM_DUT_CFM:
    case CL_DM_ROLE_CFM:
    case CL_DM_ROLE_IND:
    case CL_DM_SNIFF_SUB_RATING_IND:
    case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        break;
        
    default:   
        CL_MSG_DEBUG(("CL UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}
