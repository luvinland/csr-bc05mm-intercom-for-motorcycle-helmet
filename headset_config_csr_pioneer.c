/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_config_csr_pioneer.c
@brief    This file contains the default configuration parameters for a CSR Stereo headset.
*/


#include "headset_config.h"


#define DEFAULT_CONFIG_CSR_PIONEER
#ifdef DEFAULT_CONFIG_CSR_PIONEER


/* PSKEY_USR_0 - Battery config */
static const config_battery_type battery_config =
{
 	sizeof(battery_config_type),
    {
        0x0442,  /*Divider ratio*/	
        0xA596,  /*Minimum threshold	*/         /*Shutdown threshold*/	
        0xB91e   /*High threshold	*/             /*Monitoring period (secs)*/
    }    
};


/* PSKEY_USR_1 - Button config */
static const config_button_type button_config =
{
  	sizeof(button_config_type),
    {
      	0x01f4, /* Button double press time */
      	0x03e8, /* Button long press time */
      	0x0DAC, /* Button very long press time */
      	0x0320, /* Button repeat time */
      	0x1f40, /* Button Very Very Long Duration*/
		0x040f	/* Debounce (number of reads ; time between reads) */
  	}   
};


/* PSKEY_USR_2 - buttonpatterns */
#define NUM_BUTTON_PATTERNS 2

static const config_button_pattern_type button_pattern_config = 
{
    sizeof(button_pattern_config_type ) * NUM_BUTTON_PATTERNS ,
    
    { 
		/* enter DUT mode (MFB, MFB, FWD/BACK, Vol+/Vol-, MFB) */
        0x0e   ,0x0100,0x0000, 0x0100,0x0000, 0x0000,0xc000, 0x0000,0x1800, 0x0100,0x0000, 0,0, 
		/* enter DFU mode (PLAY, PLAY, FWD/BACK, Vol+/Vol-, PLAY) */
        0x3f   ,0x0000,0x2000, 0x0000,0x2000, 0x0000,0xc000, 0x0000,0x1800, 0x0000,0x2000, 0,0
    }    
    
};


/* PSKEY_USR_6 - timeouts */
static const config_timeouts timeouts =
{
  	3,
  	{
  	0x00b4 , /*PairModeTimeout_s*/ 
    0x0005 , /*MuteRemindTime_s*/
    0x0258 	 /*AutoSwitchOffTime_s*/
	}
};


/* PSKEY_USR_9 - amp config */
static const config_amp amp_config =
{
  	1,
  	{
		0x4305  		/*useAmp:1 ampAutoOff:1 unused:1 ampPio:5 ampOffDelay:8*/
	}
};


/* PSKEY_USR_15 - Number of LED filters */
#define NO_LED_FILTERS  (0x0003)

static const config_uint16_type no_led_filters =
{
  	1,
  	{NO_LED_FILTERS}
};


/* PSKEY_USR_16 - LED filter configuration */
static const config_led_filters_type led_filters =
{
 	sizeof(led_filter_config_type) * NO_LED_FILTERS,
 	{
/*1*/        0x1b00, 0x800e, 0x8000,     /*new filter for charger connected */
/*2*/        0x1c00, 0x0010, 0x0000,     /*cancel for chg disconnected*/ 			 		 
/*3*/        0x0e00, 0x0010, 0x0000      /*cancel for DUT mode*/ 			 		 			 
 	}, 
};


/* PSKEY_USR_17 - Number of LED states */
#define NO_LED_STATES_A  (0x0010)

static const config_uint16_type no_led_states_a =
{
  	1,
  	{NO_LED_STATES_A}
};


/* PSKEY_USR_18 - LED state configuration */
static const config_led_states_type led_states_a =
{
  	sizeof(led_config_type) * NO_LED_STATES_A,
  	{
        0x0100, 0x0505, 0x0100, 0x002f, 0xe100, /*pairing mode*/
                
        0x0200, 0x0a64, 0x1400, 0x002f, 0xe100, /*hfp connectable, a2dp connectable*/    	
    	0x0300, 0x05c8, 0x2800, 0x002f, 0xe100, /*hfp connected,   a2dp connectable*/    	
    	0x0400, 0x05c8, 0x2800, 0x002f, 0xe100, /*outgoing call,   a2dp connectable*/
    	0x0500, 0x0505, 0x0100, 0x002f, 0xe100, /*incoming call,   a2dp connectable*/    	
    	0x0600, 0x05c8, 0x2800, 0x002f, 0xe100, /*active call,     a2dp connectable*/
        
    	0x0201, 0x0a64, 0x1400, 0x002f, 0xe100, /*hfp connectable, a2dp connected*/    	
    	0x0301, 0x05c8, 0x2800, 0x002f, 0xe100, /*hfp connected,   a2dp connected*/    	
    	0x0401, 0x05c8, 0x2800, 0x002f, 0xe100, /*outgoing call,   a2dp connected*/
    	0x0501, 0x0505, 0x0100, 0x002f, 0xe100, /*incoming call,   a2dp connected*/    	
    	0x0601, 0x05c8, 0x2800, 0x002f, 0xe100, /*active call,     a2dp connected*/
        
        0x0202, 0x0a64, 0x1400, 0x002f, 0xe100, /*hfp connectable, a2dp streaming*/    	
    	0x0302, 0x05c8, 0x2800, 0x002f, 0xe100, /*hfp connected,   a2dp streaming*/    	
    	0x0402, 0x05c8, 0x2800, 0x002f, 0xe100, /*outgoing call,   a2dp streaming*/
    	0x0502, 0x0505, 0x0100, 0x002f, 0xe100, /*incoming call,   a2dp streaming*/    	
    	0x0602, 0x05c8, 0x2800, 0x002f, 0xe100  /*active call,     a2dp streaming*/		
  	}, 
};

/* PSKEY_USR_19 - Number of LED states */
#define NO_LED_STATES_B  (0x0006)

static const config_uint16_type no_led_states_b =
{
  	1,
  	{NO_LED_STATES_B}
};


/* PSKEY_USR_20 - LED state configuration */
static const config_led_states_type led_states_b =
{
  	sizeof(led_config_type) * NO_LED_STATES_B,
  	{ 
        0x0203, 0x0a64, 0x1400, 0x002f, 0xe100, /*hfp connectable, a2dp paused*/    	
    	0x0303, 0x05c8, 0x2800, 0x002f, 0xe100, /*hfp connected,   a2dp paused*/    	
    	0x0403, 0x05c8, 0x2800, 0x002f, 0xe100, /*outgoing call,   a2dp paused*/
    	0x0503, 0x0505, 0x0100, 0x002f, 0xe100, /*incoming call,   a2dp paused*/    	
    	0x0603, 0x05c8, 0x2800, 0x002f, 0xe100, /*active call,     a2dp paused*/    	
		
		0x0700, 0xff00, 0x0100, 0x00ff, 0xe400  /*test mode*/   
		
  	}, 
};

/* PSKEY_USR_21 - Number of LED events */
#define NO_LED_EVENTS  (0x000a)

static const config_uint16_type no_led_events =
{
  	1,
  	{NO_LED_EVENTS}
};


/* PSKEY_USR_22 - LED event configuration */
static const config_led_events_type led_events =
{
 	sizeof(led_config_type) * NO_LED_EVENTS,
 	{
   		0x0100, 0x6464, 0x0000, 0x001f, 0xe200, /*power on*/
        0x0200, 0x6464, 0x0000, 0x001f, 0xe200, /*power off*/
        0x0600, 0x0a0a, 0x0000, 0x001f, 0xe200, /*answer*/
        0x1500, 0x0a0a, 0x0000, 0x001f, 0xe200, /*end of call*/
        0x0d00, 0x6432, 0x0000, 0x002f, 0xe200, /*reset paired devices*/
        0x1400, 0x0505, 0x0000, 0x002f, 0xe200, /*low battery*/
        0x1f00, 0x3232, 0x0000, 0x002f, 0xe200, /*link loss*/
        0x0a00, 0x0505, 0x0000, 0x001f, 0xe300, /*toggle mute*/
        0x2500, 0x0505, 0x0000, 0x001f, 0xe200, /*slc connected*/
		0x3a00, 0x0505, 0x0000, 0x001f, 0xe200  /*a2dp connected*/
 	}, 
};


#define  NO_EVENTS  (20)

/* PSKEY_USR_23 - System event configuration A */
static const config_events_type events_a =
{
    sizeof(event_config_type) * NO_EVENTS,
  	{
        0x0102, 0x0100, 0x0000, 0x0101, /*power On (long MFB, powering on)*/
    	0x0202, 0x0100, 0x0000, 0xfeff, /*power Off (long MFB, powered on)*/
        0x1b06, 0x0200, 0x0000, 0xffff, /*charger connected (rising edge)*/
    	0x1c07, 0x0200, 0x0000, 0xffff, /*charger disconnected (falling edge)*/
               
    	0x0303, 0x0100, 0x0000, 0x0401, /*enter pairing (v long MFB, HFP connectable)*/       
        0x0408, 0x0100, 0x0000, 0x08ff, /*initiate voice dial (short MFB, HFP connected)*/       
        0x0504, 0x0100, 0x0000, 0x08ff, /*initiate LNR (double MFB, HFP connected)*/       
        0x0608, 0x0100, 0x0000, 0x20ff, /*answer call (short MFB, incoming call)*/     
        
        0x0704, 0x0100, 0x0000, 0x20ff, /*reject call (double MFB, incoming call)*/       
        0x0808, 0x0100, 0x0000, 0x50ff, /*end call (short MFB, outgoing/active call)*/              
        0x0904, 0x0100, 0x0000, 0x40ff, /*transfer audio (double MFB, active call)*/       
        0x3e09, 0x0100, 0x0000, 0x04ff, /*connect from power on (long release MFB, HFP connectable)*/    
        
        0x1608, 0x0100, 0x0000, 0x04ff, /*connect HFP from headset (short MFB, HFP connectable)*/       
    	0x0d0b, 0x0100, 0x0000, 0xfeff, /*reset paired device list (vv long MFB, powered on)*/               
        0x2901, 0x0000, 0x0001, 0xfeff,	/*button locking (short press bluemedia button, powered on)*/
        0x0b01, 0x0000, 0x0800, 0xfeff, /*vol up*/    
        
    	0x0b05, 0x0000, 0x0800, 0xfeff, /*vol up repeat*/       
        0x0c01, 0x0000, 0x1000, 0xfeff, /*vol down*/       
        0x0c05, 0x0000, 0x1000, 0xfeff, /*vol down repeat*/       
        0x0a02, 0x0000, 0x1800, 0x40ff, /*toggle mute (long vol up + vol down, active call)*/
    },
};


/* PSKEY_USR_24 - System event configuration B */
static const config_events_type events_b =
{

    sizeof(event_config_type) * NO_EVENTS,
    {
        0x3901, 0x0000, 0x2000, 0xfc01,	/*play (short press, a2dp not connected)*/        
        0x3001, 0x0000, 0x2000, 0xfc0a, /*play (short press, a2dp connected/paused)*/
        0x3101, 0x0000, 0x2000, 0xfc04, /*pause (short press, a2dp streaming)*/       
        0x3202, 0x0000, 0x2000, 0xfc0e, /*stop (long press, a2dp connected/streaming/paused)*/    
        
        0x3701, 0x0000, 0x4000, 0xfc0e, /*skip forward (short fwd, a2dp connected/streaming/paused)*/           
        0x3801, 0x0000, 0x8000, 0xfc0e, /*skip backward (short back, a2dp connected/streaming/paused)*/        
        0x3302, 0x0000, 0x4000, 0xfc0e, /*fast forward press (long fwd, a2dp connected/streaming/paused)*/
		0x3305, 0x0000, 0x4000, 0xfc0e, /*fast forward repeat (fwd long held, a2dp connected/streaming/paused)*/        

        0x3409, 0x0000, 0x4000, 0xfc0e, /*fast forward long release (fwd long release, a2dp connected/streaming/paused)*/
        0x340a, 0x0000, 0x4000, 0xfc0e, /*fast forward v long release (fwd v long release, a2dp connected/streaming/paused)*/
        0x340c, 0x0000, 0x4000, 0xfc0e, /*fast forward vv long release (fwd vv long release, a2dp connected/streaming/paused)*/
        0x3502, 0x0000, 0x8000, 0xfc0e, /*fast rewind press (long back, a2dp connected/streaming/paused)*/

        0x3505, 0x0000, 0x8000, 0xfc0e, /*fast rewind repeat (back long held, a2dp connected/streaming/paused)*/        
        0x3609, 0x0000, 0x8000, 0xfc0e, /*fast rewind long release (back long release, a2dp connected/streaming/paused)*/
        0x360a, 0x0000, 0x8000, 0xfc0e, /*fast rewind v long release (back v long release, a2dp connected/streaming/paused)*/
        0x360c, 0x0000, 0x8000, 0xfc0e, /*fast rewind vv long release (back vv long release, a2dp connected/streaming/paused)*/

        0x1604, 0x0100, 0x0000, 0x04ff, /*connect HFP from headset (double MFB, HFP connectable)*/
        0x3704, 0x0000, 0x4000, 0xfc0e, /*skip forward (double fwd, a2dp connected/streaming/paused)*/           
        0x3804, 0x0000, 0x8000, 0xfc0e, /*skip backward (double back, a2dp connected/streaming/paused)*/   
        0x0000, 0x0000, 0x0000, 0x0000		
    },
};


/* PSKEY_USR_25 - Number of tone events */
#define NO_TONE_EVENTS  (0x0018)

static const config_uint16_type no_tone_events =
{
  	1,
  	{NO_TONE_EVENTS}
};


/* PSKEY_USR_26 - Tone event configuration */
static const config_tone_events_type tone_events =
{
  	sizeof(tone_config_type) * NO_TONE_EVENTS,
  	{
    	0x0101, /*power on*/
        0x0201, /*power off*/
        0x0302, /*enter pairing*/
        0x0d02, /*reset paired devices*/
        0x0409, /*voice dial*/
        0x050a, /*LNR*/
        0x090a, /*transfer audio*/
        0x0609, /*answer*/
        0x070a, /*reject*/
        0x0809, /*cancel end*/
        0x1405, /*low battery*/
        0x2104, /*mute on*/
        0x2203, /*mute off*/
        0x2507, /*slc connected*/
        0x2608, /*error*/
        0xff0c, /*ringtone*/
		0x3c06,	/*volume max*/	
		0x3d06,	/*volume min*/	
		0x3a07, /*a2dp connected*/
		0x230b,	/*mute reminder*/
		0x1609, /*connect hfp*/
		0x3909,	/*connect a2dp*/
		0x0f04, /*button lock on*/
        0x4003, /*button lock off*/
  	}
};


/* PSKEY_USR_28 - unused6 */
static const config_uint16_type tri_col_leds = 
{
    1,
    {0xfe50}
};

/* PSKEY_USR_36 - volume levels */
static const config_volume_type vol_gains = 
{
    (VOL_NUM_VOL_SETTINGS),
    {
	    0x0000,
	    0x0101,
	    0x0202,
	    0x0303,
	    0x0404,
	    0x0505,
	    0x0606,
	    0x0707,
	    0x0808,
	    0x0909,
	    0x0a0a,
	    0x0b0b,
	    0x0c0c,
	    0x0d0d,
	    0x0e0e,
	    0x0f0f
    }
};


/* PSKEY_USR_37 - features */
static const config_features features = 
{
    1,
    {
        0xc000  /* autoSendAvrcp:1 cvcEnabled:1 forceMitmEnabled:1 writeAuthEnable:1
                   debugKeysEnabled:1 unused:11 */
    }
};


/* PSKEY_USR_40 - Sniff Subrate parameters */
static const config_ssr_params_type ssr_config =
{
    6,
    {
        0x0000, 0x0000, 0x0000,     /* stream params SCO & A2DP Media */
        0x0000, 0x0000, 0x0000      /* signalling params SLC, A2DP Signalling & AVRCP */
    }
};

/* CSR Pioneer Default Configuration */
const config_type csr_pioneer_default_config = 
{
    &battery_config,        	/* PSKEY_USR_0 - Battery configuration */
  	&button_config,       		/* PSKEY_USR_1 - Button configuration */
  	&button_pattern_config,		/* PSKEY_USR_2 - */
    &timeouts, 		        	/* PSKEY_USR_6 - Timeouts*/
	&amp_config, 		        /* PSKEY_USR_9 - Amp*/
  	&no_led_filters,       		/* PSKEY_USR_15 - Number of LED filters */
  	&led_filters,         		/* PSKEY_USR_16 - LED filter configuration */
  	&no_led_states_a,      		/* PSKEY_USR_17 - Number of LED states */
  	&led_states_a,         		/* PSKEY_USR_18 - LED state configuration */
	&no_led_states_b,      		/* PSKEY_USR_19 - Number of LED states */
  	&led_states_b,         		/* PSKEY_USR_20 - LED state configuration */
  	&no_led_events,        		/* PSKEY_USR_21 - Number of LED events */
  	&led_events,         		/* PSKEY_USR_22 - LED event configuration */
  	&events_a,          		/* PSKEY_USR_23 - Number of system events */
  	&events_b,           		/* PSKEY_USR_24 - System event configuration */
  	&no_tone_events,       		/* PSKEY_USR_25 - Number of tone events */
  	&tone_events,         		/* PSKEY_USR_26 - Tone event configuration */
  	&vol_gains,        			/* PSKEY_USR_35 - Volume Gains */
    &features,         			/* PSKEY_USR_37 - Features */
    &ssr_config                         /* PSKEY_USR_40 - Sniff Subrate parameters */
};


#endif
