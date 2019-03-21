/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_event_handler.c
@brief    Handle events arriving at the app.
*/

#include "headset_a2dp_connection.h"
#include "headset_avrcp_event_handler.h"
#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_event_handler.h"
#include "headset_events.h"
#include "headset_hfp_call.h"
#include "headset_hfp_slc.h"
#include "headset_LEDmanager.h"
#include "headset_powermanager.h"
#include "headset_tones.h"
#include "headset_statemanager.h"
#include "headset_volume.h"
#include "headset_auth.h"
#include "hfp.h"

#include <boot.h>
#include <ps.h>

static bool lPowerOffLock = FALSE;
static bool lshutdown = FALSE;

#ifdef DEBUG_EVENTS
#define EVENTS_DEBUG(x) DEBUG(x)
#else
#define EVENTS_DEBUG(x) 
#endif

#ifdef FAVORITES_CALL
#define PSKEY_FC_NUMBER 13
#endif

/****************************************************************************
  FUNCTIONS
*/

#include <pio.h>
#ifdef Z100_CLASS1
#define POWER_AMP_MASK ((uint16)1 << 10)
#define AMP_GAIN_MASK ((uint16)1 << 13)
#else
#define AMP_GAIN_MASK ((uint16)1 << 0)
#define ASW_PIN_MASK ((uint16)1 << 1)
#endif
#define MSP_HIGH_MASK ((uint16)1 << 9) /* R100 */
#ifdef AUTO_MIC_DETECT
#define EXT_MIC_PIN_MASK ((uint16)1 << 13)
#endif


#ifdef Z100_CLASS1
static void powerampOn(void)
{
    PioSetDir(POWER_AMP_MASK, POWER_AMP_MASK);
    PioSet(POWER_AMP_MASK, POWER_AMP_MASK);
}
#endif


static void ampGainTrans(hsTaskData* pApp)
{
    if(pApp->gHfpVolumeLevel > 4 || pApp->gAvVolumeLevel > 4)
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, AMP_GAIN_MASK);
    }
    else
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, 0);
    }
}


#ifdef S100A /* GOLDWING_v101020 */
#define PSKEY_TARGET_BDADDR 12

#ifdef AUTO_MIC_DETECT
static void automicDetect( void )
{
    if(PioGet() & 0x800)
    {
        PioSetDir(ASW_PIN_MASK, ASW_PIN_MASK);
        PioSet(ASW_PIN_MASK, ASW_PIN_MASK);
#ifndef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        PioSetDir(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
        PioSet(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
#endif
    }
    else
    {
        PioSetDir(ASW_PIN_MASK, ASW_PIN_MASK);
        PioSet(ASW_PIN_MASK, 0);
#ifndef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        PioSetDir(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
        PioSet(EXT_MIC_PIN_MASK, 0);
#endif
    }
}
#endif
#else
#ifndef Z100_CLASS1
static void micTrans(hsTaskData* pApp, bool powerOn)
{
    uint8 lbextmicmode = 0;

    if(!powerOn)
    {
        pApp->extmic_mode = pApp->extmic_mode == FALSE ? TRUE : FALSE;

        lbextmicmode = pApp->extmic_mode;
        (void) PsStore(PSKEY_EXTMIC_MODE, &lbextmicmode, sizeof(uint8));
    }

    if(pApp->extmic_mode)
    {
        PioSetDir(ASW_PIN_MASK, ASW_PIN_MASK);
        PioSet(ASW_PIN_MASK, ASW_PIN_MASK);
        if(!powerOn) MessageSend(&pApp->task, EventEnableLEDS, 0);
    }
    else
    {
        PioSetDir(ASW_PIN_MASK, ASW_PIN_MASK);
        PioSet(ASW_PIN_MASK, 0);
        if(!powerOn) MessageSend(&pApp->task, EventFFWDPress, 0);
    }
}
#endif
#endif


static void mSP430Trans(bool highSignal) /* R100 */
{
    if(highSignal)
    {
        PioSetDir(MSP_HIGH_MASK, MSP_HIGH_MASK);
        PioSet(MSP_HIGH_MASK, MSP_HIGH_MASK);
    }
    else
    {
        PioSetDir(MSP_HIGH_MASK, MSP_HIGH_MASK);
        PioSet(MSP_HIGH_MASK, 0);
    }
}


static void voiceControlTrans(hsTaskData* pApp, bool powerOn) /* v100225 */
{
    uint8 lbvoicemanualmode = 0;

    if(!powerOn)
    {
        pApp->voice_manual_mode = pApp->voice_manual_mode == FALSE ? TRUE : FALSE;

        lbvoicemanualmode = pApp->voice_manual_mode;
        (void) PsStore(PSKEY_VOICE_MANUAL_MODE, &lbvoicemanualmode, sizeof(uint8));
    }

    if(pApp->voice_manual_mode)
    {
#ifdef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        PioSetDir(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
        PioSet(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
#else
        mSP430Trans(TRUE);
#endif
        if(!powerOn) MessageSend(&pApp->task, EventEnableLEDS, 0);
    }
    else
    {
#ifdef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        PioSetDir(EXT_MIC_PIN_MASK, EXT_MIC_PIN_MASK);
        PioSet(EXT_MIC_PIN_MASK, 0);
#else
        mSP430Trans(FALSE);
#endif
        if(!powerOn) MessageSend(&pApp->task, EventFFWDPress, 0);
    }
}


/****************************************************************************/
void handleUEMessage( Task task, MessageId id, Message message )
{
    hsTaskData * lApp = (hsTaskData *) getAppTask() ;
    
    headsetHfpState lState = stateManagerGetHfpState() ;
    headsetA2dpState lA2dpState = stateManagerGetA2dpState() ;
    
    /* If we do not want the event received to be indicated then set this to FALSE. */
    bool lIndicateEvent = TRUE ;

    /* Deal with user generated Event specific actions*/
    switch ( id )
    {   
        /*these are the events that are not user generated and can occur at any time*/
        case EventOkBattery:
        case EventChargerDisconnected:
        case EventLEDEventComplete:
        case EventTrickleCharge:
        case EventLowBattery:
        case EventPowerOff:
        case EventLinkLoss:
        case EventSLCConnected:
		case EventA2dpConnected:
        case EventError:
        case EventChargeError:
        case EventCancelLedIndication:
		case EventHfpReconnectFailed:
		case EventA2dpReconnectFailed:
            /*do nothing for these events*/
            break ;
        default:
            /*handles the LED event timeouts - restarts state indications if we have had a user generated event only*/
            if (lApp->theLEDTask.gLEDSStateTimeout)
            {   
                LEDManagerIndicateState (&lApp->theLEDTask , lState , lA2dpState) ;     
                lApp->theLEDTask.gLEDSStateTimeout = FALSE ;
            }
            else
            {
                /*reset the current number of repeats complete - i.e restart the timer so that the leds will disable after
                the correct time*/
                LEDManagerResetStateIndNumRepeatsComplete  ( &lApp->theLEDTask ) ;
            }
   
        break;
    }

#ifndef LABRADOR
#ifdef AUTO_MIC_DETECT
    if((id == EventDisableLEDS || id == EventMuteReminder || id == EventAutoSwitchOff || id == EventVLongTimer) && lApp->dsp_process) return; /* 4s LED ON concept */ /* Battery Check */ /* v091111 Release */ /* v100225 */
#else
    if((id == EventDisableLEDS || id == EventToggleMute || id == EventMuteReminder || id == EventVLongTimer) && lApp->dsp_process) return; /* 4s LED ON concept */ /* Battery Check */ /* v091111 Release */
#endif
#endif

    switch (id)
    {
    case EventPowerOn:
    {
        bdaddr addr;

        if(lshutdown)
        {
            lIndicateEvent = FALSE;
            break;
        }

        if(PsRetrieve(PSKEY_LAST_USED_AG, &addr, sizeof(bdaddr)) && PsRetrieve(PSKEY_LAST_PAIRED_DEVICE, &addr, sizeof(bdaddr)))
        {
            lPowerOffLock = TRUE;
            MessageSendLater(&lApp->task, EventButtonLockingOff, 0, D_SEC(6));
        }

#ifdef S100A /* GOLDWING_v101020 */
        if(PsRetrieve(PSKEY_TARGET_BDADDR, &addr, sizeof(bdaddr)))
        {
                lApp->init_aghfp_power_on = TRUE;
                MessageSendLater(&lApp->task, EventRWDPress, 0, D_SEC(9));
        }
#endif

#ifdef Z100_CLASS1
        powerampOn();
        ampGainTrans(lApp);
#else
        ampGainTrans(lApp);
#ifdef S100A /* GOLDWING_v101020 */
#ifdef AUTO_MIC_DETECT
        if(PioGet() & 0x800) /* PowerOn External MIC Detect v110422 */
        {
            lApp->theButtonTask.gOldPioState |= 0x800;
        }

        automicDetect();
#endif
#else
        micTrans(lApp, TRUE);
#endif
#endif
        mSP430Trans(FALSE); /* R100 */
        voiceControlTrans(lApp, TRUE); /* v100225 */
        stateManagerPowerOn ( lApp );
        break;
    }
    case EventPowerOff:
        if(lApp->aghfp_connect) AghfpSlcDisconnect(lApp->aghfp);

        lshutdown = TRUE;

        if(lPowerOffLock)
        {
            lIndicateEvent = FALSE;
            break;
        }

        stateManagerEnterPoweringOffState ( lApp );
        AuthResetConfirmationFlags(lApp);

        hfpCallClearQueuedEvent ( lApp ) ;
        MessageCancelAll ( &lApp->task , EventPairingFail) ;
        break;
	case EventPowerOnConnect:
		lApp->slcConnectFromPowerOn = TRUE;
        MessageSend ( &lApp->task , EventEstablishSLC , 0 ) ;
        break;	
    case EventLimboTimeout:
        stateManagerUpdateLimboState ( lApp );
        break;
    case EventChargerConnected:
        powerManagerChargerConnected ( lApp );
        break;    
    case EventChargerDisconnected:
        powerManagerChargerDisconnected( lApp );
        if (lState == headsetPoweringOn )
        {
            stateManagerUpdateLimboState ( lApp ) ;
        }
        break;
    case EventTrickleCharge:  
        break;
    case EventFastCharge:
        break;
#ifdef FAVORITES_CALL
    case EventAutoSwitchOff:
        TonesPlayTone(lApp, 11, FALSE);
        MessageSendLater(&lApp->task, EventInitateVoiceDial, 0, D_SEC(3));
        break;
    case EventInitateVoiceDial:
    {
        uint16 pin_length = 0;
        uint8 pin[16];

        if((lApp->aghfp_connect && lApp->audio_connect) || lApp->slave_function)
        {
            lIndicateEvent = FALSE;
            break;
        }

        pin_length = PsRetrieve(PSKEY_FC_NUMBER, pin, 16);
        HfpDialNumber(lApp->hfp, pin_length, pin);
        break;
    }
#endif
    case EventAutoSwitchOff: /* v100225 */
#ifdef S100A /* Intercom Pairing mode out. v110422 */
        if(lApp->intercom_pairing_mode)
        {
            MessageSend(&lApp->task, EventPairingFail, 0);
        }
#endif
        voiceControlTrans(lApp, FALSE);
        break;
#ifdef S100A
    case EventInitateVoiceDial: /* v100622 Intercom Pairing mode */
        if (lState != headsetPoweringOn)
        {
            if(lApp->slave_function || lApp->aghfp_connect)
            {
                lApp->intercom_pairing_mode = FALSE;
                MessageSend(&lApp->task, EventEndOfCall, 0);
            }
            else
            {
                lApp->intercom_pairing_mode = TRUE;
                stateManagerEnterConnDiscoverableState ( lApp ) ;
            }
        }    
        break;
#endif
    case EventCancelLedIndication:
        LedManagerResetLEDIndications ( &lApp->theLEDTask ) ;            
		break ;  
    case EventOkBattery:
        break;
    case EventLowBattery:
        break;
    case EventEnterPairing:
        if (lState != headsetPoweringOn)
        {
            stateManagerEnterConnDiscoverableState ( lApp ) ;
        }    
        break ;
    case EventPairingFail:  
        if (lState != headsetTestMode)
        {
             stateManagerEnterHfpConnectableState( lApp, TRUE) ;          
        }
        break ;    
    case EventPairingSuccessful:
        if (lState == headsetConnDiscoverable)
        {
            stateManagerEnterHfpConnectableState( lApp , FALSE) ;
        }
        break ;
    case EventSLCConnected:
#ifdef S100A
        lApp->intercom_pairing_mode = FALSE; /* v100622 Intercom Pairing mode */
#endif
        hfpCallRecallQueuedEvent ( lApp ) ;
        break;
    case EventLinkLoss:
        break;
    case EventSLCDisconnected:
        if(!HfpGetSlcSink(lApp->intercom_hsp))
        {
            if(lApp->slave_function) lApp->slave_function = FALSE;
#ifdef SINPUNG
            if(lApp->slave_function_sinpung) lApp->slave_function_sinpung = FALSE;
#endif
        }
        break;
    case EventLongTimer:
        if (lState == headsetPoweringOn || (lApp->aghfp_connect && lApp->audio_connect) || (lApp->slave_function && (int)HfpGetAudioSink(lApp->intercom_hsp) && lApp->dsp_process)) /* Don't play music during intercom - v110422 */
            lIndicateEvent = FALSE ;
        break;
    case EventVLongTimer:
        if(lState != headsetConnDiscoverable && lState != headsetHfpConnectable && lState != headsetHfpConnected) lIndicateEvent = FALSE ; /* 4s LED ON concept */
        break;
    case EventLEDEventComplete:
        if ( (( LMEndMessage_t *)message)->Event  == EventResetPairedDeviceList )
        {
            /* The reset has been completed */
            MessageSend(&lApp->task , EventResetComplete , 0 ) ;
        }
        
        if (lApp->theLEDTask.Queue.Event1)
        {
             LEDManagerIndicateEvent (&lApp->theLEDTask , (EVENTS_EVENT_BASE + lApp->theLEDTask.Queue.Event1) ) ;
    
             /*shuffle the queue*/
             lApp->theLEDTask.Queue.Event1 = lApp->theLEDTask.Queue.Event2 ;
             lApp->theLEDTask.Queue.Event2 = lApp->theLEDTask.Queue.Event3 ;
             lApp->theLEDTask.Queue.Event3 = lApp->theLEDTask.Queue.Event4 ;
             lApp->theLEDTask.Queue.Event4 = 0x00 ;
        }
        break;
    case EventEstablishSLC:   
    {
        bdaddr addr;

        if(PsRetrieve(PSKEY_LAST_USED_AG, &addr, sizeof(bdaddr)) && PsRetrieve(PSKEY_LAST_PAIRED_DEVICE, &addr, sizeof(bdaddr)))
        {
            lPowerOffLock = TRUE;
            MessageSendLater(&lApp->task, EventButtonLockingOff, 0, D_SEC(6));
        }

        if(lApp->aghfp_connect && lApp->audio_connect)
        {
			lIndicateEvent = FALSE;
			break;
        }

		/* Don't indicate event if it's from power on */
		if (lApp->slcConnectFromPowerOn) lIndicateEvent = FALSE ;

		/* Only send a connect request if the headset isn't currently connecting HFP and the headset
		   is on and not in pairing mode.
		*/
        if (! hfpSlcIsConnecting (lApp)  && (lState >= headsetHfpConnectable) )
        {
            hfpSlcConnectRequest( lApp , hfp_handsfree_profile ) ;
        }   
        else
        {
            /*do not perform the action*/
            lIndicateEvent = FALSE ;
        } 
        break;
    }
    case EventHfpReconnectFailed:
        break;
	case EventA2dpReconnectFailed:
        break;
    case EventLastNumberRedial:
    {
		if (lApp->aghfp_connect && lApp->audio_connect)
		{
			lIndicateEvent = FALSE ;
			break;
		}
        hfpCallInitiateLNR(lApp);
        break ;
    }
    case EventButtonLockingOn: /* R100 */
        if(lApp->normal_answer) MessageSend(&lApp->task, EventAnswer, 0);
        break;
    case EventAnswer:
        if(lState == headsetIncomingCallEstablish) /* Auto Answer Update */
        {
            MessageSend(&lApp->task, EventSkipForward, 0);

            /* Modify Old Mobile Call connection */
            if(lApp->aghfp_connect && lApp->audio_connect)
            {
                AghfpAudioDisconnect(lApp->aghfp);
                MessageSendLater(&lApp->task, EventReject, 0, 1000); /* SUSPEND */
            }
            else if(lApp->slave_function && (int)HfpGetAudioSink(lApp->intercom_hsp) && lApp->dsp_process) /* v100129 */
            {
                HfpAudioDisconnect(lApp->intercom_hsp);
                MessageSendLater(&lApp->task, EventReject, 0, 1000);
            }
            else
                hfpCallAnswer( lApp );
        }
        break ;
    case EventReject:
        /* Modify Old Mobile Call connection */
        hfpCallAnswer( lApp );
        break ;
    case EventCancelEnd:
        /* Terminate the current ongoing call process */
        hfpCallHangUp( lApp );
        break ;
    case EventSCOLinkOpen :        
#ifdef FAVORITES_CALL
        HfpGetCurrentCalls(lApp->hfp);
#endif
        if(lApp->slave_function && lState == headsetHfpConnected)
            LEDManagerIndicateState(&lApp->theLEDTask, headsetActiveCall, lA2dpState);
        break ;
    case EventSCOLinkClose:        
        LEDManagerIndicateState(&lApp->theLEDTask, headsetHfpConnected, lA2dpState);
        break ;        
    case EventResetPairedDeviceList:
        if ( stateManagerIsHfpConnected () )
        {
            /* Then we have an SLC active */
            hfpSlcDisconnect( lApp );
        }             
		if ( stateManagerIsA2dpConnected() )
    	{      
       		a2dpDisconnectRequest( lApp );
    	}
        configManagerReset ( lApp ) ;
        break ;
#ifdef R100
    case EventRWDPress: /* R100 */
    {
#ifdef S100A /* v101201 Power On connect to intercom addr */
        bdaddr addr;

        if(!(lState == headsetConnDiscoverable && !(lApp->intercom_pairing_mode) && !PsRetrieve(12, &addr, sizeof(bdaddr)) && !PsRetrieve(14, &addr, sizeof(bdaddr))))
#else
        if(!(lState == headsetConnDiscoverable && !(lApp->intercom_pairing_mode))) /* v100817 Z100 Only button run during Intercom Pairing mode */
#endif
        {
            lApp->intercom_button = TRUE;
            MessageSend(&lApp->task, EventEnterDFUMode, 0);
        }
        else
        {
			lIndicateEvent = FALSE ;
			break;
        }
        break;
    }
    case EventEnterDFUMode:
    {
        bdaddr lcLastAddr;
        uint8 laslavemode = 0;

#ifdef SINPUNG
        if(lApp->slave_function_sinpung)
        {
            lApp->intercom_button = FALSE;
			lIndicateEvent = FALSE ;
			break;
        }
#endif

#ifdef R100 /* v110117 Miss match. For No intercom function before intercom pariring */
        if(!lApp->aghfp_connect && !lApp->slave_function && !lApp->intercom_button)
        {
            MessageSend(&lApp->task, EventSkipForward, 0);
            MessageSendLater(&lApp->task, EventSkipBackward, 0, D_SEC(1));
            break;
        }
#endif

        if(!((lApp->slave_function && (int)HfpGetAudioSink(lApp->intercom_hsp) && lApp->dsp_process) || (lApp->aghfp_connect && lApp->audio_connect) || lApp->init_aghfp_power_on))
        {
            TonesPlayTone(lApp, 14, TRUE); /* R100 */
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
            lApp->beep_audio_con = TRUE;
#endif
        }

        if(!lApp->repeat_stop)
        {
            lApp->repeat_stop = TRUE;
            (void) PsRetrieve(PSKEY_SLAVE_MODE, &laslavemode, sizeof(uint8)); /* Slave Mode memory */
            
            if(/*lState != headsetConnDiscoverable*/!lApp->intercom_pairing_mode && laslavemode) /* v100817 Z100 Only button run during Intercom Pairing mode */
            {
                if(lApp->slave_function)
                {
                    if(HfpGetAudioSink(lApp->intercom_hsp) && lApp->dsp_process)
                        HfpAudioDisconnect(lApp->intercom_hsp);
                    else
                    {
                        if(lApp->intercom_button) MessageSend(&lApp->task, EventSkipForward, 0); /* R100 */
                        HfpAudioConnect(lApp->intercom_hsp, sync_all_sco, 0);
                    }
                }
                else
                {
#ifdef S100A /* v101201 Power On connect to intercom addr */
                    if(lState == headsetConnDiscoverable || lState == headsetHfpConnectable || lState == headsetHfpConnected) /* Slc con attemp to last used INT */
#else
                    if(lState == headsetHfpConnectable || lState == headsetHfpConnected) /* Slc con attemp to last used INT */
#endif
                    {
                        lApp->slcConnecting = TRUE;

                        if(!PsRetrieve(PSKEY_LAST_USED_INT, &lcLastAddr, sizeof(bdaddr)))
                        {
                            lApp->intercom_button = FALSE; /* R100 */
                            /* We have failed to connect as we don't have a valid address */
                            hfpSlcConnectFail(lApp);
                            break;
                        }

                        hfpSlcAttemptConnect(lApp, hfp_headset_profile, &lcLastAddr);
                    }
                    else
                    {
                        lApp->repeat_stop = FALSE;
                        MessageSend(&lApp->task, EventError, 0);
                    }
                }
            }
            else
            {
                MessageSend(&lApp->task, APP_INTERCOM_MODE, 0);
            }
        }
        break ;    
    }
#endif
    case EventButtonLockingOff :
        lPowerOffLock = FALSE;
        break ;
    case EventEnterDutMode :
        if (lState != headsetTestMode)
		{
			stateManagerEnterTestModeState ( lApp ) ;
		}
        ConnectionEnterDutMode();               
        break;  
    case EventResetComplete:
        lApp->reset_complete = TRUE;
        MessageSendLater(&lApp->task, EventPowerOff, 0, 500);
        break;
    case EventError:        
        break;
    case EventChargeError:
        break;
    case EventEndOfCall :        
        break;    
    case EventDisableLEDS:
    {
        uint8 lbnormalanswer = 0;

        if(lApp->intercom_pairing_mode)
        {
            MessageSend(&lApp->task, EventPairingFail, 0);
        }

        lApp->normal_answer = lApp->normal_answer == FALSE ? TRUE : FALSE;

#ifdef NORMAL_ANSWER_MODE
        if(lApp->normal_answer)
        {
            TonesPlayTone(lApp, 2, TRUE);
            MessageSend(&lApp->task, EventFFWDPress, 0);
        }
        else
        {
            TonesPlayTone(lApp, 13, TRUE);
            MessageSend(&lApp->task, EventEnableLEDS, 0);
        }
#else
        if(lApp->normal_answer)
        {
            TonesPlayTone(lApp, 2, TRUE);
            MessageSend(&lApp->task, EventEnableLEDS, 0);
        }
        else
        {
            TonesPlayTone(lApp, 13, TRUE);
            MessageSend(&lApp->task, EventFFWDPress, 0);
        }
#endif

        lbnormalanswer = lApp->normal_answer;
        (void) PsStore(PSKEY_AUTO_ANSWER, &lbnormalanswer, sizeof(uint8));
        break;
    }
	case EventEstablishA2dp:
		if (lApp->aghfp_connect && lApp->audio_connect)
		{
			lIndicateEvent = FALSE ;
			break;
		}

		/* Don't indicate event if it's from power on */
		if (lApp->slcConnectFromPowerOn)
			lIndicateEvent = FALSE ;
		
		/* Only send a connect request if the headset isn't currently connecting A2DP and the headset
		   is on and not in pairing mode.
		*/
		if ( (lState >= headsetHfpConnectable) && !a2dpIsConnecting(lApp) )
		{
			if ( !stateManagerIsA2dpConnected() || lApp->slcConnectFromPowerOn )
			{
				/* Use reconnect sequence */ 
				a2dpReconnectProcedure(lApp);
			}
			else
			{
				/* Signalling is connected, but now check if the media is connected. */
				if (A2dpGetMediaSink(lApp->a2dp))
				{
					/* Signalling and media both connected */
            		/*do not perform the action*/
            		lIndicateEvent = FALSE ;
				}
				else
				{
					/* Media not connected so do it now */
					a2dpConnectRequest(lApp, TRUE);
				}
			}
		}
		else
        {
            /*do not perform the action*/
            lIndicateEvent = FALSE ;
        } 
		lApp->slcConnectFromPowerOn = FALSE;
        break;
	case EventA2dpConnected:
        /* For improvement A2DP Play and auto Pause problem */
        /*avrcpEventPlay(lApp);*/ /* TTA & A2DP conn */
        break;    
	case EventA2dpDisconnected:
        break; 
	case EventVolumeMax:
		break;
    case EventVolumeMin:
        break;
    case EventVolumeUp:
        /* Volume change's beep occur */
        break;
    case EventVolumeDown: /* R100 */
        /* Volume change's beep occur */
        break;
        
	case EventPlay:
		if ((lApp->aghfp_connect && lApp->audio_connect) || (lApp->slave_function && (int)HfpGetAudioSink(lApp->intercom_hsp) && lApp->dsp_process)) /* Don't play music during intercom - v110422 */
		{
			lIndicateEvent = FALSE ;
			break;
		}
#ifdef S100A /* Intercom Pairing mode out. v110422 */
        if(lApp->intercom_pairing_mode)
        {
            MessageSend(&lApp->task, EventPairingFail, 0);
        }
#endif
		/* Always indicate play event as will try to connect A2DP if not already connected */
		avrcpEventPlay(lApp);		
		break;
    case EventStop:
 		avrcpEventStop(lApp);		
		break;
#ifndef S100A /* GOLDWING_v101020 */
    case EventToggleMute:
#ifndef Z100_CLASS1
        lApp->extmic_evt = TRUE;
        micTrans(lApp, FALSE);
#endif
        break;
#endif
#ifdef READ_VOL
    case EventMuteReminder:
    {
        uint8 pin[2];

        if(lApp->aghfp_connect && lApp->audio_connect)
        {
            lIndicateEvent = FALSE;
            break;
        }

        if(lApp->battery_mv > 3800)
        {
            MessageSend(&lApp->task, EventFFWDPress, 0);
        }
        else
        {
            MessageSend(&lApp->task, EventEnableLEDS, 0);
        }

        if(lState == headsetHfpConnected && !lApp->slave_function)
        {
            pin[0] = (lApp->battery_mv / 1000) + 48;
            pin[1] = ((lApp->battery_mv - ((lApp->battery_mv / 1000) * 1000)) / 100) + 48;

            HfpDialNumber(lApp->hfp, 2, pin);
        }
		break;
    }
#endif
    case EventSkipForward: /* R100 */
#ifdef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        mSP430Trans(TRUE); /* v100225 */
#else
        if(!lApp->voice_manual_mode)
        {
            mSP430Trans(TRUE); /* v100225 */
        }
#endif
        break;
    case EventSkipBackward: /* R100 */
#ifdef VOICE_SEASON2 /* MSP430 ip_count thread value trans between OPEN and FULL Helmet */
        mSP430Trans(FALSE); /* v100225 */
#else
        if(!lApp->voice_manual_mode)
        {
            mSP430Trans(FALSE); /* v100225 */
        }
#endif
        break;
    case EventFFWDPress: /* BLUE LED */
        break;
    case EventFFWDRelease: /* R100 */
        break;
    case EventEnableLEDS: /* RED LED */
        break;
#ifdef AUTO_MIC_DETECT
    case EventTransferToggle: /* PIO11 Rising Edge Detect */
        MessageSend(&lApp->task, EventEnableLEDS, 0); /* Red LED */
        automicDetect();
        break;
    case EventToggleMute: /* PIO11 Falling Edge Detect */
        MessageSend(&lApp->task, EventFFWDPress, 0); /* Blue LED */
        automicDetect();
        break;
#endif
    default:
        EVENTS_DEBUG(("UNHANDLED EVENT: 0x%x\n",id));
        lIndicateEvent = FALSE ;
        break;
    }    

    if ( lIndicateEvent )
    {
#ifdef AUTO_MIC_DETECT
        LEDManagerIndicateEvent ( &lApp->theLEDTask , id ) ;
#else
        if(!(id == EventToggleMute && !lApp->extmic_mode)) LEDManagerIndicateEvent ( &lApp->theLEDTask , id ) ;
#endif
        TonesPlayEvent ( lApp , id ) ;		
    }   
}
