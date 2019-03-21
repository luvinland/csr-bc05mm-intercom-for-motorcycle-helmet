/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_pio.c
@brief  Implementation of PIO control functions.
*/


#include "headset_debug.h"
#include "headset_pio.h"
#include "headset_private.h"

#include <pio.h>


#ifdef DEBUG_PIO
#define PIO_DEBUG(x)  DEBUG (x)
#else
#define PIO_DEBUG(x) 
#endif

#ifdef DEBUG_DIM
#define DIM_DEBUG(x) DEBUG(x)
#else
#define DIM_DEBUG(x) 
#endif

#define DIM_NUM_STEPS (0xf)
#define DIM_STEP_SIZE ((4096) / (DIM_NUM_STEPS + 1) ) 
#define DIM_PERIOD    (0x0)


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/

static void PioSetLed ( LedTaskData * pLedTask , uint16 pPIO , bool pOnOrOff ) ;


/****************************************************************************
    FUNCTIONS
*/


/*****************************************************************************/
void PioSetLedPin ( LedTaskData * pLedTask , uint16 pPIO , bool pOnOrOff ) 
{
	 /*handle tricolour LEDS first */
	/*
	switch (pPIO)
	{		
		case (11):
		{	
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_a, pOnOrOff) ;
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_b, pOnOrOff) ;		
		}
		break ;
		case (12):
		{
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_b, pOnOrOff) ;
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_c, pOnOrOff) ;		
		}
		break ;
		case (13) :
		{
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_c, pOnOrOff) ;
			PioSetLed (pLedTask , pLedTask->gTriColLeds.TriCol_a, pOnOrOff) ;	
		}
		break ;			
		default:
		{	
			PioSetLed (pLedTask , pPIO , pOnOrOff) ;
		}	
		break ;
	}
	*/
	
	/* a single LED pin to update */
	PioSetLed (pLedTask , pPIO , pOnOrOff) ;
}	


/*****************************************************************************/
void PioSetPio ( uint16 pPIO , bool pOnOrOff  ) 
{
    uint16 lPinVals = 0 ;
    uint16 lWhichPin  = (1<< pPIO) ;
    
    PIO_DEBUG(("PIO : set[%d][%d] [%x]\n",pPIO, pOnOrOff ,lWhichPin)) ;
    
    if ( pOnOrOff )    
    {
        lPinVals = lWhichPin  ;
    }
    else
    {
        lPinVals = 0x0000;/*clr the corresponding bit*/
    }
    
    PIO_DEBUG(("PIO : set[%x][%x]\n",lWhichPin , lPinVals)) ;
      	/*(mask,bits) setting bit to a '1' sets the corresponding port as an output*/
    PioSetDir( lWhichPin , lWhichPin );   
    	/*set the value of the pin*/         
    PioSet ( lWhichPin , lPinVals ) ;     
}


/*****************************************************************************/
void PioSetDimState ( LedTaskData * pLedTask , uint16 pPIO )
{
    uint16 lDim = 0x0000 ;
    
    if (pLedTask->gActiveLEDS[pPIO].DimDir) 
    {
        /*going up*/
        if ( pLedTask->gActiveLEDS[pPIO].DimState >= DIM_NUM_STEPS )
        {       /*this is the final level - set led 0*/
            lDim = 0xFFF;
            DIM_DEBUG(("DIM:+[F] [ON]\n" ));
        }
        else
        {         
            pLedTask->gActiveLEDS[pPIO].DimState++ ;
            lDim = (pLedTask->gActiveLEDS[pPIO].DimState * (DIM_STEP_SIZE) ) ;
            DIM_DEBUG(("DIM:+[%d] [%d] [%d]\n" , pLedTask->gActiveLEDS[pPIO].DimState , (pLedTask->gActiveLEDS[pPIO].DimState * DIM_STEP_SIZE) , pPIO)) ;
            MessageCancelAll ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ) ;                
            MessageSendLater ( &pLedTask->task, (DIM_MSG_BASE + pPIO) , 0 , pLedTask->gActiveLEDS[pPIO].DimTime ) ;    
        }
    }
    else
    {
        /*we are going down*/
        if ( pLedTask->gActiveLEDS[pPIO].DimState == 0x0 )
        {    /*this is the final level - set led 0*/
            lDim = 0 ;
            DIM_DEBUG(("DIM:-[0] [OFF]\n" ));
        }
        else
        {         
            pLedTask->gActiveLEDS[pPIO].DimState-- ;
            lDim = (pLedTask->gActiveLEDS[pPIO].DimState * (DIM_STEP_SIZE)) ;
            DIM_DEBUG(("DIM:-[%d] [%d] [%d]\n" , pLedTask->gActiveLEDS[pPIO].DimState , (pLedTask->gActiveLEDS[pPIO].DimState * DIM_STEP_SIZE) ,pPIO )) ;
            MessageCancelAll ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ) ;
            MessageSendLater ( &pLedTask->task, (DIM_MSG_BASE + pPIO) , 0 , pLedTask->gActiveLEDS[pPIO].DimTime ) ;    
        }
    }    

#ifndef NO_CHARGER_TRAPS    
    if (pPIO == 14)
    {
        PioDimLed0 ( lDim , DIM_PERIOD ) ;  
        PioSetLed0 ( TRUE ) ;
           
    }
    else if (pPIO ==15)
    {
        PioDimLed1 ( lDim , DIM_PERIOD ) ;  
        PioSetLed1 ( TRUE ) ;
    }
#endif
    
}


/****************************************************************************
NAME	
	PioSetLed

DESCRIPTION
   Internal fn to change set an LED attached to a PIO or a special LED pin. 
    
*/
static void PioSetLed ( LedTaskData * pLedTask , uint16 pPIO , bool pOnOrOff ) 
{	
   /* LED pins are special cases*/
    if ( pPIO == 14)        
    {
        if ( pLedTask->gActiveLEDS[pPIO].DimTime > 0 ) /*if this is a dimming led / pattern*/ 
        {
            if (pLedTask->gLED_0_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                    /*set led to max or min depending on whether we think the led is on or off*/
                pLedTask->gActiveLEDS[pPIO].DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                pLedTask->gActiveLEDS[pPIO].DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 
                        
#ifndef NO_CHARGER_TRAPS
                if ( PioDimLed0 ( (pLedTask->gActiveLEDS[pPIO].DimState * (DIM_STEP_SIZE) ) , DIM_PERIOD ) )
                {
                    DIM_DEBUG(("DIM: Set LED [%d][%x][%d]\n" ,pPIO ,pLedTask->gActiveLEDS[pPIO].DimState , pLedTask->gActiveLEDS[pPIO].DimDir  )) ;
                    PioSetLed0 ( TRUE ) ;
                        /*send the first message*/
                    MessageCancelAll ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ) ;                
                    MessageSendLater ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ,0 ,pLedTask->gActiveLEDS[pPIO].DimTime ) ;
                }        
                else    /*if the Dim requst fails, then we must use to standard calls*/
                {
                    
                    DIM_DEBUG(("DIM !0\n")) ;
                    PioSetLed0 ( pOnOrOff ) ;
                }
#endif                
                pLedTask->gLED_0_STATE = pOnOrOff ;
            }
        }else
        {              
#ifndef NO_CHARGER_TRAPS
            DIM_DEBUG(("DIM 0 N:[%d]\n" , pOnOrOff)) ;
		    PioSetLed0 ( pOnOrOff ) ;
            PioDimLed0 ( (0xfff ) , DIM_PERIOD ) ;
            pLedTask->gLED_0_STATE = pOnOrOff ;
#endif
        }
    }
    else if (pPIO == 15 )
    {
        if ( pLedTask->gActiveLEDS[pPIO].DimTime > 0 ) /*if this is a dimming led / pattern*/ 
        {
            if (pLedTask->gLED_1_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                   /*set led to max or min depending on whether we think the led is on or off*/
                pLedTask->gActiveLEDS[pPIO].DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                pLedTask->gActiveLEDS[pPIO].DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 
                                    
#ifndef NO_CHARGER_TRAPS
                if ( PioDimLed1 ( (pLedTask->gActiveLEDS[pPIO].DimState * (DIM_STEP_SIZE)) , DIM_PERIOD ) )
                {
                    DIM_DEBUG(("DIM: Set LED [%d][%x][%d]\n" ,pPIO ,pLedTask->gActiveLEDS[pPIO].DimState , pLedTask->gActiveLEDS[pPIO].DimDir  )) ;
                    PioSetLed1 ( TRUE ) ;
                       /*send the first message*/
                    MessageCancelAll ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ) ;                
                    MessageSendLater ( &pLedTask->task, (DIM_MSG_BASE + pPIO) ,0 ,pLedTask->gActiveLEDS[pPIO].DimTime ) ;
                }        
                else    /*if the Dim request fails, then we must use to standard calls*/
                {
                    DIM_DEBUG(("DIM !1\n")) ;
                    PioSetLed1 ( pOnOrOff ) ;
                }    
#endif
                pLedTask->gLED_1_STATE = pOnOrOff ;                                  
            }
        }else
        {
#ifndef NO_CHARGER_TRAPS
            DIM_DEBUG(("DIM 1 N:[%d]\n" , pOnOrOff)) ;
            PioSetLed1 ( pOnOrOff ) ;
            PioDimLed1 ( (0xfff ) , DIM_PERIOD ) ;
            pLedTask->gLED_1_STATE = pOnOrOff ;
#endif
        }
    }
    else
    {
        PioSetPio (pPIO , pOnOrOff) ;
    }
}













