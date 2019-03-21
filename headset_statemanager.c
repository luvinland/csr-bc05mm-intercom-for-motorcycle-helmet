/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_statemanager.c
@brief    State machine helper functions used for state changes etc - provide single state change points etc for the headset app.
*/

#include "headset_a2dp_connection.h"
#include "headset_a2dp_stream_control.h"
#include "headset_buttonmanager.h"
#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_hfp_slc.h"
#include "headset_LEDmanager.h"
#include "headset_scan.h"
#include "headset_statemanager.h"
#include "headset_volume.h"

#include <audio.h>
#include <bdaddr.h>
#include <pio.h>
#include <ps.h>
#include <stdlib.h>

#ifdef DEBUG_STATES

#include <panic.h>

#define SM_DEBUG(x) DEBUG(x)
#define SM_ASSERT(c, x) { if (!(c)) { DEBUG(x); Panic();} }

const char * const gHSStateStrings [ 8 ] = {
                               "Limbo",
                               "ConnDisc",
                               "Connectable",
                               "Connected",
                               "Out",
                               "Inc",
                               "ActiveCall",
                               "TESTMODE"} ;

const char * const gA2DPStateStrings [HEADSET_NUM_A2DP_STATES] = {
                               "Connectable",
                               "Connected",
                               "Streaming",
                               "Paused"} ;

#else
#define SM_DEBUG(x) 
#define SM_ASSERT(c, x)

#endif


#define SM_LIMBO_TIMEOUT_SECS (5)


typedef struct
{
    /* The hfp headset state variable - accessed only from below fns */
    headsetHfpState gTheHfpState:4;
    /* The a2dp headset state variable - accessed only from below fns */
    headsetA2dpState gTheA2dpState:3 ;
    /* The avrcp headset state variable - accessed only from below fns */
    headsetAvrcpState gTheAvrcpState:3 ;
} headsetAppStates;

static headsetAppStates appStates;


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/

static void stateManagerSetHfpState ( hsTaskData * pApp , headsetHfpState pNewState ) ;

static void stateManagerSetA2dpState ( hsTaskData * pApp , headsetA2dpState pNewState ) ;


/****************************************************************************
    FUNCTIONS
*/

/*****************************************************************************/
headsetHfpState stateManagerGetHfpState ( void )
{
    return appStates.gTheHfpState ;
}


/*****************************************************************************/
headsetA2dpState stateManagerGetA2dpState ( void )
{
    return appStates.gTheA2dpState ;
}


/*****************************************************************************/
uint16 stateManagerGetCombinedState ( void )
{
    return (appStates.gTheHfpState + (HEADSET_NUM_HFP_STATES * appStates.gTheA2dpState));
}


/*****************************************************************************/
void stateManagerEnterHfpConnectableState ( hsTaskData *pApp, bool req_disc )
{   
    headsetHfpState lOldHfpState = stateManagerGetHfpState() ;

    
#ifdef S100A /* Intercom Pairing mode out. v110422 */
    if(pApp->intercom_pairing_mode)
    {
        pApp->intercom_pairing_mode = FALSE;
    
        /*disable the discoverable mode*/
        headsetDisableDiscoverable ( pApp) ;
        MessageCancelAll ( &pApp->task , EventPairingFail ) ;        
    
        MessageSend(&pApp->task, EventSkipForward, 0);
        MessageSendLater(&pApp->task, EventSkipBackward, 0, D_SEC(1));
    }
    else
    {
#endif
        if ( stateManagerIsHfpConnected() && req_disc )
        {       /*then we have an SLC active*/
           hfpSlcDisconnect( pApp );
        }
        
        /* Make the headset connectable */
        headsetEnableConnectable(pApp);
        
        stateManagerSetHfpState ( pApp , headsetHfpConnectable) ;
        
        /*determine if we have got here after a DiscoverableTimeoutEvent*/
        if ( lOldHfpState == headsetConnDiscoverable )
        {   
            /*disable the discoverable mode*/
            headsetDisableDiscoverable ( pApp) ;
            MessageCancelAll ( &pApp->task , EventPairingFail ) ;        
        }
    }
}


/*****************************************************************************/
void stateManagerEnterA2dpConnectableState ( hsTaskData *pApp, bool req_disc  )
{   	    
    if ( stateManagerIsA2dpConnected() && req_disc )
    {      
       a2dpDisconnectRequest( pApp );
    }
	
	if ( stateManagerIsA2dpStreaming() && stateManagerIsAvrcpConnected() )
	{
	    /* Store the current playing status of the media, so the playing status
		  can now be tracked independently of the A2DP state. This is because 
		  AVRCP commands can be sent from the headset while not in the streaming state.
		*/
		if (stateManagerGetA2dpState() == headsetA2dpStreaming)
			pApp->PlayingState = 1;
		else
			pApp->PlayingState = 0;
	}
    
    /* Make the headset connectable if it's turned on */
	if (stateManagerGetHfpState() != headsetPoweringOn)
    	headsetEnableConnectable(pApp);
	
	stateManagerSetA2dpState(pApp , headsetA2dpConnectable);
}

void stateManagerEnterA2dpConnectedState(hsTaskData * pApp)
{
	if ( stateManagerIsA2dpSignallingActive ( pApp ) )
	{
   		stateManagerSetA2dpState(pApp , headsetA2dpConnected);

#ifndef BNFON
    	if ( stateManagerIsHfpConnected() && !pApp->audio_connect && (pApp->aghfp_connect || pApp->slave_function)) /* SUSPEND */
        {      
            headsetDisableConnectable  ( pApp ) ;
        }
#endif

#if 0 /* TTA & A2DP conn */
		/* Must not be connectable if Faststream is in use */
		if (pApp->seid == FASTSTREAM_SEID)
			headsetDisableConnectable  ( pApp ) ;
#endif
	}
	
	if ( stateManagerIsA2dpStreaming() && stateManagerIsAvrcpConnected() )
	{
		/* Store the current playing status of the media, so the playing status
		  can now be tracked independently of the A2DP state. This is because 
		  AVRCP commands can be sent from the headset while not in the streaming state.
		*/
		if (stateManagerGetA2dpState() == headsetA2dpStreaming)
			pApp->PlayingState = 1;
		else
			pApp->PlayingState = 0;
	}
}

/*****************************************************************************/
void stateManagerEnterA2dpStreamingState(hsTaskData * pApp)
{
   	stateManagerSetA2dpState(pApp , headsetA2dpStreaming);
}

/*****************************************************************************/
void stateManagerEnterA2dpPausedState(hsTaskData * pApp)
{
   	stateManagerSetA2dpState(pApp , headsetA2dpPaused);
}


/*****************************************************************************/
void stateManagerEnterConnDiscoverableState ( hsTaskData *pApp )
{
    MessageSend(&pApp->task, EventSkipForward, 0);

#ifdef S100A /* v100622 Intercom Pairing mode */
    if(!pApp->intercom_pairing_mode)
    {
        if ( stateManagerIsHfpConnected() )
        {       
            hfpSlcDisconnect( pApp );
        }

        if ( stateManagerIsA2dpConnected() )
        {      
           a2dpDisconnectRequest( pApp );
        }
    }
#else
    if ( stateManagerIsHfpConnected() )
    {       
        hfpSlcDisconnect( pApp );
    }
	
	if ( stateManagerIsA2dpConnected() )
    {      
       a2dpDisconnectRequest( pApp );
    }
#endif
    
    /* Make the headset connectable */
    headsetEnableConnectable ( pApp );
    
    /* Make the headset discoverable */  
    headsetEnableDiscoverable ( pApp );    
    
    if(!pApp->intercom_pairing_mode)
    {
        /* The headset is now in the connectable/discoverable state */
        stateManagerSetHfpState (pApp , headsetConnDiscoverable ) ;
    }
           
    /* Cancel Pairing mode after a configurable number of secs */
    MessageSendLater ( &pApp->task , EventPairingFail , 0 , D_SEC(pApp->Timeouts.PairModeTimeout_s) ) ;
}


/*****************************************************************************/
void stateManagerEnterHfpConnectedState ( hsTaskData *pApp )
{
	headsetHfpState hfpState = stateManagerGetHfpState (); 
			
    if (hfpState != headsetHfpConnected )
    {
        headsetDisableDiscoverable ( pApp ) ;

#ifndef BNFON
        if ( stateManagerIsA2dpSignallingActive ( pApp ) && (pApp->aghfp_connect || pApp->slave_function) )
            headsetDisableConnectable  ( pApp ) ;
#endif

		if ( (hfpState == headsetActiveCall) && (pApp->dsp_process == dsp_process_sco) && pApp->cvcEnabled )
        {
			AudioSetMode (AUDIO_MODE_MUTE_SPEAKER , NULL) ;
		}

        switch ( hfpState )
        {
            case headsetActiveCall:
            case headsetOutgoingCallEstablish:
            case headsetIncomingCallEstablish:
                /* We have just ended a call */
			
				streamControlResumeA2dpStreaming(pApp, 0);
				
                MessageSend ( &pApp->task , EventEndOfCall , 0 ) ;
                break;
            default:
                break;                      
        }
        
        MessageCancelAll ( &pApp->task , EventPairingFail ) ;
        
        stateManagerSetHfpState ( pApp , headsetHfpConnected ) ;
    }
}


/*****************************************************************************/
void stateManagerEnterIncomingCallEstablishState ( hsTaskData * pApp )
{
    /*headsetHfpState lState =  stateManagerGetHfpState() ;*/
    
    stateManagerSetHfpState ( pApp , headsetIncomingCallEstablish ) ;
}


/*****************************************************************************/
void stateManagerEnterOutgoingCallEstablishState ( hsTaskData * pApp )
{
    stateManagerSetHfpState ( pApp , headsetOutgoingCallEstablish ) ;
}


/*****************************************************************************/
void stateManagerEnterActiveCallState ( hsTaskData * pApp )   
{
    stateManagerSetHfpState ( pApp , headsetActiveCall ) ;
	
	/* Update the SCO audio mode if the SCO is already running */
	if ( (pApp->dsp_process == dsp_process_sco) && pApp->cvcEnabled )
	{
		if (pApp->gMuted)
			AudioSetMode ( AUDIO_MODE_MUTE_MIC , NULL ) ;
		else
			AudioSetMode( AUDIO_MODE_CONNECTED, NULL);
	}
}


/*****************************************************************************/
void stateManagerEnterPoweringOffState ( hsTaskData *pApp )
{
	if ( stateManagerIsA2dpConnected() )
    {      
      	a2dpDisconnectRequest( pApp );
    }
	if ( stateManagerIsHfpConnected() )
    {       
        hfpSlcDisconnect( pApp );
    }

    if(!pApp->reset_complete) VolumeStoreLevels(pApp);

    /* Now just waiting for switch off */
    stateManagerEnterLimboState ( pApp ) ;
}


/*****************************************************************************/
void stateManagerPowerOff ( hsTaskData * pApp ) 
{
    /* Update state in case we are debugging */
    stateManagerSetHfpState ( pApp , headsetPoweringOn) ;
    /* Attempt to physically power down device - this will fail if the chg is plugged in */
	PioSetPsuRegulator ( FALSE ) ;
}


/*****************************************************************************/
void stateManagerPowerOn ( hsTaskData * pApp ) 
{
    bdaddr addr;

    /* Cancel the event message if there was one so it doesn't power off */
    MessageCancelAll ( &pApp->task , EventLimboTimeout ) ;
    
    PioSetPsuRegulator ( TRUE ) ;
    
    /* Reset sec mode config - always turn off debug keys on power on */
    ConnectionSmSecModeConfig(&pApp->task,
                             cl_sm_wae_acl_owner_none,
                             FALSE,
                             TRUE);

#ifndef S100A /* v101201 Power On connect to intercom addr */
    if(!PsRetrieve(PSKEY_LAST_USED_AG, &addr, sizeof(bdaddr)) && !PsRetrieve(PSKEY_LAST_PAIRED_DEVICE, &addr, sizeof(bdaddr)))
#else
    if(!PsRetrieve(PSKEY_LAST_USED_AG, &addr, sizeof(bdaddr)) && !PsRetrieve(PSKEY_LAST_USED_AV_SOURCE, &addr, sizeof(bdaddr)))
#endif

    {
        stateManagerEnterConnDiscoverableState(pApp);
    }
    else
    {
        stateManagerEnterHfpConnectableState ( pApp, TRUE );
        stateManagerEnterA2dpConnectableState ( pApp, TRUE );
    }
}


/*****************************************************************************/
void stateManagerEnterLimboState ( hsTaskData * pApp )
{
    PioSetPsuRegulator ( TRUE ) ;
    
    headsetDisableDiscoverable ( pApp ) ;    
    headsetDisableConnectable  ( pApp ) ;
   
    /* Set a timeout so that we will turn off eventually anyway */
    MessageSendLater ( &pApp->task , EventLimboTimeout , 0 , D_SEC(5) ) ;

    stateManagerSetHfpState ( pApp , headsetPoweringOn) ;
	
	pApp->connect_a2dp_when_no_call = FALSE;
    
    /* Reset SLC settings */
    hfpSlcReset( pApp );
}


/*****************************************************************************/
void stateManagerUpdateLimboState ( hsTaskData * pApp ) 
{
     /* We are entering here as a result of a power off */
     switch (pApp->charger_state)
        {
            case disconnected :
                /* Power has been removed and we are logically off so switch off */
                SM_DEBUG(("SM: LimboDiscon\n")) ;
                stateManagerPowerOff ( pApp ) ;
            break ;    
                /* This means connected */
            case trickle_charge:
            case fast_charge:
			case charge_error:
                SM_DEBUG(("SM: LimboConn\n")) ;
                /* Stay in this state until a charger event or a power on occurs */
            break ;
               
            default:
            break ;
        }  
}


/*****************************************************************************/
bool stateManagerIsHfpConnected ( void )
{
    bool lIsConnected = FALSE ;
    
    switch (stateManagerGetHfpState() )
    {
        case headsetPoweringOn:
        case headsetHfpConnectable:
        case headsetConnDiscoverable:  
        case headsetTestMode:
            lIsConnected = FALSE ;    
            break ;
        
        default:
            lIsConnected = TRUE ;
            break ;
    }
    return lIsConnected ;
}


/*****************************************************************************/
bool stateManagerIsA2dpConnected ( void )
{
    bool lIsConnected = FALSE ;
    
    switch (stateManagerGetA2dpState() )
    {
        case headsetA2dpConnectable:   
            lIsConnected = FALSE ;    
            break ;
            
        default:
            lIsConnected = TRUE ;
            break ;
    }
    return lIsConnected ;
}


/*****************************************************************************/
bool stateManagerIsAvrcpConnected ( void )
{
	return (stateManagerGetAvrcpState() == avrcpConnected);
}


/****************************************************************************/
bool stateManagerIsA2dpStreaming(void)
{
	bool lRet = FALSE;
	if ((stateManagerGetA2dpState() == headsetA2dpStreaming) || (stateManagerGetA2dpState() == headsetA2dpPaused))
		lRet = TRUE;
		
	return lRet;
}

/****************************************************************************/
bool stateManagerIsA2dpSignallingActive(hsTaskData * pApp)
{
	bool lRet = FALSE;
	
	if (A2dpGetSignallingSink(pApp->a2dp))
		lRet = TRUE;
		
	return lRet;
}


/*****************************************************************************/
void stateManagerEnterTestModeState ( hsTaskData * pApp )
{
	/* Cancel the event message if there was one so it doesn't power off */
    MessageCancelAll ( &pApp->task , EventLimboTimeout ) ;
	
    stateManagerSetHfpState (pApp , headsetTestMode ) ;
}


/*****************************************************************************/
void stateManagerSetAvrcpState ( hsTaskData* pApp, headsetAvrcpState state )
{
	SM_DEBUG(("SM (AVRCP): AvrcpSetState : From %d to %d\n", appStates.gTheAvrcpState, state));	
	appStates.gTheAvrcpState = state;
}


/*****************************************************************************/
headsetAvrcpState stateManagerGetAvrcpState ( void )
{
	return appStates.gTheAvrcpState;
}


/****************************************************************************
NAME	
	stateManagerSetHfpState

DESCRIPTION
	Helper function to Set the current hfp headset state
    provides a single state change point and passes the information
    on to the managers requiring state based responses. 
    
*/
static void stateManagerSetHfpState ( hsTaskData * pApp , headsetHfpState pNewState )
{
	SM_DEBUG(("SM (HFP):[%s]->[%s][%d]\n",gHSStateStrings[stateManagerGetHfpState()] , gHSStateStrings[pNewState] , pNewState ));
    
    if ( pNewState < HEADSET_NUM_HFP_STATES )
    {

        if (pNewState != appStates.gTheHfpState )
        {
            /* Inform the LED manager of the current state to be displayed */
            LEDManagerIndicateState ( &pApp->theLEDTask , pNewState , stateManagerGetA2dpState () ) ;
        }
        else
        {
            /* We are already indicating this state no need to set */
        }
   
        appStates.gTheHfpState = pNewState ;
   
    }
    else
    {
        SM_DEBUG(("SM (HFP): ? [%s] [%x]\n",gHSStateStrings[ pNewState] , pNewState)) ;
    }
    
    /*if we are in chargererror then reset the leds and reset the error*/
    if (pApp->charger_state == charge_error )
    {
       /* Cancel current LED indication */
	   MessageSend(getAppTask(), EventCancelLedIndication, 0);
	   /* Indicate charger error */
	   MessageSend(getAppTask(), EventChargeError, 0);
    }
}


/****************************************************************************
NAME	
	stateManagerSetA2dpState

DESCRIPTION
	Helper function to Set the current a2dp headset state
    provides a single state change point and passes the information
    on to the managers requiring state based responses. 
    
*/
static void stateManagerSetA2dpState ( hsTaskData * pApp , headsetA2dpState pNewState )
{
	SM_ASSERT((pNewState < HEADSET_NUM_A2DP_STATES), ("SM (A2DP): Invalid New State [%d]\n", pNewState));
	
    SM_DEBUG(("SM (A2DP):[%s]->[%s][%d]\n",gA2DPStateStrings[stateManagerGetA2dpState()] , gA2DPStateStrings[pNewState] , pNewState ));
	appStates.gTheA2dpState = pNewState ;
	
	/*if we are in chargererror then reset the leds and reset the error*/
    if (pApp->charger_state == charge_error )
    {
       /* Cancel current LED indication */
	   MessageSend(getAppTask(), EventCancelLedIndication, 0);
	   /* Indicate charger error */
	   MessageSend(getAppTask(), EventChargeError, 0);
    }
}



