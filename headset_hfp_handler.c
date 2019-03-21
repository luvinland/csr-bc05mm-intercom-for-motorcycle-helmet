/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_hfp_handler.c
@brief    Functions which handle the HFP library messages.
*/

#include "headset_a2dp_stream_control.h"
#include "headset_amp.h"
#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_hfp_handler.h"
#include "headset_hfp_slc.h"
#include "headset_init.h"
#include "headset_LEDmanager.h"
#include "headset_link_policy.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_volume.h"
#include "headset_csr_features.h"
#include "headset_scan.h"

#include <audio.h>
#include <bdaddr.h>
#include <hfp.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>

#ifdef DEBUG_HFP
#define HFP_DEBUG(x) DEBUG(x)
#else
#define HFP_DEBUG(x) 
#endif

#if 0 /* Jace_Test */
#include <csr_cvsd_no_dsp_plugin.h> /*no dsp connection*/
#include <csr_cvsd_8k_cvc_1mic_headset_plugin.h>
#else
#include <csr_cvc_common_plugin.h>
#include <csr_common_no_dsp_plugin.h>
#endif

#ifdef R100 /* v091221 */
#include <pio.h>
#ifdef Z100_CLASS1
#define AMP_GAIN_MASK ((uint16)1 << 13)
#else
#define AMP_GAIN_MASK ((uint16)1 << 0)
#endif
#endif


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/
static bool audioConnectSco(hsTaskData *pApp, HFP *hfp, AUDIO_SINK_T sink_type, uint32 bandwidth);

/****************************************************************************
  HFP MESSAGE HANDLING FUNCTIONS
*/

/****************************************************************************/
void hfpHandlerInitCfm( hsTaskData * pApp , const HFP_INIT_CFM_T *cfm )
{
    /* Make sure the profile instance initialisation succeeded. */
    if (cfm->status == hfp_init_success)
    {
        /* Check for an hfp instance, that's registered first */
        if (!pApp->hfp)
        {
            /* This must be the hfp instance */ 
            pApp->hfp = cfm->hfp;
        }
        else
        {
            /* Its not HFP so must be HSP */
            pApp->hsp = cfm->hfp;
            /* HFP/HSP Library initialisation was a success, initailise the A2DP library. */
            InitA2dp();
        }
    }
    else
        /* If the profile initialisation has failed then things are bad so panic. */
        Panic();
}


/****************************************************************************/
void hfpHandlerConnectInd( hsTaskData * pApp , const HFP_SLC_CONNECT_IND_T *ind )
{
    pApp->repeat_stop = TRUE;


#ifndef BNFON
    /* We support more than one HFP so check which one this request is for */   
    if (stateManagerGetHfpState() != headsetPoweringOn)
    {
        /* See whether we are connecting as HSP or HFP */
        if (ind->hfp == pApp->hfp)
        {
            if(ind->addr.nap == 0x24 && ind->addr.uap == 0xbc) /* v100810 For HSB-Series Compatibility */
            {
                HfpSlcConnectResponse(ind->hfp, 0, &ind->addr, 0);
                (void) ConnectionSmDeleteAuthDevice(&ind->addr); /* v100817 For HSB-Series Compatibility */
                (void)PsStore ( PSKEY_LAST_PAIRED_DEVICE , 0 , 0 ) ; /* v100817 For HSB-Series Compatibility */
            }
            else
            {
                HfpSlcConnectResponse(ind->hfp, 1, &ind->addr, 0);
                pApp->profile_connected = hfp_handsfree_profile;
            }
        }
        else if (ind->hfp == pApp->hsp)
        {
#ifdef SINPUNG
            if((ind->addr.nap == 0x24 && ind->addr.uap == 0xbc) || (ind->addr.nap == 0x15 && ind->addr.uap == 0x45)) /* R100 */
#else
            if(ind->addr.nap == 0x24 && ind->addr.uap == 0xbc) /* R100 */
#endif
            {
                HfpSlcConnectResponse(ind->hfp, 1, &ind->addr, 0);
                MessageSend(&pApp->task, EventSkipForward, 0);
            }
            else
            {
                HfpSlcConnectResponse(ind->hfp, 0, &ind->addr, 0);
            }
        }
        else
            /* Something is wrong we should be either hfp or hsp */
            Panic();
    }
    else
    {
        /* Reject the connect attempt we're already connected */
        HfpSlcConnectResponse(ind->hfp, 0, &ind->addr, 0);
    }
#else
    /* We support more than one HFP so check which one this request is for */   
    if (stateManagerGetHfpState() != headsetPoweringOn)
    {
        /* See whether we are connecting as HSP or HFP */
        if (ind->hfp == pApp->hfp)
        {
            HfpSlcConnectResponse(ind->hfp, 1, &ind->addr, 0);
            MessageSend(&pApp->task, EventSkipForward, 0);
            pApp->profile_connected = hfp_handsfree_profile;
        }
        else if (ind->hfp == pApp->hsp)
        {
            HfpSlcConnectResponse(ind->hfp, 1, &ind->addr, 0);
            MessageSend(&pApp->task, EventSkipForward, 0);
        }
        else
            /* Something is wrong we should be either hfp or hsp */
            Panic();
    }
    else
    {
        /* Reject the connect attempt we're already connected */
        HfpSlcConnectResponse(ind->hfp, 0, &ind->addr, 0);
    }
#endif
}


/****************************************************************************/
void hfpHandlerConnectCfm( hsTaskData * pApp , const HFP_SLC_CONNECT_CFM_T *cfm )
{
    if (stateManagerGetHfpState() == headsetPoweringOn)
    {
        if ( cfm->status == hfp_connect_success )
        {
            /* A connection has been made and we are now logically off */
            hfpSlcDisconnect( pApp ); 
        }
		pApp->slcConnecting = FALSE;
		pApp->slcConnectFromPowerOn = FALSE;
        return;
    }
    
    if (cfm->status == hfp_connect_success)
    {      
        hfpSlcConnectSuccess(pApp, cfm->hfp, cfm->sink);

		/* Update Link Policy as HFP has connected. */
		linkPolicyHfpStateChange(TRUE, FALSE);
		
		/* Enable CSR 2 CSR Extensions */
		csr2csrEnable(cfm->hfp);
    }
    else if (cfm->status == hfp_connect_sdp_fail)
    {
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
        pApp->beep_audio_con = FALSE; 
#endif

        if(pApp->repeat_stop) /* Slave Mode memory */
        {
            pApp->repeat_stop = FALSE;
            pApp->intercom_button = FALSE; /* R100 */
            hfpSlcConnectFail(pApp);

            MessageSend(&pApp->task, EventSkipForward, 0);
            MessageSendLater(&pApp->task, EventSkipBackward, 0, D_SEC(1));
            return;
        }

        if ( !stateManagerIsHfpConnected() )  /*only continue if not already connected*/
        {
            if (cfm->hfp == pApp->hfp)
            {
                hfpSlcConnectFail(pApp);
            }
            else
            {
                Panic();
            }
        } 
		else
		{
			pApp->slcConnecting = FALSE;
			pApp->slcConnectFromPowerOn = FALSE;
		}
    }
    else
    {
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
        pApp->beep_audio_con = FALSE; 
#endif

        if(pApp->repeat_stop) /* Slave Mode memory */ /* After Master Intercom Power OFF */
        {
            pApp->repeat_stop = FALSE;
            pApp->intercom_button = FALSE; /* R100 */
            hfpSlcConnectFail(pApp);
        
            MessageSend(&pApp->task, EventSkipForward, 0);
            MessageSendLater(&pApp->task, EventSkipBackward, 0, D_SEC(1));
            return;
        }

        if ( !stateManagerIsHfpConnected() )  /*only continue if not already connected*/
        {    /* Failed to connect */    
            hfpSlcConnectFail(pApp);     
        }
		else
		{
			pApp->slcConnecting = FALSE;
			pApp->slcConnectFromPowerOn = FALSE;
		}
    }       
}


/*****************************************************************************/
void hfpHandlerDisconnectInd(hsTaskData * pApp, const HFP_SLC_DISCONNECT_IND_T *ind)
{	
	/* Store current volume levels to ensure correct values on next SLC connect */
	VolumeStoreLevels(pApp);
	
	/* Check if this was the result of an abnormal link loss */
    if (ind->status == hfp_disconnect_link_loss ) 
    {
		bdaddr ag_addr, a2dp_addr;
		
        HFP_DEBUG(("HFP: Link Loss Detect\n")) ;
               
        MessageSend( &pApp->task , EventLinkLoss , 0 ) ;
		
		if (!stateManagerIsA2dpStreaming())
		{
            if(!pApp->slave_function) /* Slave Mode memory */
            {
    			/* If A2DP disconnected already then reconnect it once HFP established */
    			if (pApp->combined_link_loss)
    				pApp->slcConnectFromPowerOn = TRUE;

    			/* A Link Loss has occured - attempt reconnect */
            	hfpSlcConnectRequest( pApp , hfp_handsfree_profile ) ;
            }
		}
		else
		{
			/* If link loss has occured on a combined device then it's probable that the A2DP will suffer link
			   loss shortly, so wait for this to happen.
			  */
			if (A2dpGetSignallingSink(pApp->a2dp) && PsRetrieve(PSKEY_LAST_USED_AG, &ag_addr, sizeof(bdaddr)) && PsRetrieve(PSKEY_LAST_USED_AV_SOURCE, &a2dp_addr, sizeof(bdaddr)))
			{
				if (BdaddrIsSame(&ag_addr, &a2dp_addr))
					pApp->combined_link_loss = TRUE;
			}
				
		}
    }
    else
    {
		pApp->combined_link_loss = FALSE;
    }
	
    /* Update the app state if we are connected */
    if ( stateManagerIsHfpConnected() )
    {
        if(!(HfpGetSlcSink(pApp->hfp_hsp) || HfpGetSlcSink(pApp->intercom_hsp)))
            stateManagerEnterHfpConnectableState( pApp, FALSE ) ;
        else
            headsetEnableConnectable(pApp);
    }
    
    if(ind->hfp == pApp->hfp_hsp)
    {
        /* Connection disconnected */
        pApp->profile_connected = hfp_no_profile;
        pApp->hfp_hsp = NULL;
        
        /* Reset in-band ring tone support flag */
        pApp->InBandRingEnabled = FALSE;
    }
    else if(ind->hfp == pApp->hsp)
    {
        pApp->intercom_hsp = NULL;
    }
    else
    {
        Panic();
    }

    MessageSend(&pApp->task , EventSLCDisconnected , 0) ;

    /* Update Link Policy as HFP has disconnected. */
    linkPolicyHfpStateChange(FALSE, FALSE);

    PROFILE_MEMORY(("HFPDisco"))

}


/*****************************************************************************/
void hfpHandlerInbandRingInd( hsTaskData * pApp, const HFP_IN_BAND_RING_IND_T * ind )
{    
    pApp->InBandRingEnabled = ind->ring_enabled;        
}


/*****************************************************************************/
void hfpHandlerCallInd ( hsTaskData *pApp,  const HFP_CALL_IND_T * pInd ) 
{
    switch (pInd->call)
    {
    case 0:
		if ( stateManagerGetHfpState() == headsetActiveCall )
        {
#ifdef AUTO_INT_AFTERCALL
            if(pApp->discon_intercom_incoming) /* v100617 Auto connetion INTERCOM after end call */
            {
                pApp->discon_intercom_incoming = FALSE;
                MessageSendLater(&pApp->task, EventRWDPress, 0, D_SEC(8));
            }
#endif
            stateManagerEnterHfpConnectedState ( pApp ) ;
        }    
      	break;

    case 1:
        stateManagerEnterActiveCallState ( pApp ) ;  
		ToneTerminate ( pApp ) ;
        break;

    default:
        break;
    }
}


/*****************************************************************************/
void hfpHandlerCallSetupInd ( hsTaskData *pApp,  const HFP_CALL_SETUP_IND_T * pInd ) 
{			
    switch (pInd->call_setup)
    {
    case (hfp_no_call_setup): /*!< No call currently being established.*/

     	/* Spec says that this is a clear indication that an incoming call has been interrupted
       	   and we can assume that the call is not going to continue....*/          
        	   
        /* However, we have come across phones that send this before a callind [1]
           on accepting a call - -interop issue*/
               
        if ( stateManagerIsHfpConnected() )
        {
#ifdef AUTO_INT_AFTERCALL
            if(pApp->discon_intercom_incoming) /* v100527 Auto connetion INTERCOM after end call */
            {
                pApp->discon_intercom_incoming = FALSE;
                MessageSendLater(&pApp->task, EventRWDPress, 0, D_SEC(8)); /* SUSPEND */
            }
#endif
            stateManagerEnterHfpConnectedState ( pApp ) ;
			ToneTerminate ( pApp ) ;
        }
               
        break;
    case (hfp_incoming_call_setup): /*!< HFP device currently ringing.*/

        if(pApp->aghfp_connect && pApp->audio_connect) /* SUSPEND */
        {
#ifdef AUTO_INT_AFTERCALL
            pApp->discon_intercom_incoming = TRUE; /* v100617 Auto connetion INTERCOM after end call */
#endif
            AghfpAudioDisconnect(pApp->aghfp);
        }
        else if(pApp->slave_function && (int)HfpGetAudioSink(pApp->intercom_hsp) && pApp->dsp_process) /* v100617 */
        {
#ifdef AUTO_INT_AFTERCALL
            pApp->discon_intercom_incoming = TRUE; /* v100617 Auto connetion INTERCOM after end call */
#endif
            HfpAudioDisconnect(pApp->intercom_hsp);
        }

/*        if(!pApp->normal_answer) MessageSendLater(&pApp->task, EventAnswer, 0, D_SEC(6));*/ /* Auto Answer Update */ /* SUSPEND */ /* v110128 Move hfpHandlerRingInd() */
        stateManagerEnterIncomingCallEstablishState ( pApp ) ;
        break;
    case (hfp_outgoing_call_setup): /*!< Call currently being dialed.*/
#ifdef S100A /* Discon INT when manual outgoing call v110422 */
        if(pApp->aghfp_connect && pApp->audio_connect) /* SUSPEND */
        {
#ifdef AUTO_INT_AFTERCALL
            pApp->discon_intercom_incoming = TRUE; /* v100617 Auto connetion INTERCOM after end call */
#endif
            AghfpAudioDisconnect(pApp->aghfp);
        }
        else if(pApp->slave_function && (int)HfpGetAudioSink(pApp->intercom_hsp) && pApp->dsp_process) /* v100617 */
        {
#ifdef AUTO_INT_AFTERCALL
            pApp->discon_intercom_incoming = TRUE; /* v100617 Auto connetion INTERCOM after end call */
#endif
            HfpAudioDisconnect(pApp->intercom_hsp);
        }

        stateManagerEnterOutgoingCallEstablishState ( pApp ) ;
        break;
#endif
    case (hfp_outgoing_call_alerting_setup):/*!< Remote end currently ringing.*/
        stateManagerEnterOutgoingCallEstablishState ( pApp ) ;
        break;
    default:
        break;
        
    }
}


/*****************************************************************************/
void hfpHandlerRingInd ( hsTaskData *pApp )
{
	/* Disconnect A2DP audio if it is streaming from a standalone A2DP source */
	if (!IsA2dpSourceAnAg(pApp) && !pApp->audio_connect) /* SUSPEND */
    {
		streamControlCeaseA2dpStreaming(pApp, TRUE);
    }

    if ( ! pApp->InBandRingEnabled )
    {
    	HFP_DEBUG(("HFP_DEBUG: OutBandRing\n")) ;
        pApp->RingTone = pApp->normal_answer ? 12 : 13;

        /* Play ring tone from configuration */
	    TonesPlayTone ( pApp , pApp->RingTone , FALSE);    
    }

    /* v110128 Move hfpHandlerRingInd() for Lockup - incoming call answer very quickly, iPhone4 no problem */
    if(!pApp->normal_answer) MessageSendLater(&pApp->task, EventAnswer, 0, D_SEC(6)); /* Auto Answer Update */ /* SUSPEND */
}


/*****************************************************************************/
void hfpHandlerLastNoRedialCfm( hsTaskData *pApp, const HFP_LAST_NUMBER_REDIAL_CFM_T *cfm )
{
    if (cfm->status)
    {
		MessageSend ( &pApp->task , EventError , 0 );
    }
}


/*****************************************************************************/
void hfpHandlerEncryptionChangeInd( hsTaskData *pApp, const HFP_ENCRYPTION_CHANGE_IND_T *ind )
{
    
}


/*****************************************************************************/
void hfpHandlerSpeakerVolumeInd( hsTaskData *pApp, const HFP_SPEAKER_VOLUME_IND_T *ind )
{
	HFP_DEBUG(("HFP: hfpHandlerSpeakerVolumeInd [%d]\n", ind->volume_gain)) ;
	pApp->gHfpVolumeLevel = ind->volume_gain;
			
	if (stateManagerIsA2dpStreaming())
	    return;    
	
    VolumeSetHeadsetVolume(pApp, pApp->gHfpVolumeLevel, FALSE);
}


/*****************************************************************************/
void hfpHandlerAudioConnectInd( hsTaskData *pApp, const HFP_AUDIO_CONNECT_IND_T *ind )
{
    HFP *active_hfp = 0;

    pApp->repeat_stop = TRUE;

    /* Accept audio if its for a task we own */
    if ((ind->hfp == pApp->hfp) || (ind->hfp == pApp->hsp))
        active_hfp = ind->hfp;    

    if (active_hfp)
    {
        /* Accept the audio connection */		
        HfpAudioConnectResponse(active_hfp, 1, (sync_all_sco), 0, ind->bd_addr);
    }
    else
	{
        HfpAudioConnectResponse(active_hfp, 0, sync_hv1, 0, ind->bd_addr);	
	}
}


/*****************************************************************************/
void hfpHandlerAudioConnectCfm( hsTaskData *pApp, const HFP_AUDIO_CONNECT_CFM_T *cfm )
{
    pApp->repeat_stop = FALSE;

    if ( cfm->status == hfp_success)
    {
	    AUDIO_SINK_T sink_type;

#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
        if(pApp->beep_audio_con)
        {
            pApp->beep_audio_con = FALSE;
        }
        else
        {
            TonesPlayTone(pApp, 7, TRUE);
        }
#else
        TonesPlayTone(pApp, 7, TRUE);
#endif

#if 0
        /* Refresh the link encryption key */
        ConnectionSmEncryptionKeyRefreshSink(HfpGetSlcSink(pApp->hfp_hsp));
#else
        if(cfm->hfp == pApp->hfp_hsp)
        {
            /* Refresh the link encryption key */
            ConnectionSmEncryptionKeyRefreshSink(HfpGetSlcSink(pApp->hfp_hsp));
        }
        else if(cfm->hfp == pApp->intercom_hsp)
        {
            /* Refresh the link encryption key */
            ConnectionSmEncryptionKeyRefreshSink(HfpGetSlcSink(pApp->intercom_hsp));
        }
        else
        {
            Panic();
        }
#endif

        switch (cfm->link_type)
        {
			case sync_link_unknown:  /* Assume SCO if we don't know */
			case sync_link_sco:
				sink_type = AUDIO_SINK_SCO;
				break;
			case sync_link_esco:
				sink_type = AUDIO_SINK_ESCO;
				break;
			default:
				sink_type = AUDIO_SINK_INVALID;
				break;
        }
        
        audioConnectSco(pApp, cfm->hfp, sink_type, cfm->tx_bandwidth) ;
        
       /* Send an event to indicate that a SCO has been opened. This indicates
          that an audio connection has been successfully created to the AG. */
    	MessageSend ( &pApp->task , EventSCOLinkOpen , 0 ) ;

		/* Update Link Policy as SCO has connected. */
		linkPolicyHfpStateChange(TRUE, TRUE);
    }
    else
    {
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
        pApp->beep_audio_con = FALSE; 
#endif

        if(pApp->slave_function)
        {
            TonesPlayTone(pApp, 8, TRUE);
            pApp->intercom_button = FALSE; /* R100 */

            MessageSend(&pApp->task, EventSkipForward, 0);
            MessageSendLater(&pApp->task, EventSkipBackward, 0, D_SEC(1));
        }
    }
}


/*****************************************************************************/
void hfpHandlerAudioDisconnectInd( hsTaskData *pApp, const HFP_AUDIO_DISCONNECT_IND_T *ind )
{
    pApp->repeat_stop = FALSE;

    AudioDisconnect() ;
	
	/* Turn the audio amp off after a delay */
	AmpOffLater(pApp);
	
	pApp->dsp_process = dsp_process_none;

	/* Try to resume A2DP streaming if not outgoing or incoming call */
	if ((stateManagerGetHfpState() != headsetIncomingCallEstablish) && (stateManagerGetHfpState() != headsetOutgoingCallEstablish))     
    {
		if ((stateManagerGetHfpState() == headsetActiveCall) && IsA2dpSourceAnAg(pApp))
		{
			/* Do nothing with A2DP_AG as the music can't start while call active */
		}
		else
		{
            streamControlResumeA2dpStreaming(pApp, D_SEC(3)); /* SUSPEND */
		}
    }
    
    MessageSend ( &pApp->task , EventSCOLinkClose , 0 ) ;
    
    /* Set the mic bias */
    LEDManagerSetMicBias ( pApp , FALSE ) ;
    
    /* If we are muted - then un mute at disconnection */
    if (pApp->gMuted )
    {
         MessageSend(&pApp->task , EventMuteOff , 0) ;   
    }    
	
	/* Update Link Policy as SCO has disconnected. */
	linkPolicyHfpStateChange(TRUE, TRUE);
}


/****************************************************************************
NAME    
    audioConnectSco
    
DESCRIPTION
	This function connects the synchronous connection to the CVC EC/NR Algorithm running
    in the Kalimba DSP. 

*/	  
static bool audioConnectSco(hsTaskData *pApp, HFP *hfp, AUDIO_SINK_T sink_type, uint32 bandwidth)
{
    bool lResult = FALSE ;
	TaskData * plugin = NULL ;
	audio_codec_type codec = audio_codec_none;
   	
    /* The mode to connect - connected as default */
    AUDIO_MODE_T lMode = AUDIO_MODE_CONNECTED ;

	/* Disconnect A2DP audio if it was active */
#ifdef DUAL_STREAM
    if(pApp->slave_function) streamControlCeaseA2dpStreaming(pApp, FALSE);
    else streamControlCeaseA2dpStreaming(pApp, TRUE);
#else
    streamControlCeaseA2dpStreaming(pApp, TRUE);
#endif
    
	/* Turn the audio amp on */
	AmpOn(pApp);
    
    /* Mute control */
    if (pApp->gMuted )
    {
        lMode = AUDIO_MODE_MUTE_MIC ;
    }    	
    
    switch (pApp->SCO_codec_selected)
    {
		case audio_codec_cvsd:
			if (!pApp->cvcEnabled)
				plugin = (TaskData *)&csr_cvsd_no_dsp_plugin;
			else
				plugin = (TaskData *)&csr_cvsd_cvc_1mic_headset_plugin; /* Jace_Test */
			break;
		case audio_codec_auristream_2_bit:
			codec = pApp->SCO_codec_selected;
			break;
		case audio_codec_auristream_4_bit:
			codec = pApp->SCO_codec_selected;
			break;
		default:
			{
				HFP_DEBUG(("HFP: Unknown SCO Codec [%d]\n", pApp->SCO_codec_selected));
				Panic();
				break;
			}
    }
	
	HFP_DEBUG(("HFP: Route SCO mode=%d mic_mute=%d volindex=%d volgain=%d\n",lMode,pApp->gMuted,pApp->gHfpVolumeLevel,VolumeRetrieveGain(pApp->gHfpVolumeLevel, FALSE)));

#ifdef R100 /* v091221 */
    if(pApp->gHfpVolumeLevel > 4)
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, AMP_GAIN_MASK);
    }
    else
    {
        PioSetDir(AMP_GAIN_MASK, AMP_GAIN_MASK);
        PioSet(AMP_GAIN_MASK, 0);
    }
#endif

#if 0
    /* v091111 Release */
    lResult = AudioConnect(plugin,
                             HfpGetAudioSink(pApp->hfp_hsp),
                             sink_type,
                             pApp->theCodecTask,
                             VolumeRetrieveGain(pApp->gHfpVolumeLevel, FALSE),
                             0,
                             TRUE,
                             lMode,
                             (void*)codec);
#else
    if(hfp == pApp->hfp_hsp)
    {
        /* v091111 Release */
        lResult = AudioConnect(plugin,
                                 HfpGetAudioSink(pApp->hfp_hsp),
                                 sink_type,
                                 pApp->theCodecTask,
                                 VolumeRetrieveGain(pApp->gHfpVolumeLevel, FALSE),
                                 8000, /* Jace_Test */
                                 TRUE,
                                 lMode,
                                 NULL, /* Jace_Test */
                                 &pApp->task); /* Jace_Test */
    }
    else if(hfp == pApp->intercom_hsp)
    {
        /* v091111 Release */
        lResult = AudioConnect(plugin,
                                 HfpGetAudioSink(pApp->intercom_hsp),
                                 sink_type,
                                 pApp->theCodecTask,
                                 VolumeRetrieveGain(pApp->gHfpVolumeLevel, FALSE),
                                 8000, /* Jace_Test */
                                 TRUE,
                                 lMode,
                                 NULL, /* Jace_Test */
                                 &pApp->task); /* Jace_Test */
    }
#endif

	pApp->dsp_process = dsp_process_sco;
	
    /* Set the mic bias */
    LEDManagerSetMicBias ( pApp , TRUE ) ;    

	return (lResult=TRUE);
}
