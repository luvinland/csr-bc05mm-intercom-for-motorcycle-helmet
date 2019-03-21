/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_hfp_slc.c
@brief    Handle HFP SLC.
*/

#include "headset_a2dp_connection.h"
#include "headset_a2dp_stream_control.h"
#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_hfp_call.h"
#include "headset_hfp_slc.h"
#include "headset_statemanager.h"
#include "headset_volume.h"

#include <bdaddr.h>
#include <hfp.h>
#include <panic.h>
#include <ps.h>

#ifdef DEBUG_HFP_SLC
#define HFP_SLC_DEBUG(x) DEBUG(x)
#else
#define HFP_SLC_DEBUG(x) 
#endif


/****************************************************************************
  FUNCTIONS
*/


/****************************************************************************/
void hfpSlcReset( hsTaskData * pApp )
{
    HFP_SLC_DEBUG(("SLC: reset\n")) ;

	pApp->slcConnecting = FALSE;
	pApp->slcConnectFromPowerOn = FALSE;	
}  


/****************************************************************************/
bool hfpSlcIsConnecting ( hsTaskData * pApp )
{
	return pApp->slcConnecting;	
}


/*****************************************************************************/
void hfpSlcConnectSuccess ( hsTaskData * pApp , HFP * pProfile, Sink sink )
{
    bdaddr ag_addr;
#ifdef R100
    uint8 lslavemode;
#endif

	HFP_SLC_DEBUG(("HFP: Connected[%x]\n", (uint16)sink)) ;
    
    pApp->slcConnecting = FALSE;
	
    if ( SinkGetBdAddr ( sink, &ag_addr ) )
    {
#ifdef SINPUNG
        if((ag_addr.nap == 0x24 && ag_addr.uap == 0xbc) || (ag_addr.nap == 0x15 && ag_addr.uap == 0x45)) /* Slc con attemp to last used INT */
#else
        if(ag_addr.nap == 0x24 && ag_addr.uap == 0xbc) /* Slc con attemp to last used INT */
#endif
        {
#ifdef R100
            lslavemode = 1;
            pApp->slave_function = TRUE;

            (void) PsStore(PSKEY_SLAVE_MODE, &lslavemode, sizeof(uint8)); /* Slave Mode memory */
            (void) PsStore(PSKEY_LAST_USED_INT, &ag_addr, sizeof(bdaddr));
#ifdef S100A
            (void)PsStore(12, 0, 0);
#endif
#endif
#ifdef SINPUNG
            if(ag_addr.nap == 0x15 && ag_addr.uap == 0x45) pApp->slave_function_sinpung = TRUE;
#endif
        }
        else
        {
            pApp->repeat_stop = FALSE;
            MessageSend(&pApp->task, EventSkipBackward, 0); /* R100 */
            hfpSlcStoreBdaddr ( &ag_addr );
        }
    }

	/* Enter connected state if not already connected */
	if (!stateManagerIsHfpConnected())
	{
	    stateManagerEnterHfpConnectedState( pApp );	
	}
    else
    {
        MessageCancelAll ( &pApp->task , EventPairingFail ) ; 
    }
	
	/* Resume A2DP streaming if any existed before connection attempt */
	streamControlResumeA2dpStreaming( pApp, 0);
 
	if (pProfile == pApp->hfp)
    {
        pApp->profile_connected = hfp_handsfree_profile;
		pApp->hfp_hsp = pApp->hfp;
    }
    else if (pProfile == pApp->hsp)
    {
		pApp->intercom_hsp = pApp->hsp;
    }    

	/* Reinitialise HFP Volume level with stored values */
	VolumeInitHfp(pApp);

    /* Ensure the underlying ACL is encrypted */       
    ConnectionSmEncrypt( &pApp->task , sink , TRUE );
	
	/* Disable NR/EC on AG if it's supported. This is called only if CVC is used on the headset. */
    if ((pApp->profile_connected == hfp_handsfree_profile) && pApp->cvcEnabled)
        HfpDisableNrEc(pApp->hfp);
    
    /* Send a user event to the app for indication purposes */
    MessageSend ( &pApp->task , EventSLCConnected , 0 );
	
	/* Send an event to connect to last used A2DP source if this connection was from power on */
	if (pApp->slcConnectFromPowerOn)
	{	
		/* Only connect A2DP to a combined device if no call is ongoing */
		if (stateManagerGetHfpState() == headsetHfpConnected)
		{
    		MessageSendLater(&pApp->task, EventEstablishA2dp, 0, 1000); /* TTA & A2DP conn */
		}
		else
		{
			pApp->connect_a2dp_when_no_call = TRUE;
			pApp->slcConnectFromPowerOn = FALSE;
		}
	}
	
	PROFILE_MEMORY(("HFPConnect"))
}


/*****************************************************************************/
void hfpSlcConnectFail( hsTaskData *pApp )
{
	bool connecting_a2dp = FALSE;
    /* Update the app state */  
    HFP_SLC_DEBUG(("SLC : Connect Fail \n")) ; 

	/* Send an event to connect to last used A2DP source if this connection was from power on, 
	   and it's not the same device as the last used AG. */ /* TTA & A2DP conn */
	if (pApp->slcConnectFromPowerOn)
	{
		bdaddr ag_addr, a2dp_addr;
		uint8 seid;
		
		if (a2dpGetLastUsedSource(pApp, &a2dp_addr, &seid))
		{
			if (hfpSlcGetLastConnectedAG(&ag_addr))
			{
				if (!BdaddrIsSame(&a2dp_addr, &ag_addr))
				{
					connecting_a2dp = TRUE;
    				MessageSend ( &pApp->task , EventEstablishA2dp , 0 );
				}
			}
			else
			{
				connecting_a2dp = TRUE;
				MessageSend ( &pApp->task , EventEstablishA2dp , 0 );	
			}
		}
	}

    pApp->slcConnecting = FALSE; /*reset the connecting flag*/ 
    pApp->repeat_stop = FALSE;

	if (!connecting_a2dp)
		pApp->slcConnectFromPowerOn = FALSE;
	
	/* Resume A2DP streaming if any existed before connection attempt */
	streamControlResumeA2dpStreaming( pApp, 0);

    /* No connection */
    pApp->profile_connected = hfp_no_profile;
    
    /* Send event to signify that all reconnection attempts failed */
	MessageSend(&pApp->task, EventHfpReconnectFailed, 0);
    
    /* Clear the queue */
    hfpCallClearQueuedEvent ( pApp ) ;
    
    PROFILE_MEMORY(("HFPConnectFail"))
}


/****************************************************************************/
void hfpSlcConnectRequest( hsTaskData *pApp, hfp_profile pProfile )
{
    bdaddr lLastAddr ;   
	
	if (stateManagerGetHfpState() == headsetPoweringOn)
	{
		HFP_SLC_DEBUG(("SLC: Already powered off\n"));
		return;
	}
    
    pApp->slcConnecting = TRUE;    
    
    /* Retrieve the address of the last used AG from PS */
	if (!hfpSlcGetLastUsedAG(&lLastAddr))
	{
		/* We have failed to connect as we don't have a valid address */
        HFP_SLC_DEBUG(("SLC: No Last Addr \n"));
		hfpSlcConnectFail ( pApp );
		return;
	}

    HFP_SLC_DEBUG(("SLC: Connect Req [%x]\n" , pProfile));
    hfpSlcAttemptConnect ( pApp , pProfile , &lLastAddr );	
}


/****************************************************************************/
void hfpSlcAttemptConnect( hsTaskData *pApp, hfp_profile pProfile , bdaddr * pAddr )
{
	
	/* Pause A2DP streaming if any */
	streamControlCeaseA2dpStreaming(pApp, TRUE);
	
    switch ( pProfile )
    {
        case (hfp_handsfree_profile) :
        {
            pApp->profile_connected = hfp_handsfree_profile;
           
            HFP_SLC_DEBUG(("SLC: Attempt HFP\n")) ;                    
            /* Issue a connect request for HFP */
            HfpSlcConnect(pApp->hfp, pAddr, 0);
        }    
        break;
        case (hfp_headset_profile) :
        {
            HFP_SLC_DEBUG(("SLC: Attempt HSP\n")) ;                    
            /* Issue a connect request for HFP */
            HfpSlcConnect(pApp->hsp, pAddr, 0);
        }   
        break;
        default:
            Panic();
            break;
    }
}


/****************************************************************************/
void hfpSlcDisconnect( hsTaskData *pApp )
{
    /* Issue the disconnect request and let the HFP lib do the rest */
    if(HfpGetSlcSink(pApp->hfp_hsp)) HfpSlcDisconnect(pApp->hfp_hsp);

    if(HfpGetSlcSink(pApp->intercom_hsp)) HfpSlcDisconnect(pApp->intercom_hsp);
}


/****************************************************************************/
void hfpSlcStoreBdaddr ( const bdaddr * pAddr ) 
{
    (void) PsStore(PSKEY_LAST_USED_AG, pAddr, sizeof(bdaddr));
}


/****************************************************************************/
bool hfpSlcGetLastUsedAG(bdaddr *addr)
{
	bool ret = TRUE;
	
  	if (!PsRetrieve(PSKEY_LAST_USED_AG, addr, sizeof(bdaddr)))
	{
		if (!PsRetrieve(PSKEY_LAST_PAIRED_DEVICE, addr, sizeof(bdaddr)))
		{ 
			ret = FALSE;
		}
	}
	
	return ret;
}


/****************************************************************************/
bool hfpSlcGetLastConnectedAG(bdaddr *addr)
{
	bool ret = TRUE;
	
  	if (!PsRetrieve(PSKEY_LAST_USED_AG, addr, sizeof(bdaddr)))
	{
	
		ret = FALSE;
	}
	
	return ret;
}

