/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_init.c
@brief   Implementation of headset application initialisation functions. 
*/

#include "headset_configmanager.h"
#include "headset_config.h"
#include "headset_init.h"
#include "headset_LEDmanager.h"
#include "headset_powermanager.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_states.h"
#include "headset_volume.h"
#include "headset_auth.h"
#include "headset_debug.h"

#include <a2dp.h>
#include <avrcp.h>
#include <codec.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <memory.h>
#include <panic.h>
#include <ps.h>

#if 0 /* Jace_Test */
#ifdef R100 /* v091221 */
#include <csr_sbc_decoder_plugin.h>
#else
#include <csr_a2dp_decoder_common_plugin.h>
#endif
#else
#include <csr_a2dp_decoder_common_plugin.h>
#endif

#ifdef DEBUG_INIT
#define INIT_DEBUG(x) DEBUG(x)
#else
#define INIT_DEBUG(x) 
#endif

static const sep_config_type sbc_sep = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(sbc_caps_sink), sbc_caps_sink };

#if 0 /* TTA & A2DP conn */
#define NUM_OPTIONAL_CODECS	(NUM_SEPS-1)

typedef struct
{
	unsigned				seid:8;		/* The unique ID for the SEP. */
	unsigned				bit:8;		/* The bit position in PSKEY_USR_33 to enable the codec. */
	const sep_config_type	*config;	/* The SEP config data. These configs are defined above. */
	TaskData				*plugin;	/* The audio plugin to use. */
} optional_codec_type;
#endif


/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    headsetInitComplete
    
DESCRIPTION
    This function is called when all the libraries have been initialised,
    and the config has been loaded. The headset is now ready to be used.    
*/
static void headsetInitComplete( hsTaskData *pApp )
{
    /* Enter the limbo state as we may be ON due to a charger being plugged in */
    stateManagerEnterLimboState ( pApp );  

    /* Initialise the A2DP state */
    stateManagerEnterA2dpConnectableState ( pApp, TRUE );
    
    /* Set the class of device to indicate this is a headset */
    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_COD_RENDER | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);
    
    /* Configure security */
    ConnectionSmSetSecurityLevel(0, 1, ssp_secl4_l0, TRUE, FALSE, FALSE);
    if (pApp->forceMitmEnabled)
    {
        /* Require MITM on the MUX (incoming and outgoing) */
        ConnectionSmSetSecurityLevel(0,3,ssp_secl4_l3, TRUE, TRUE, FALSE);
    }
    
	PROFILE_TIME(("InitComplete"))
	PROFILE_MEMORY(("InitComplete"))
}


/**************************************************************************/
void InitHeadsetData ( hsTaskData *pApp ) 
{      
    uint8 lanormalanswer = 0;
#ifndef AUTO_MIC_DETECT
    uint8 laextmicmode = 0;
#endif
    uint8 lavoicemanualmode = 0; /* v100225 */

	memset(&pApp->a2dp, 0, sizeof(a2dpData));
	memset(&pApp->avrcp, 0, sizeof(avrcpData));

    pApp->hfp = NULL;
    pApp->hsp = NULL;
	pApp->hfp_hsp = NULL;
    pApp->intercom_hsp = NULL;
	pApp->a2dp = NULL;

    pApp->profile_connected = hfp_no_profile;
	pApp->a2dpSourceSuspended = FALSE;
	pApp->dsp_process = dsp_process_none;
	pApp->combined_link_loss = FALSE;
	pApp->connect_a2dp_when_no_call = FALSE;
	pApp->buttons_locked = FALSE;
	pApp->a2dpCodecsEnabled = 0;
	pApp->SCO_codec_selected = audio_codec_cvsd;
#ifdef READ_VOL
    pApp->battery_mv = 0;
#endif
#ifndef AUTO_MIC_DETECT
    pApp->extmic_evt = FALSE;
#endif
    pApp->reset_complete = FALSE;

    (void) PsRetrieve(PSKEY_AUTO_ANSWER, &lanormalanswer, sizeof(uint8));
    pApp->normal_answer = lanormalanswer;

#ifndef AUTO_MIC_DETECT
    (void) PsRetrieve(PSKEY_EXTMIC_MODE, &laextmicmode, sizeof(uint8));
    pApp->extmic_mode = laextmicmode;
#endif

    (void) PsRetrieve(PSKEY_VOICE_MANUAL_MODE, &lavoicemanualmode, sizeof(uint8)); /* v100225 */
    pApp->voice_manual_mode = lavoicemanualmode;

    AuthResetConfirmationFlags(pApp);
}


/**************************************************************************/
void InitUserFeatures ( hsTaskData *pApp ) 
{
    /* Initialise the Tones */
	uint16 size = TonesInit( pApp ) ;
    
    /* Initialise the Volume */    
    VolumeInit( pApp ) ;
    
    /* Initialise the LED Manager */
	LEDManagerInit( &pApp->theLEDTask ) ;
    
	/* Initialise the Button Manager */
    buttonManagerInit( &pApp->theButtonTask , &pApp->task);
    
    /* Initialise the Power Manager */
	pApp->power = powerManagerInit(pApp->gEventTones, size);
    
    /* Once system Managers are initialised, load up the configuration */
    configManagerInit( pApp );
       
    /* The headset initialisation is almost complete. */
    headsetInitComplete( pApp );
	
	/* Enable LEDs */
	LedManagerEnableLEDS( &pApp->theLEDTask ) ;
}


/*****************************************************************************/
void InitCodec(void)
{
    CodecInitCsrInternal (getAppTask()) ;
}


/*****************************************************************************/
void InitConnection(void)
{
    ConnectionInit (getAppTask()) ;
}


/*****************************************************************************/
void InitHfp(hsTaskData *pApp)
{
    hfp_init_params init;
    
    configManagerSetupSupportedFeatures(pApp);
    
    /* Initialise an HFP profile instance */
    init.supported_profile = hfp_handsfree_15_profile;
    init.supported_features = pApp->local_hfp_features;
    init.size_service_record = 0;
    init.service_record = 0;

    HfpInit(&pApp->task, &init);

    /* Initialise an HSP profile instance */
    init.supported_profile = hfp_headset_profile;
    init.supported_features = 0;
    init.size_service_record = 0;
    init.service_record = 0;
    
    HfpInit(&pApp->task, &init);
}


/*****************************************************************************/
void InitA2dp(void)
{
    hsTaskData* app = (hsTaskData*)getAppTask();
	
#if 0 /* TTA & A2DP conn */
	uint8 codec_enabled;
    sep_data_type seps[NUM_SEPS];
	uint16 number_of_seps = 0;
#else
    sep_data_type seps[1];
#endif
	
#if 0 /* TTA & A2DP conn */
	if (PsRetrieve(PSKEY_CODEC_ENABLED, &codec_enabled, sizeof(uint8)))
	{
		app->a2dpCodecsEnabled = codec_enabled;
	}
			        
	seps[number_of_seps].sep_config = &sbc_sep;
	seps[number_of_seps].in_use = FALSE;
	number_of_seps++;
#else
    seps[0].sep_config = &sbc_sep;
    seps[0].in_use = FALSE;
#endif
	
#if 0 /* TTA & A2DP conn */
	if (app->a2dpCodecsEnabled & (1<<AAC_CODEC_BIT))	
	{   
	}
	else
	{
		/* Initialise the A2DP library */
	    A2dpInit(&app->task, A2DP_INIT_ROLE_SINK, NULL, number_of_seps, seps);
    }
#else
    /* Initialise the A2DP library */
    A2dpInit(&app->task, A2DP_INIT_ROLE_SINK, NULL, 1, seps);
#endif

}


/*****************************************************************************/
void InitAvrcp(void)
{
    avrcp_init_params config;
    hsTaskData* app = (hsTaskData*)getAppTask();
    
#if 0 /* TTA & A2DP conn */
    config.device_type = avrcp_target_and_controller;
#else
    config.device_type = avrcp_controller;
#endif
   
    stateManagerSetAvrcpState(app, avrcpInitialising);
    
	/* Go ahead and Initialise the AVRCP library */
	AvrcpInit(&app->task, &config);
}


/*****************************************************************************/
uint16 InitSeidConnectPriority(hsTaskData *pApp, uint8 seid, uint8 *seid_list)
{
	uint16 size_seids = 1;

	if (seid != SBC_SEID)
	{
		seid_list[0] = seid;
		seid_list[1] = SBC_SEID;
		size_seids = 2;
		INIT_DEBUG(("INIT: Connect using SEID %d then SBC\n",seid));
	}
	else
	{
		seid_list[0] = SBC_SEID;
		INIT_DEBUG(("INIT: Connect using SBC\n" ));
	}
	
	return size_seids;
}


/*****************************************************************************/
Task InitA2dpPlugin(uint8 seid)
{
	if (seid == SBC_SEID)
		return (TaskData *)&csr_sbc_decoder_plugin;

	/* No plugin found so Panic */
	Panic();
	
	return 0;
}


/*****************************************************************************/
void InitIntercomData(hsTaskData *pApp)
{
    pApp->aghfp = NULL;
    pApp->audio_sink = NULL;
    pApp->ag_bd_addr.nap = 0;
    pApp->ag_bd_addr.uap = 0;
    pApp->ag_bd_addr.lap = 0;

    pApp->aghfp_initial = FALSE;
    pApp->slave_function = FALSE;
    pApp->aghfp_connect = FALSE;
    pApp->audio_connect = FALSE;
    pApp->aghfp_connecting = FALSE; /* R100 */
    pApp->aghfp_attempt_count = 0; /* R100 */
    pApp->is_slc_connect_ind = FALSE; /* R100 */
    pApp->intercom_button = FALSE; /* R100 */
    pApp->repeat_stop = FALSE;
    pApp->intercom_init = FALSE;
#ifdef AUTO_INT_AFTERCALL
    pApp->discon_intercom_incoming = FALSE; /* v100617 Auto connetion INTERCOM after end call */
#endif
#ifdef DUAL_STREAM
    pApp->a2dp_state_change = FALSE; /* SUSPEND */
#endif
#ifdef S100A
    pApp->intercom_pairing_mode = FALSE; /* v100622 Intercom Pairing mode */
#endif
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
    pApp->beep_audio_con = FALSE;
#endif
}

