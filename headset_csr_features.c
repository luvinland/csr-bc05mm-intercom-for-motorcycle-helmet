/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2007-2008
*/

/*!
@file    headset_csr_features.c
@brief    handles the csr to csr features 
*/

#include "headset_csr_features.h"
#include "headset_debug.h"

#include "headset_private.h"
#include "panic.h"
#include "vm.h"

/* Header files */
#include <stdlib.h>


#include <app/bluestack/types.h>
#include <app/bluestack/bluetooth.h>
#include <app/bluestack/hci.h>
#include <app/bluestack/dm_prim.h>

#include <vm.h>
#include <panic.h>


#ifdef DEBUG_CSR2CSR
    #define CSR2CSR_DEBUG(x) DEBUG(x)
#else
    #define CSR2CSR_DEBUG(x)
#endif     


void csr2csrEnable( HFP * hfp) 
{
    CSR2CSR_DEBUG(("ENABLE CSR2CSR\n"));
    
	/*enable the features on connection*/
    HfpCsrSupportedFeaturesReq(hfp, TRUE, TRUE, TRUE, TRUE, TRUE, audio_codec_cvsd|audio_codec_auristream_2_bit|audio_codec_auristream_4_bit);
}

void csr2csrHandleSupportedFeaturesCfm( HFP_CSR_SUPPORTED_FEATURES_CFM_T * cfm)
{
    CSR2CSR_DEBUG(("CSR2CSR SUPP FEATS  [%d]..%d.%d.%d.%d.%d\n ", cfm->status,
                                                                      cfm->callerName,
                                                                      cfm->rawText,
                                                                      cfm->smsInd,
                                                                      cfm->battLevel,
                                                                      cfm->pwrSource));
}

void csr2csrHandleTxtInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}

void csr2csrHandleModifyIndicatorsCfm(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}

void csr2csrHandleSmsInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}

void csr2csrHandleSmsNameInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}

void csr2csrHandleSmsCfm(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}
void csr2csrHandleModifyAgIndicatorsInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}
void csr2csrHandleAgIndicatorsDisableInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}
void csr2csrHandleAgBatteryRequestInd(void) 
{
    CSR2CSR_DEBUG(("CSR2CSR \n"));
}



#if 0 /* Jace_Test */
void csr2csrFeatureNegotiationInd(hsTaskData * pApp,  HFP_CSR_FEATURE_NEGOTIATION_IND_T * ind)  
{
    CSR2CSR_DEBUG(("CSR2CSR FEAT NEG IND[%d],[%d]\n", ind->indicator, ind->value));
    
    if(ind->indicator == CSR_IND_CODEC)
    {
		/*store the selected audio codec*/
		pApp->SCO_codec_selected = ind->value;     
		
		/*respond to the ag with the agreed codec*/
		hfpFeatureNegotiationResponse( ind->hfp, CSR_IND_CODEC, ind->value);

		/* Enable Auristream Voice Settings with the firmware */    
	    if((ind->value == audio_codec_auristream_2_bit) ||(ind->value == audio_codec_auristream_4_bit))
	    {
	        DM_HCI_WRITE_VOICE_SETTING_T *prim = PanicUnlessNew(DM_HCI_WRITE_VOICE_SETTING_T); 
	        
	        prim->common.op_code = DM_HCI_WRITE_VOICE_SETTING; 
	        prim->common.length = sizeof(DM_HCI_WRITE_VOICE_SETTING_T); 
	        
	        prim->voice_setting = 0x63;
	
	        VmSendDmPrim(prim);
	    }
    }
}
#endif
   
