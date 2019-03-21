/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
Part of Stereo-Headset-SDK Q3-2007.RC3.1
*/

/*!
@file	headset_link_policy.c
@brief  Implementation of headset link policy control functions.
*/
#include <connection.h>


/****************************************************************************
	Header files
*/
#include "headset_a2dp_stream_control.h"
#include "headset_debug.h"
#include "headset_hfp_slc.h"
#include "headset_link_policy.h"
#include "headset_private.h"
#include "headset_statemanager.h"

#include <bdaddr.h>
#include <sink.h>


#ifdef DEBUG_LINK_POLICY
#define POLICY_DEBUG(x) DEBUG(x)
#else
#define POLICY_DEBUG(x) 
#endif   

#ifdef SEHWA_TEST
/* Lower power table for the HFP. */
static const lp_power_table hfp_powertable[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_active,		0,	    	  0,			0,		 0,		0},    /* Active mode for 1 second */
	{lp_sniff,		800,	      800,			4,		 1,	    0}    /* Enter sniff mode (500mS)*/
};
#else
/* Lower power table for the HFP. */
static const lp_power_table hfp_powertable[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_active,		0,	    	  0,			0,		 0,		1},    /* Active mode for 1 second */
	{lp_sniff,		800,	      800,			4,		 1,	    0}    /* Enter sniff mode (500mS)*/
};
#endif


/* Lower power table for the HFP when an audio connection is open */
static const lp_power_table hfp_powertable_sco[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_passive,		0,	    	  0,			0,		 0,		0}    /* Passive mode */
};


/* Lower power table for the A2DP. */
static const lp_power_table a2dp_powertable[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_active,		0,	    	  0,			0,		 0,		1},    /* Active mode for 1 sec */	
	{lp_passive,	0,	    	  0,			0,		 0,		4},    /* Passive mode for 4 secs */	
	{lp_sniff,		32,	      	  200,			1,		 8,	    0}	   /* Enter sniff mode (20-40mS) */
};


/* Lower power table for the AVRCP. */
static const lp_power_table avrcp_powertable[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_passive,	0,	    	  0,			0,		 0,		0} /* Go into passive mode and stay there */
};


static void setHfpLinkPolicy(hsTaskData *app)
{
	Sink slc_sink = HfpGetSlcSink(app->hfp_hsp);
	Sink audio_sink = HfpGetAudioSink(app->hfp_hsp);
    Sink intercom_slc_sink = HfpGetSlcSink(app->intercom_hsp);
    Sink intercom_audio_sink = HfpGetAudioSink(app->intercom_hsp);

    if(slc_sink)
    {
        /* Update link to use HFP power table */
        if (audio_sink)
        {
            /* set up our sniff sub rate params for SCO link - use streaming parameters */
            ConnectionSetSniffSubRatePolicy(audio_sink,
                                            app->ssr_data.streaming_params.max_remote_latency,
                                            app->ssr_data.streaming_params.min_remote_timeout,
                                            app->ssr_data.streaming_params.min_local_timeout);
            /* SCO is active */
            ConnectionSetLinkPolicy(slc_sink, 1, hfp_powertable_sco);

            POLICY_DEBUG(("LINK POLICY : Set SCO power table for sink 0x%x\n",(uint16)slc_sink));
        }
        else
        {
            /* set up our sniff sub rate params for SLC link - use signalling parameters */
            ConnectionSetSniffSubRatePolicy(slc_sink,
                                            app->ssr_data.signalling_params.max_remote_latency,
                                            app->ssr_data.signalling_params.min_remote_timeout,
                                            app->ssr_data.signalling_params.min_local_timeout);
            /* SCO isn't active */
            ConnectionSetLinkPolicy(slc_sink, 2, hfp_powertable);

            POLICY_DEBUG(("LINK POLICY : Set HFP power table for sink 0x%x\n",(uint16)slc_sink));
        }

        ConnectionSetLinkSupervisionTimeout(slc_sink, 0x1f80);
    }

    if(intercom_slc_sink)
    {
        /* Update link to use HFP power table */
        if (intercom_audio_sink)
        {
            /* set up our sniff sub rate params for SCO link - use streaming parameters */
            ConnectionSetSniffSubRatePolicy(intercom_audio_sink,
                                            app->ssr_data.streaming_params.max_remote_latency,
                                            app->ssr_data.streaming_params.min_remote_timeout,
                                            app->ssr_data.streaming_params.min_local_timeout);
            /* SCO is active */
            ConnectionSetLinkPolicy(intercom_slc_sink, 1, hfp_powertable_sco);

            POLICY_DEBUG(("LINK POLICY : Set SCO power table for sink 0x%x\n",(uint16)intercom_slc_sink));
        }
        else
        {
            /* set up our sniff sub rate params for SLC link - use signalling parameters */
            ConnectionSetSniffSubRatePolicy(intercom_slc_sink,
                                            app->ssr_data.signalling_params.max_remote_latency,
                                            app->ssr_data.signalling_params.min_remote_timeout,
                                            app->ssr_data.signalling_params.min_local_timeout);
            /* SCO isn't active */
            ConnectionSetLinkPolicy(intercom_slc_sink, 2, hfp_powertable);

            POLICY_DEBUG(("LINK POLICY : Set HFP power table for sink 0x%x\n",(uint16)intercom_slc_sink));
        }

        ConnectionSetLinkSupervisionTimeout(intercom_slc_sink, 0x1f80);
    }
}


static void setAvrcpLinkPolicy(hsTaskData *app)
{
	Sink avrcp_sink = AvrcpGetSink(app->avrcp);
    /* set up our sniff sub rate params for AVRCP link - use signalling parameters */
    ConnectionSetSniffSubRatePolicy(avrcp_sink,
                                        app->ssr_data.signalling_params.max_remote_latency,
                                        app->ssr_data.signalling_params.min_remote_timeout,
                                        app->ssr_data.signalling_params.min_local_timeout);
	ConnectionSetLinkPolicy(avrcp_sink, 1, avrcp_powertable);
	ConnectionSetLinkSupervisionTimeout(avrcp_sink, 0x1f80);
	
	POLICY_DEBUG(("LINK POLICY : Set AVRCP power table for sink 0x%x\n",(uint16)avrcp_sink));
}


static void setA2dpLinkPolicy(hsTaskData *app)
{
	Sink a2dp_sink = A2dpGetSignallingSink(app->a2dp);
    /* set up our sniff sub rate params for A2DP link - use streaming parameters */
    ConnectionSetSniffSubRatePolicy(a2dp_sink,
                                        app->ssr_data.streaming_params.max_remote_latency,
                                        app->ssr_data.streaming_params.min_remote_timeout,
                                        app->ssr_data.streaming_params.min_local_timeout);
	ConnectionSetLinkPolicy(a2dp_sink, 3, a2dp_powertable);
	ConnectionSetLinkSupervisionTimeout(a2dp_sink, 0x3f00);
	
	POLICY_DEBUG(("LINK POLICY : Set A2DP power table for sink 0x%x\n",(uint16)a2dp_sink));
}


/*****************************************************************************/
void linkPolicyHfpStateChange(bool hfpConnected, bool scoStateChange)
{
	hsTaskData * app = (hsTaskData *) getAppTask() ;
	bdaddr hfp_addr, a2dp_addr, avrcp_addr;
#ifdef SINPUNG_DONGLE /* For Sinpung Dongle v110422 */
    bdaddr intercom_addr;
#endif
	
	bool a2dp_connected = (stateManagerGetA2dpState() != headsetA2dpConnectable)?TRUE:FALSE;
	Sink a2dp_sink = A2dpGetSignallingSink(app->a2dp);
	Sink hfp_sink = HfpGetSlcSink(app->hfp_hsp);
    Sink intercom_sink = HfpGetSlcSink(app->intercom_hsp);
	
	if (hfpConnected)
	{
		/* HFP has connected or SCO connection has changed state. */

#ifdef S100A /* For intercom slave mode current down. v110422 */
#ifdef SINPUNG_DONGLE /* For Sinpung Dongle v110422 */
        if (SinkGetBdAddr(intercom_sink, &intercom_addr) && (intercom_addr.nap == 0x15 && intercom_addr.uap == 0x45))
        {
            return;
        }

        if (!SinkGetBdAddr(hfp_sink, &hfp_addr) && !intercom_sink)
#else
        if (!SinkGetBdAddr(hfp_sink, &hfp_addr) && !SinkGetBdAddr(intercom_sink, &hfp_addr))
#endif
#else
        if (!SinkGetBdAddr(hfp_sink, &hfp_addr))
#endif
        {
            return;
        }
        
		/* Get bdaddr of both A2DP and HFP connections and see if they are to the same device */
		if (a2dp_connected && SinkGetBdAddr(a2dp_sink, &a2dp_addr))
		{
#ifdef S100A /* For intercom slave mode current down. v110422 */
#ifdef SINPUNG_DONGLE /* For Sinpung Dongle v110422 */
            if (intercom_addr.nap == 0x15 && intercom_addr.uap == 0x45)
            {
                return;
            }

            if (BdaddrIsSame(&hfp_addr, &a2dp_addr) && !intercom_sink)
#else
            if (BdaddrIsSame(&hfp_addr, &a2dp_addr) && !intercom_sink)
#endif
#else
            if (BdaddrIsSame(&hfp_addr, &a2dp_addr))
#endif
			{
                /* A2DP is connected to same device, so decide which policy should win. */
                /* For now assume A2DP always wins over HFP. */
                return;
			}

			if (!scoStateChange)
			{
				/* Both A2DP and HFP are now connected. Set headest to be Master of 
					A2DP and HFP links as they are with different devices. */
				ConnectionSetRole(&app->task, a2dp_sink, hci_role_master);
				ConnectionSetRole(&app->task, hfp_sink, hci_role_master);
                ConnectionSetRole(&app->task, intercom_sink, hci_role_master);
			}
		}

		setHfpLinkPolicy(app);
	}
	else
	{
		/* HFP has disconnected. */
		
		/* Assume A2DP always wins over HFP so nothing to do on A2DP link. */
		
		/* See if AVRCP is connected */
		if (stateManagerGetAvrcpState() == avrcpConnected)
		{
			if (!SinkGetBdAddr(AvrcpGetSink(app->avrcp), &avrcp_addr))
            {
                return;
            }

			/* Check A2DP and see if connections are to the same device */
			if (a2dp_connected && SinkGetBdAddr(a2dp_sink, &a2dp_addr))
			{
				if (BdaddrIsSame(&avrcp_addr, &a2dp_addr))
				{
					/* A2DP always wins over AVRCP. */
					return;
				}
			}

			setAvrcpLinkPolicy(app);
		}							
	}
}


/*****************************************************************************/
void linkPolicyA2dpStateChange(bool a2dpConnected)
{
	hsTaskData * app = (hsTaskData *) getAppTask() ;
	bdaddr hfp_addr, avrcp_addr;
	bool hfp_connected = (stateManagerGetHfpState() >= headsetHfpConnected)?TRUE:FALSE;
	Sink hfp_sink = HfpGetSlcSink(app->hfp_hsp);
    Sink intercom_sink = HfpGetSlcSink(app->intercom_hsp);

	if (a2dpConnected)
	{
		/* A2DP has connected, use this power table */
		setA2dpLinkPolicy(app);
		
		if (hfp_connected && !IsA2dpSourceAnAg(app))
		{
			/* Both A2DP and HFP are now connected. Set headest to be Master of 
				A2DP and HFP links as they are with different devices. */
			ConnectionSetRole(&app->task, A2dpGetSignallingSink(app->a2dp), hci_role_master);
			ConnectionSetRole(&app->task, hfp_sink, hci_role_master);
            ConnectionSetRole(&app->task, intercom_sink, hci_role_master);
		}
	}
	else
	{
		/* A2DP has disconnected. */
	
		bool avrcp_connected = (stateManagerGetAvrcpState() == avrcpConnected)?TRUE:FALSE;
		
		if (hfp_connected)
		{
			/* Update link to use HFP power table */
			setHfpLinkPolicy(app);
		}
		if (avrcp_connected)
		{
			if (!SinkGetBdAddr(AvrcpGetSink(app->avrcp), &avrcp_addr))
            {
                return;
            }

			if (hfp_connected && SinkGetBdAddr(hfp_sink, &hfp_addr))
			{
				if (BdaddrIsSame(&avrcp_addr, &hfp_addr))
				{
					/* HFP always wins over AVRCP. */
					return;
				}
			}

			setAvrcpLinkPolicy(app);
		}							
	}
		
}


/*****************************************************************************/
void linkPolicyAvrcpStateChange(bool avrcpConnected)
{
	hsTaskData * app = (hsTaskData *) getAppTask() ;
	bdaddr hfp_addr, avrcp_addr, a2dp_addr;
	
	if (avrcpConnected)
	{
		/* AVRCP has connected. */
		
		bool hfp_connected = (stateManagerGetHfpState() >= headsetHfpConnected)?TRUE:FALSE;
		bool a2dp_connected = (stateManagerGetA2dpState() != headsetA2dpConnectable)?TRUE:FALSE;
		
		if (!SinkGetBdAddr(AvrcpGetSink(app->avrcp), &avrcp_addr))
        {
            return;
        }

		if (hfp_connected && SinkGetBdAddr(HfpGetSlcSink(app->hfp_hsp), &hfp_addr))
		{
			if (BdaddrIsSame(&avrcp_addr, &hfp_addr))
			{
				/* HFP always wins over AVRCP. */
				return;
			}
		}
		if (a2dp_connected && SinkGetBdAddr(A2dpGetSignallingSink(app->a2dp), &a2dp_addr))
		{
			if (BdaddrIsSame(&avrcp_addr, &a2dp_addr))
			{
				/* A2DP always wins over AVRCP. */
				return;
			}
		}

		setAvrcpLinkPolicy(app);
	}
}


