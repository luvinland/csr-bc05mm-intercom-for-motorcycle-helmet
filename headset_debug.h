/***************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_debug.h
@brief   Interface to control output of debug from the headset application. 
*/

#ifndef _HEADSET_DEBUG_H_
#define _HEADSET_DEBUG_H_

    
#ifdef DEBUG_PRINT_ENABLED
#include <stdio.h>
#define DEBUG(x) {printf x;}

/*The individual Debug enables*/

/*The main system messages*/
#define DEBUG_MAINx
/* The audio amp messages*/
#define DEBUG_AMPx
/* The a2dp connection messages*/
#define DEBUG_A2DP_CONNECTIONx
/*The a2dp library messages*/
#define DEBUG_A2DP_MSGx
/*The stream control messages*/
#define DEBUG_A2DP_STREAM_CONTROLx
/*The avrcp event handling*/
#define DEBUG_AVRCP_EVENTx
/*The authorisation handling*/
#define DEBUG_AUTHx
/*The avrcp library messages*/
#define DEBUG_AVRCP_MSGx
/*The battery handling*/
#define DEBUG_BATx
/*The button manager */
#define DEBUG_BUT_MANx
/*The low level button parsing*/
#define DEBUG_BUTTONSx
/*The charger*/
#define DEBUG_CHARGERx
/*The connection library messages*/
#define DEBUG_CL_MSGx
/*The codec library messages*/
#define DEBUG_CODEC_MSGx
/*The config manager */
#define DEBUG_CONFIGx
/* The events received */
#define DEBUG_EVENTSx
/*The hfp handling*/
#define DEBUG_HFPx
/*The call functionality*/
#define DEBUG_HFP_CALLx
/*The hfp library messages*/
#define DEBUG_HFP_MSGx
/*The hfp slc handling*/
#define DEBUG_HFP_SLCx
/*The Init handling*/
#define DEBUG_INITx
/*The LED manager */
#define DEBUG_LMx
/*The Lower level LED drive */
#define DEBUG_LEDSx
/*The Link policy messages */
#define DEBUG_LINK_POLICYx
/*The Lower lvel PIO drive*/
#define DEBUG_PIOx
/*The power manager*/
#define DEBUG_POWERx
/*Scan manager*/
#define DEBUG_SCANx
/*State manager*/
#define DEBUG_STATESx
/*Tone manager*/
#define DEBUG_TONESx
/*Volume manager*/
#define DEBUG_VOLUMEx
/* CSR 2 CSR Extensions */
#define DEBUG_CSR2CSRx
/* Insert code for Intercom by Jace */
#define DEBUG_INTERCOM_MSGx
#else

#define DEBUG(x) 

#endif /* DEBUG_PRINT_ENABLED */


#ifdef DEBUG_PROFILE_ENABLED 
#include <stdio.h>
#include <vm.h>

/* NOTE : PROFILE_TIME Will wrap the time value */
#define PROFILE_TIME(x) {printf(x); printf(" time : %d\n", (uint16)VmGetClock());}
/* NOTE : PROFILE_TIME_SLOW is slower than PROFILE_TIME */
#define PROFILE_TIME_SLOW(x) {uint32 prof_time = VmGetClock(); printf(x); printf(" time : 0x%X, %X\n", (uint16)(prof_time>>16), (uint16)(prof_time % 0xffff));}
#define PROFILE_MEMORY(x) {printf(x); printf(" slots : %d\n", VmGetAvailableAllocations());}

#else

#define PROFILE_TIME(x)
#define PROFILE_TIME_SLOW(x)
#define PROFILE_MEMORY(x)

#endif /* DEBUG_PROFILE_ENABLED  */

#endif /* _HEADSET_DEBUG_H */

