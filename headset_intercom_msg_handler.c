/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file     headset_aghfp_msg_handler.c
@brief    Handle aghfp library messages arriving at the app.
*/

#include <aghfp.h>
#include <panic.h>
#include <ps.h>
#include <codec.h>
#include <audio.h>
#include <vm.h>
#if 0 /* Jace_Test */
#include <csr_cvsd_no_dsp_plugin.h>
#include <csr_cvsd_8k_cvc_1mic_headset_plugin.h>
#else
#include <csr_cvc_common_plugin.h>
#include <csr_common_no_dsp_plugin.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "headset_debug.h"
#include "headset_private.h"
#include "headset_intercom_msg_handler.h"
#include "headset_volume.h"
#include "headset_a2dp_stream_control.h"
#include "headset_amp.h"
#include "headset_LEDmanager.h"
#include "headset_init.h"
#include "headset_tones.h"
#include "headset_statemanager.h"
#include "headset_hfp_slc.h"
#include "headset_a2dp_connection.h"
/* For AG inquire */
#include "headset_intercom_inquire.h"
#include "headset_scan.h" /* v100817 Disable Connectable Problem (AGHFP, A2DP, HFP) */

#ifdef DEBUG_INTERCOM_MSG
#define INTERCOM_MSG_DEBUG(x) DEBUG(x)
#else
#define INTERCOM_MSG_DEBUG(x) 
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
    ENUM DEFINITIONS
*/


/****************************************************************************
    MESSAGE DEFINITIONS
*/

/****************************************************************************
    LOCAL FUNCTIONS
*/

/****************************************************************************/

/* For AG inquire */
#define PSKEY_TARGET_BDADDR 12

/* Manipulate far_addr, keeping it in step with the copy in Persistent Store */

static void write_far_addr(bdaddr* far_addr)
{
    (void)PanicZero(PsStore(PSKEY_TARGET_BDADDR, far_addr, sizeof(bdaddr)));
    INTERCOM_MSG_DEBUG(("Write far addr: %ld %d %d\n", far_addr->lap, far_addr->uap, far_addr->nap));
}

static void clear_far_addr(bdaddr* far_addr)
{
    memset(far_addr, 0, sizeof(bdaddr));
    (void)PsStore(PSKEY_TARGET_BDADDR, 0, 0);
    INTERCOM_MSG_DEBUG(("Clear far addr:\n"));
}

static void read_far_addr(bdaddr* far_addr)
{
    if(!PsRetrieve(PSKEY_TARGET_BDADDR, far_addr, sizeof(bdaddr)))
        clear_far_addr(far_addr);
    INTERCOM_MSG_DEBUG(("Read far addr: %ld %d %d\n", far_addr->lap, far_addr->uap, far_addr->nap));
}

static uint16 know_far_addr(bdaddr far_addr)
{
    return far_addr.lap || far_addr.nap || far_addr.uap;
}


void handleINTERCOMMessage(Task task, MessageId id, Message message)
{
    hsTaskData* app = (hsTaskData*)getAppTask();

    switch(id)
    {
    case AGHFP_INIT_CFM:
    {
        AGHFP_INIT_CFM_T* msg = (AGHFP_INIT_CFM_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_INIT_CFM\n"));

        read_far_addr(&app->ag_bd_addr);

        if(msg->status == success)
        {
            INTERCOM_MSG_DEBUG(("success\n"));
            app->aghfp = msg->aghfp;
            app->aghfp_initial = TRUE;

#ifdef S100A
            if(app->init_aghfp_power_on)
            {
                app->repeat_stop = FALSE;
                app->intercom_button = FALSE;
                app->init_aghfp_power_on = FALSE;
                break;
            }
#endif

            if(know_far_addr(app->ag_bd_addr))
            {
                /*  We paired with a device, try and connect to it */
                app->intercom_init = TRUE;

                if(app->intercom_button) MessageSend(&app->task, EventSkipForward, 0);

                AghfpSlcConnect(app->aghfp, &app->ag_bd_addr);
            }
            else if(app->intercom_pairing_mode)
            {
                MessageSend(&app->task, EventSkipForward, 0);
                MessageSendLater(&app->task, AGHFP_INQUIRE_START, 0, 10); /* v100617 Pairing problem after slave's S/W Init. */
            }
            else
            {
                /* Insert code for Intercom by Jace */
                app->repeat_stop = FALSE;
                TonesPlayTone(app, 8, TRUE);

                MessageSend(&app->task, EventSkipForward, 0);
                MessageSendLater(&app->task, EventSkipBackward, 0, D_SEC(1));
            }
        }
        else
        {
            INTERCOM_MSG_DEBUG(("failure\n"));
            InitIntercomData(app);
        }
        break;
    }

    case AGHFP_SLC_CONNECT_CFM:
    {
        AGHFP_SLC_CONNECT_CFM_T* msg = (AGHFP_SLC_CONNECT_CFM_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_SLC_CONNECT_CFM\n"));

        MessageCancelAll(&app->task, AGHFP_CONNECT_FAIL_TIMEOUT);
        app->aghfp_connecting = FALSE; /* R100 */

        if(msg->status == aghfp_connect_success)
        {
            INTERCOM_MSG_DEBUG(("success\n"));
#ifdef S100A
            app->intercom_init = FALSE; /* v100817 v100617 remodify */
#endif
            MessageSend(&app->task, EventEndOfCall, 0); /* 4s LED ON concept */
            app->aghfp_connect = TRUE;
            app->aghfp_attempt_count = 0; /* R100 */

            write_far_addr(&app->ag_bd_addr); /* For AG inquire */
#ifdef R100
            (void)PsStore ( 7 , 0 , 0 ) ;
#endif

            TonesPlayTone(app, 7, TRUE);
#if 0
            if (lState == headsetConnDiscoverable) MessageSend(&app->task, EventPairingFail, 0);
#else
            if (app->intercom_pairing_mode)
            {
                MessageCancelAll(&app->task, EventPairingFail);
                app->intercom_pairing_mode = FALSE;
            }
#endif
#ifdef S100A /* v101201 Power On connect to intercom addr */
            if (!stateManagerIsHfpConnected())
            {
                stateManagerEnterHfpConnectedState( app ); 
            }
#endif

            MessageSendLater(&app->task, AGHFP_STABILIZE_AUDIO_CONNECT, 0, D_SEC(10));
        }
        else
        {
            INTERCOM_MSG_DEBUG(("failure : %d\n", msg->status));

            app->repeat_stop = FALSE;
            app->intercom_button = FALSE; /* R100 */
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
            app->beep_audio_con = FALSE; 
#endif

            MessageSend(&app->task, EventSkipForward, 0);
            MessageSendLater(&app->task, EventSkipBackward, 0, D_SEC(1));

            if(app->aghfp_attempt_count == 0 || app->aghfp_attempt_count == 30) /* Link loss retry error tone remove */
                TonesPlayTone(app, 8, TRUE);

            if(app->intercom_init)
            {
                /* Restart enquiry process */
                if(app->intercom_pairing_mode)
                {
                    MessageSendLater(&app->task, AGHFP_INQUIRE_START, 0, 500); /* v100617 Pairing problem after slave's S/W Init. */
                }
            }
        }
        break;
    }

    case AGHFP_INQUIRE_START: /* v100617 Pairing problem after slave's S/W Init. */
        if(app->intercom_init)
        {
#ifndef S100A /* v101201 Repairing fail for same addr */
            ConnectionSmAuthenticate(&app->task, &app->ag_bd_addr, 1);
#else
            (void) ConnectionSmDeleteAuthDevice(&app->ag_bd_addr);
#endif
            memset(&app->ag_bd_addr, 0, sizeof(bdaddr));
            app->intercom_init = FALSE; /* v100817 v100617 remodify */
        }

        intercomInquire(app);
        break;

    case AGHFP_AUDIO_CONNECT_CFM:
    {
        AGHFP_AUDIO_CONNECT_CFM_T* msg = (AGHFP_AUDIO_CONNECT_CFM_T*)message;
        TaskData * plugin = NULL;
        INTERCOM_MSG_DEBUG(("AGHFP_AUDIO_CONNECT_CFM\n"));

        MessageCancelAll(&app->task, AGHFP_CONNECT_FAIL_TIMEOUT);

        app->repeat_stop = FALSE;
        app->is_slc_connect_ind = FALSE; /* R100 */

        if(msg->status == aghfp_audio_connect_success)
        {
            INTERCOM_MSG_DEBUG(("success [%x] volumeLevel [%d] link [%d]\n", (int)msg->audio_sink, app->gHfpVolumeLevel, msg->link_type));
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
            if(app->beep_audio_con)
            {
                app->beep_audio_con = FALSE;
            }
            else
            {
                TonesPlayTone(app, 7, TRUE);
            }
#else
            TonesPlayTone(app, 7, TRUE);
#endif

            app->audio_connect = TRUE;
            app->audio_sink = msg->audio_sink;

            /* Disconnect A2DP audio if it was active */
            if((stateManagerGetA2dpState() == headsetA2dpStreaming) || (stateManagerGetA2dpState() == headsetA2dpPaused))
            {
#ifdef DUAL_STREAM
                app->a2dp_state_change = TRUE; /* SUSPEND */

                streamControlCeaseA2dpStreaming(app, FALSE);
                stateManagerEnterA2dpConnectedState(app); /* SUSPEND */
#else
                streamControlCeaseA2dpStreaming(app, TRUE);
#endif
            }

            if (!app->cvcEnabled) plugin = (TaskData *)&csr_cvsd_no_dsp_plugin;
            else plugin = (TaskData *)&csr_cvsd_cvc_1mic_headset_plugin; /* Jace_Test */

            /* Turn the audio amp on */
            AmpOn(app);

#ifdef R100 /* v091221 */
            if(app->gHfpVolumeLevel > 4)
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

            /* v091111 Release */
            AudioConnect(plugin,
                           app->audio_sink,
                           msg->link_type,
                           app->theCodecTask,
                           VolumeRetrieveGain(app->gHfpVolumeLevel, FALSE),
                           8000, /* Jace_Test */
                           TRUE,
                           AUDIO_MODE_CONNECTED,
                           NULL,
                           &app->task); /* Jace_Test */

            app->dsp_process = dsp_process_sco;

            /* Set the mic bias */
            LEDManagerSetMicBias(app, TRUE);
            LEDManagerIndicateState(&app->theLEDTask, headsetActiveCall, stateManagerGetA2dpState());
        }
        else
        {
            INTERCOM_MSG_DEBUG(("failure\n"));
            TonesPlayTone(app, 8, TRUE);
            app->intercom_button = FALSE; /* R100 */
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
            app->beep_audio_con = FALSE; 
#endif

            MessageSend(&app->task, EventSkipForward, 0);
            MessageSendLater(&app->task, EventSkipBackward, 0, D_SEC(1));
        }
        break;
    }

    case AGHFP_SLC_DISCONNECT_IND:
    {
        AGHFP_SLC_DISCONNECT_IND_T* msg = (AGHFP_SLC_DISCONNECT_IND_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_SLC_DISCONNECT_IND\n"));

        app->aghfp_connect = FALSE;
        app->audio_connect = FALSE;

        headsetEnableConnectable(app); /* v100817 Disable Connectable Problem (AGHFP, A2DP, HFP) */

        LEDManagerIndicateState(&app->theLEDTask, headsetHfpConnectable, stateManagerGetA2dpState());
        TonesPlayTone(app, 9, TRUE);

        if(msg->status == aghfp_disconnect_link_loss && !(app->ag_bd_addr.nap == 0x24 && app->ag_bd_addr.uap == 0xbc && (app->ag_bd_addr.lap >= 0x100000 && app->ag_bd_addr.lap < 0x200000))) /* F100 È£È¯¼º */
            MessageSendLater(&app->task, AGHFP_LINK_LOSS_SLC_CONNECT_ATTEMPT, 0, D_SEC(20)); /* R100 */
        break;
    }

    case AGHFP_AUDIO_DISCONNECT_IND:
        INTERCOM_MSG_DEBUG(("AGHFP_AUDIO_DISCONNECT_IND\n"));

        app->audio_connect = FALSE;
        app->repeat_stop = FALSE;
        app->intercom_button = FALSE; /* R100 */

        AudioDisconnect();

        /* Turn the audio amp off after a delay */
        AmpOffLater(app);
        app->dsp_process = dsp_process_none;
        /* Set the mic bias */
        LEDManagerSetMicBias(app, FALSE);
        LEDManagerIndicateState(&app->theLEDTask, headsetHfpConnected, stateManagerGetA2dpState());
        TonesPlayTone(app, 2, TRUE);

        if(stateManagerGetHfpState() != headsetIncomingCallEstablish) /* Natural Volume Increase */
        {
#ifdef DUAL_STREAM
            if(app->a2dp_state_change) /* SUSPEND */
            {
                app->a2dp_state_change = FALSE;
            
                stateManagerEnterA2dpStreamingState(app);
                streamControlResumeA2dpStreaming(app, D_SEC(3));
            }
#else
            streamControlResumeA2dpStreaming(app, D_SEC(3));
#endif
        }
        else
        {
            if(app->a2dp_state_change) /* Music -> Intercom -> Incoming Call -> Call end -> Music -> Music OFF -> Intercom ON -> OFF -> Amp on. v110422 */
            {
                app->a2dp_state_change = FALSE;
            }
        }

        MessageSend(&app->task, EventSkipForward, 0);
        MessageSendLater(&app->task, EventSkipBackward, 0, D_SEC(1));
        break;

    case AGHFP_SLC_CONNECT_IND:
    {
        AGHFP_SLC_CONNECT_IND_T* msg = (AGHFP_SLC_CONNECT_IND_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_SLC_CONNECT_IND\n"));

        app->repeat_stop = TRUE;

        /* Ignore SLC CON IND while active call by Jace */
        switch (stateManagerGetHfpState() )
        {
            case headsetPoweringOn:
            case headsetConnDiscoverable:
            case headsetHfpConnectable:
            case headsetHfpConnected:
                MessageSendLater(&app->task, AGHFP_CONNECT_FAIL_TIMEOUT, 0, D_SEC(10));
                AghfpSlcConnectResponse(app->aghfp, TRUE, &msg->bd_addr);

                MessageSend(&app->task, EventSkipForward, 0);
                app->is_slc_connect_ind = TRUE; /* R100 */
                break;

            default:
                /* Ignore IND */
                AghfpSlcConnectResponse(app->aghfp, FALSE, &msg->bd_addr);
                break;
        }
        break;
    }

    case AGHFP_AUDIO_CONNECT_IND:
    {
        AGHFP_AUDIO_CONNECT_IND_T* msg = (AGHFP_AUDIO_CONNECT_IND_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_AUDIO_CONNECT_IND\n"));

        MessageSend(&app->task, EventSkipForward, 0);
        app->repeat_stop = TRUE;
        AghfpAudioConnectResponse(msg->aghfp, TRUE, sync_all_sco, 0);
        break;
    }

    case AGHFP_CSR_SUPPORTED_FEATURES_IND:
    {
        AGHFP_CSR_SUPPORTED_FEATURES_IND_T* msg = (AGHFP_CSR_SUPPORTED_FEATURES_IND_T*)message;
        INTERCOM_MSG_DEBUG(("AGHFP_CSR_SUPPORTED_FEATURES_IND\n"));

        aghfpCsrSupportedFeaturesResponse(msg->aghfp, 0, 0, 0, 0, 0, 7);
        aghfpFeatureNegotiate(msg->aghfp, 6, 1);
        break;
    }

    case AGHFP_CSR_FEATURE_NEGOTIATION_IND:
        INTERCOM_MSG_DEBUG(("AGHFP_CSR_FEATURE_NEGOTIATION_IND\n"));
        break;

    case AGHFP_STABILIZE_AUDIO_CONNECT:
        INTERCOM_MSG_DEBUG(("AGHFP_STABILIZE_AUDIO_CONNECT\n"));

        MessageCancelAll(&app->task, AGHFP_STABILIZE_AUDIO_CONNECT);
        AghfpAudioConnect(app->aghfp, sync_all_sco, 0);
        break;
        
    case AGHFP_CONNECT_FAIL_TIMEOUT:
        INTERCOM_MSG_DEBUG(("AGHFP_CONNECT_FAIL_TIMEOUT\n"));

        MessageCancelAll(&app->task, AGHFP_CONNECT_FAIL_TIMEOUT);
        TonesPlayTone(app, 8, TRUE);
        app->aghfp_connect = FALSE;
        app->audio_connect = FALSE;
        app->repeat_stop = FALSE;
        break;

    case AGHFP_LINK_LOSS_SLC_CONNECT_ATTEMPT: /* R100 */
        INTERCOM_MSG_DEBUG(("AGHFP_LINK_LOSS_SLC_CONNECT_ATTEMPT\n"));
        if(!app->aghfp_connect && app->aghfp_attempt_count < 30)
        {
            app->aghfp_attempt_count++;

            if(!app->aghfp_connecting)
            {
                app->aghfp_connecting = TRUE;
                MessageSend(&app->task, EventSkipForward, 0);
                AghfpSlcConnect(app->aghfp, &app->ag_bd_addr);
            }

            MessageSendLater(&app->task, AGHFP_LINK_LOSS_SLC_CONNECT_ATTEMPT, 0, D_SEC(20));
        }
        break;

    default:
        INTERCOM_MSG_DEBUG(("AGHFP UNHANDLED MSG: 0x%x\n", id));
        break;
    }
}
