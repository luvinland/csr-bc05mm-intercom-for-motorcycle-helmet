/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file   headset_private.h
@brief  The private data structures used by the stereo headset application.

    The main application task and global data is declated here.
*/

#ifndef _HEADSET_PRIVATE_H_
#define _HEADSET_PRIVATE_H_


#include "headset_buttonmanager.h"
#include "headset_leddata.h"
#include "headset_states.h"

#include <app/message/system_message.h>
#include <a2dp.h>
#include <avrcp.h>
#include <battery.h>
#include <csrtypes.h>
#include <message.h>
#include <hfp.h>
#include <sink.h>
#include <aghfp.h>

/* Class of device bitmask defines */
#define AUDIO_MAJOR_SERV_CLASS  0x200000    /*!< @brief Class of Device - Major Service - Audio */
#define AV_MAJOR_DEVICE_CLASS   0x000400    /*!< @brief Class of Device - Major Device - Audio/Video */
#define AV_MINOR_HEADSET        0x000004    /*!< @brief Class of Device - Minor Device - Wearable Headset Device */
#define AV_COD_RENDER           0x040000    /*!< @brief Class of Device - Major Device - Rendering */

#define A2DP_RESTART_DELAY      (uint32)500 /*!< @brief Restart A2DP Delay Time */

/*! @brief Definition of headset tone type */
typedef uint16 HeadsetTone_t ;
#define TONE_NOT_DEFINED        0           /*!< @brief Definition of empty entry in gEventTones array */

/* Local stream end point codec IDs */
#define SBC_SEID                1           /*!< @brief Local Stream End Point ID for SBC codec */
#if 0 /* TTA & A2DP conn */
#define MP3_SEID                2           /*!< @brief Local Stream End Point ID for MP3 codec */
#define FASTSTREAM_SEID         3           /*!< @brief Local Stream End Point ID for FASTSTREAM codec */
#define AAC_SEID                4           /*!< @brief Local Stream End Point ID for AAC codec */
#define NUM_SEPS                (AAC_SEID)  /*!< @brief The total number of SEPs */
#endif

/* The bits used to enable codec support for A2DP, as read from PSKEY_CODEC_ENABLED */
#define MP3_CODEC_BIT           0           /*!< @brief Bit used to enable MP3 codec in PSKEY_CODEC_ENABLED */
#define AAC_CODEC_BIT           1           /*!< @brief Bit used to enable AAC codec in PSKEY_CODEC_ENABLED */
#define FASTSTREAM_CODEC_BIT    2           /*!< @brief Bit used to enable FASTSTREAM codec in PSKEY_CODEC_ENABLED */

#define KALIMBA_RESOURCE_ID     1           /*!< @brief Resource ID for Kalimba */

/*! @brief Audio Codec Type Bitmask */
typedef enum 
{
    audio_codec_none = 0,
    audio_codec_cvsd = 1,
    audio_codec_auristream_2_bit = 2,
    audio_codec_auristream_4_bit = 4
} audio_codec_type ;


#define HEADSET_MSG_BASE        (0x0)       /*!< @brief Locally generated message base */

/*! @brief Enumeration of message IDs for application specific messages */
enum
{
    APP_RESUME_A2DP = HEADSET_MSG_BASE,
    APP_AVRCP_CONTROLS,
    APP_AVRCP_CONNECT_REQ,
    APP_AMP_OFF,
    APP_SEND_PLAY,
    APP_CHARGER_MONITOR,
    APP_INTERCOM_MODE,
    HEADSET_MSG_TOP
};


/*! @brief Enumeration of AVRCP controls */
typedef enum
{
    AVRCP_CTRL_PAUSE_PRESS,
    AVRCP_CTRL_PAUSE_RELEASE,
    AVRCP_CTRL_PLAY_PRESS,
    AVRCP_CTRL_PLAY_RELEASE,
    AVRCP_CTRL_FORWARD_PRESS,
    AVRCP_CTRL_FORWARD_RELEASE,
    AVRCP_CTRL_BACKWARD_PRESS,
    AVRCP_CTRL_BACKWARD_RELEASE,
    AVRCP_CTRL_STOP_PRESS,
    AVRCP_CTRL_STOP_RELEASE,
    AVRCP_CTRL_FF_PRESS,
    AVRCP_CTRL_FF_RELEASE,
    AVRCP_CTRL_REW_PRESS,
    AVRCP_CTRL_REW_RELEASE
} avrcp_controls;


/*! @brief Definition of application message used for sending AVRCP control commands */
typedef struct
{
    avrcp_controls control;
} APP_AVRCP_CONTROLS_T;

/*! @brief Definition of application message used for connecting AVRCP */
typedef struct
{
    bdaddr addr;
} APP_AVRCP_CONNECT_REQ_T;


/*! @brief Enumeration of DSP processing task types */
typedef enum
{
    dsp_process_none = 0,
    dsp_process_sco  = 1,
    dsp_process_a2dp = 2
} dspProcessType;


/*! @brief Definition of the battery monitoring configuration */
typedef struct
{
    uint16 divisor_ratio;

    unsigned low_threshold:8 ;
    unsigned shutdown_threshold:8 ;  

    unsigned high_threshold:8 ;
    unsigned monitoring_period:8 ;

}battery_config_type;

/*! @brief Definition of the power configuration */
typedef struct
{
    battery_config_type battery;
} power_config_type;

/*! @brief Task to receive AIO readings */
typedef struct
{
    TaskData        task;
    BatteryState    state;
    uint16          current_reading;
} aioTask;

/*! @brief Enumeration of Charger states */
typedef enum
{
    disconnected,       /*!< Charger is disconnected */
    trickle_charge,     /*!< Charger is trickle charging */
    fast_charge,        /*!< Charger is fast charging */
    charge_error        /*!< Charger error */
} charging_state_type;


/*! @brief Possible sample rate values for A2DP */
typedef enum
{
    a2dp_rate_48_000k,
    a2dp_rate_44_100k,
    a2dp_rate_32_000k,
    a2dp_rate_24_000k,
    a2dp_rate_22_050k,
    a2dp_rate_16_000k
} a2dp_rate_type;


/*! @brief Definition of the power state */
typedef struct
{
    power_config_type   config;     /*!< Configuration */
    aioTask             vbat_task;  /*!< Vbat */
} power_type;


/*! @brief AVRCP Specific data */
typedef struct
{
    bool pending;       /*!< AVRCP is pending a command response */
    uint16 send_play;   /*!< Should a play be sent */
} avrcpData;

/*! @brief Sink codec data. */
/* This struct must match with the codec_data_type in a2dp.h. We've just missed off the last uint16 which isn't needed here */
typedef struct
{
    uint8 content_protection;
    uint32 voice_rate;
    unsigned bitpool:8;
    unsigned format:8;
} sink_codec_data_type;


/*! @brief A2DP Specific data */
typedef struct
{
    device_sep_list *sep_entries;       /*!< The Stream End Point (SEP) pointer */
    sink_codec_data_type codecData;     /*!< The audio codec related data */
} a2dpData;


/*! @brief The application timeouts */
typedef struct TimeoutsTag
{
    uint16 PairModeTimeout_s ;
    uint16 MuteRemindTime_s ;
    uint16 AutoSwitchOffTime_s ;
}Timeouts_t ;


/*! @brief The amp control */
typedef struct AmpTag
{
    unsigned useAmp:1;
    unsigned ampAutoOff:1;
    unsigned unused:1;
    unsigned ampPio:5;
    unsigned ampOffDelay:8;
}Amp_t ;


/*! @brief The application features block.

    Used to map application feature configuration bits when read from persistent storage.
    Corresponding bits are set in the hsTaskData structure for use by the application to determine if a feature is enabled.

    Please refer to the stereo headset user guide document for configuration details on
    the features listed here.
*/
typedef struct FeaturesTag
{
    unsigned autoSendAvrcp:1;       /*!< set to enable automatic sending of AVRCP commands by the application */
    unsigned cvcEnabled:1;          /*!< set to enable cVc */
    unsigned forceMitmEnabled:1;    /*!< set to force ManInTheMiddle during pairing */
    unsigned writeAuthEnable:1;     /*!< set to enable writeAuthEnable */
    unsigned debugKeysEnabled:1;    /*!< set to enable toggling of debug keys for SSP */
    unsigned dummy:11;              /*!< remaining unused bits in the bitfield */
}Features_t ;

/*! @brief Sniff SubRate parameters. */
typedef struct
{
    uint16 max_remote_latency;      /*!< sniff subrate maximum remote latency */
    uint16 min_remote_timeout;      /*!< sniff subrate minimum remote timeout */
    uint16 min_local_timeout;       /*!< sniff subrate minimum local timeout */
} ssr_params;

/*! @brief Sniff Subrate data for all application connections. */
typedef struct
{
    ssr_params streaming_params;    /*!< Sniff subrate parameters for streaming connections - SCO & A2DP Media */
    ssr_params signalling_params;   /*!< Sniff subrate parameters for signalling connections - SLC, A2DP Signalling & AVRCP */

    /* potentially we might require separate params for each sink, but
       would require 9 more words */

} subrate_data;

/*! @brief Headset data

    Global data for the stereo headset application.
*/
typedef struct
{
    TaskData            task;                           /*!< Main task for the stereo headset application */
    ButtonsTaskData     theButtonTask;                  /*!< Headset buttons task */
    LedTaskData         theLEDTask;                     /*!< Headset LEDs task */
    Task                theCodecTask;                   /*!< Codec task */

    a2dpData            a2dp_data;                      /*!< Local A2DP data */
    avrcpData           avrcp_data;                     /*!< Local AVRCP data */

    HFP                 *hfp;                           /*!< Pointer to Hands Free Profile instance */
    HFP                 *hsp;                           /*!< Pointer to Headset Profile instance */
    HFP                 *hfp_hsp;                       /*!< Pointer to either the HFP or HSP instance (whichever has been instantiated) */
    HFP                 *intercom_hsp;                 /*!< Pointer to Headset Profile instance for intercom */
    A2DP                *a2dp;                          /*!< Pointer to A2DP Profile instance */
    AVRCP               *avrcp;                         /*!< Pointer to AVRCP Profile instance */

    power_type          *power;                         /*!< Pointer to headset power configuration */
    Timeouts_t          Timeouts ;                      /*!< Application timeouts */
    HeadsetTone_t       *gEventTones ;                  /*!< Pointer to array of headset tones */

    subrate_data        ssr_data;                       /*!< Sniff Subrate parameters */
    bdaddr*             confirmation_addr;              /*!< user confirmation data */

    AGHFP               *aghfp;
    Sink                audio_sink;
    bdaddr              ag_bd_addr;
    unsigned            aghfp_initial:1;
    unsigned            slave_function:1;
    unsigned            aghfp_connect:1;
    unsigned            audio_connect:1;
    unsigned            aghfp_connecting:1; /* R100 */
    uint16              aghfp_attempt_count; /* R100 */
    unsigned            is_slc_connect_ind:1; /* R100 */
    unsigned            intercom_button:1; /* R100 */
    unsigned            repeat_stop:1;
    unsigned            intercom_init:1;
    unsigned            normal_answer:1;
    unsigned            extmic_mode:1;
    uint16              battery_mv; /* READ_VOL */
#ifndef AUTO_MIC_DETECT
    unsigned            extmic_evt:1;
#endif
    unsigned            reset_complete:1;
#ifdef AUTO_INT_AFTERCALL
    unsigned            discon_intercom_incoming:1; /* v100617 Auto connetion INTERCOM after end call */
#endif
#ifdef DUAL_STREAM
    unsigned            a2dp_state_change:1; /* SUSPEND */
#endif
#ifdef S100A
    unsigned            intercom_pairing_mode:1; /* v100622 Intercom Pairing mode */
    unsigned            init_aghfp_power_on:1;
#endif
#ifdef SINPUNG
    unsigned            slave_function_sinpung:1;
#endif
#ifdef BEEP_AUDIO_CON /* Not beep audio connection flag */
    unsigned            beep_audio_con:1;
#endif

    unsigned            voice_manual_mode:1; /* v100225 */
    
    hfp_profile         profile_connected:2;            /*!< Determines if the HFP or HSP profile is in use */
    unsigned            local_hfp_features:8;           /*!< Local HFP features that are enabled */
    unsigned            initial_battery_reading:1;      /*!< Flag to indicate if the initail battery reading is being taken */
    unsigned            a2dp_channel_mode:4;            /*!< A2DP Channel Mode */
    unsigned            connect_a2dp_when_no_call:1;    /*!< Flag to indicate that A2DP should be connected once call activity has ended */


    unsigned            seid:8;	                        /*!< Active SEP ID */
    charging_state_type charger_state:2;                /*!< The current charger state */
    unsigned            gMuted:1 ;                      /*!< Flag to indicate if call audio is muted */
    unsigned            gHfpVolumeLevel:5 ;             /*!< The HFP audio volume level */


    unsigned            gAvVolumeLevel:5 ;              /*!< The A2DP audio volume level */
    a2dp_rate_type      a2dp_rate:3;                    /*!< A2DP rate */
    unsigned            buttons_locked:1 ;             /*!< Flag to indicate if button locking is enabled */
    unsigned            useAmp:1 ;                      /*!< Flag to indicate if an audio amp is used on the hardware. */
    unsigned            ampPio:5;                       /*!< The PIO that is used to turn the audio amp on and off */


    unsigned            inquiry_scan_enabled:1;         /*!< Flag to indicate if inquiry scan is enabled */
    unsigned            page_scan_enabled:1 ;           /*!< Flag to indicate if page scan is enabled */
    unsigned            dsp_process:3;                  /*!< Flag to indicate if the dsp processing SCO, or A2DP, or neither */
    unsigned            slcConnectFromPowerOn:1 ;       /*!< Flag to indicate if headset is connecting HFP due to power on reconnect procedure */
    unsigned            slcConnecting:1 ;	        /*!< Flag to indicate if headset is connecting HFP */
    unsigned            a2dpConnecting:1;               /*!< Flag to indicate if headset is connecting A2DP */
    unsigned            a2dpSourceSuspended:1;          /*!< Flag to indicate if headset successfully suspended A2DP source */
    unsigned            ampAutoOff:1;                   /*!< Flag to indicate if amp should be turned off when there's no audio */
    unsigned            ampOn:1;                        /*!< Flag to indicate if the amp is currently switched on */
    unsigned            sendPlayOnConnection:1;         /*!< Flag to indicate if an avrcp_play should be sent once media is connected */
    unsigned            autoSendAvrcp:1;                /*!< Flag to indicate if the headset should auto send AVRCP commands to try and pause/resume music playing */
    unsigned            combined_link_loss:1;           /*!< Flag to indicate that if one profile has had link loss, it's probable the other profile will have link loss, if it's the same device */
    unsigned            InBandRingEnabled:1;            /*!< Flag to indicate if inband ringtones have been enabled from the AG */
    unsigned            PlayingState:1;                 /*!< Flag to indicate the suspected status of the playing music. ie. Playing (1) or Paused/Stopped (0). This is used to hold the state when not actually streaming as the source can be suspended but AVRCP commands can still be sent. */


    unsigned            RingTone:5;                     /*!< Flag to indicate the ringtone used by the headset for out-of-band ringing */
    unsigned            cvcEnabled:1;                   /*!< Flag to indicate if CVC should be used for (e)SCO */
    unsigned            ampOffDelay:8;                  /*!< Time in secs to wait after there's no audio, before turning audio amp off */
    unsigned            forceMitmEnabled:1;             /*!< flag to indicate if forcing MITM is a supported feature */
    unsigned            spare_bit:1;                    /*!< remaining unused bit in this word */


    unsigned            confirmation:1;                 /*!< flag to indicate we have user confirmation data to send */
    unsigned            writeAuthEnable:1;              /*!< flag to indicate write auth enable feature is configured */
    unsigned            debugKeysEnabled:1;             /*!< flag to indicate if debug keys are a supported feature */
    unsigned            debugKeysInUse:1;               /*!< flag to indicate if the SSP debug keys are being used */
    unsigned            a2dpCodecsEnabled:4;            /*!< codec enabled */
    unsigned            SCO_codec_selected:3;           /*!< which audio codec is selected - audio_codec_type */
    unsigned            unused:5;                       /*!< remaining unused bits in this word */

} hsTaskData;


/*!
    @brief Returns the headset application main task.
   
    Accessor function used throughout the stereo headset application to access the application task (hsTaskData.task),
    as needed for message communications with the bluelab libraries.

    @returns Task The headset application task.
*/
Task getAppTask(void);




#endif /* HEADSET_PRIVATE_H_ */
