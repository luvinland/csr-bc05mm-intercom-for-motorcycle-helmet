/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_csr_features.h      
@brief    handles the csr to csr features 
*/

#ifndef _CSR2CSRFEATURES_
#define _CSR2CSRFEATURES_

#include <hfp.h>

#include "headset_private.h"

void csr2csrEnable(HFP * hfp);

void csr2csrHandleSupportedFeaturesCfm(HFP_CSR_SUPPORTED_FEATURES_CFM_T * cfm);

void csr2csrHandleTxtInd(void);
void csr2csrHandleModifyIndicatorsCfm(void);
void csr2csrHandleSmsInd(void);   
void csr2csrHandleSmsNameInd(void);
void csr2csrHandleSmsCfm(void);
void csr2csrHandleModifyAgIndicatorsInd(void);
void csr2csrHandleAgIndicatorsDisableInd(void);
void csr2csrHandleAgBatteryRequestInd(void);
#if 0 /* Jace_Test */
void csr2csrFeatureNegotiationInd(hsTaskData * pApp,  HFP_CSR_FEATURE_NEGOTIATION_IND_T * ind);
#endif
/* Supported Indicators */

#define CSR_IND_CODEC 0x06
    
#endif /* _CSR2CSRFEATURES_ */
