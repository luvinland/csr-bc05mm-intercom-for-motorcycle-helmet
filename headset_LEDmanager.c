/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_LEDmanager.c
@brief    Module responsible for managing the PIO outputs including LEDs.
*/


#include "headset_configmanager.h"
#include "headset_debug.h"
#include "headset_statemanager.h"
#include "headset_LEDmanager.h"
#include "headset_leds.h"
#include "headset_pio.h"
#include "headset_private.h"

#include <stddef.h>
#include <panic.h>
#include <pio.h>

#ifdef DEBUG_LM
#define LM_DEBUG(x) DEBUG(x)
#else
#define LM_DEBUG(x) 
#endif


/****************************************************************************
    LOCAL FUNCTION PROTOTYPES
*/

 /*internal method to provide a pointer to one of the malloced patterns*/
static LEDPattern_t * LMGetPattern ( LedTaskData * ptheLEDTask )  ;
    /*method to release a pattern - actually clears data held in pattern so it can be used again*/
static void LMResetPattern ( LEDPattern_t * pPattern ) ;

static bool LMIsPatternEmpty (LEDPattern_t * pPattern ) ;

static LEDPattern_t *  LMAddPattern ( LedTaskData * ptheLEDTask , LEDPattern_t * pSourcePattern , LEDPattern_t * pDestPattern ) ;

 /*methods to allocate/ initialise the space for the patterns and mappings*/
static void LEDManagerInitStatePatterns   ( LedTaskData * ptheLEDTask ) ;
static void LEDManagerInitEventPatterns   ( LedTaskData * ptheLEDTask ) ;
static void LEDManagerInitActiveLEDS      ( LedTaskData * ptheLEDTask ) ;
static void LEDManagerCreateFilterPatterns( LedTaskData * ptheLEDTask ) ;


/****************************************************************************
  FUNCTIONS
*/

/*****************************************************************************/
void LEDManagerInit ( LedTaskData * ptheLEDTask ) 
{
    uint16 lIndex = 0 ;
    uint16 lSize = 0;
    uint16 *buffer;
    uint16 pos;
        
    LM_DEBUG(("LM Init :\n")) ;
   
	/*allocate the space for all of the patterns*/
	/* Place LED Patterns and Active LEDs in 1 allocation to minimise slot usage */
	lSize = (sizeof(LEDPattern_t) * LM_MAX_NUM_PATTERNS) + (sizeof(LEDActivity_t) * HEADSET_NUM_LEDS);
	buffer = PanicUnlessMalloc(lSize);
    ptheLEDTask->gPatterns = (LEDPattern_t*)&buffer[0];
    
    pos = (sizeof(LEDPattern_t) * LM_MAX_NUM_PATTERNS);
    ptheLEDTask->gActiveLEDS = (LEDActivity_t *)&buffer[pos];
    
    for (lIndex = 0 ; lIndex < LM_MAX_NUM_PATTERNS ; lIndex ++ )
    {
            /*make sure the pattern is released and ready for use*/
        LMResetPattern ( &ptheLEDTask->gPatterns[lIndex] )  ;      
    }
    
    /*malloc the space for all of the other data*/
    lSize =   ( (sizeof(LEDPattern_t *)) * HEADSET_NUM_HFP_STATES * HEADSET_NUM_A2DP_STATES )
            + ( (sizeof(LEDPattern_t *)) * EVENTS_MAX_EVENTS   ) ; 
        
    ptheLEDTask->gStatePatterns = (LEDPattern_t * * ) PanicUnlessMalloc (lSize) ;
     
    ptheLEDTask->gEventPatterns = (LEDPattern_t * *) (ptheLEDTask->gStatePatterns + (HEADSET_NUM_HFP_STATES * HEADSET_NUM_A2DP_STATES) ) ;
    
    LM_DEBUG(("LM : p[%x][%x][%x]\n" ,  (int)ptheLEDTask->gStatePatterns ,
                                        (int)ptheLEDTask->gEventPatterns ,
                                        (int)ptheLEDTask->gActiveLEDS    
              )) ;
    /* create the patterns we want to use*/
    LEDManagerInitStatePatterns ( ptheLEDTask) ;
    
    LEDManagerInitActiveLEDS( ptheLEDTask) ;
    
    LEDManagerInitEventPatterns( ptheLEDTask ) ;
    
    ptheLEDTask->Queue.Event1 = 0 ;
    ptheLEDTask->Queue.Event2 = 0 ;
    ptheLEDTask->Queue.Event3 = 0 ;
    ptheLEDTask->Queue.Event4 = 0 ;
    
	/*the filter information*/
    LEDManagerCreateFilterPatterns( ptheLEDTask ) ;
    
    LedsInit ( ptheLEDTask ) ;

}


/*****************************************************************************/
void LEDManagerAddLEDStatePattern ( LedTaskData * ptheLEDTask , headsetHfpState pState, headsetA2dpState pA2dpState , LEDPattern_t* pPattern )
{  
    uint16 led_state =  pState + (HEADSET_NUM_HFP_STATES * pA2dpState);
    ptheLEDTask->gStatePatterns [ led_state ] = LMAddPattern ( ptheLEDTask , pPattern ,  ptheLEDTask->gStatePatterns [ led_state ] )  ;
    LM_DEBUG(("LM: AddState[%x][%x][%x]\n" , pState , pA2dpState ,(int) ptheLEDTask->gStatePatterns [ led_state ] )) ;
}


/*****************************************************************************/
void LEDManagerAddLEDFilter  (  LedTaskData * ptheLEDTask , LEDFilter_t* pLedFilter ) 
{
    if ( ptheLEDTask->gLMNumFiltersUsed < LM_NUM_FILTER_EVENTS )
    {
        /*then add the filter pattern*/
       ptheLEDTask->gEventFilters [ ptheLEDTask->gLMNumFiltersUsed ] = *pLedFilter ;
  
       LM_DEBUG(("LM :  AF[%x][%d][%d][%d][%d] [%d][%d] [%d][%d]\n", pLedFilter->Event ,pLedFilter->IsFilterActive , 
                                                   pLedFilter->Speed, pLedFilter->SpeedAction, pLedFilter->Colour ,
                                                   pLedFilter->OverideLEDActive , pLedFilter->OverideLED , 
                                                   pLedFilter->FollowerLEDActive , pLedFilter->FollowerLEDDelay
                                                   )) ;
     /*inc the filters*/
        ptheLEDTask->gLMNumFiltersUsed ++ ;
    }
}


/*****************************************************************************/
void LEDManagerAddLEDEventPattern ( LedTaskData * ptheLEDTask , headsetEvents_t pEvent , LEDPattern_t* pPattern )
{
    uint16 lIndex = pEvent - EVENTS_EVENT_BASE ;

    ptheLEDTask->gEventPatterns [ lIndex ] = LMAddPattern ( ptheLEDTask , pPattern , ptheLEDTask->gEventPatterns [ lIndex ]  ) ;   
    
    LM_DEBUG(("LM: AddEvent[%x] [%x]\n" , pEvent ,(int)ptheLEDTask->gEventPatterns [ lIndex ])) ;    

}


/*****************************************************************************/
void LEDManagerSetMicBias ( hsTaskData * pApp , bool pEnable  ) 
{
    /* turn on and off dedicated pin and set current and voltage */
    if ( pEnable)
    {
        /* set current and voltage to magic values*/
        PioSetMicBiasHwEnabled (1);
        PioSetMicBiasHwCurrent(11);   
        PioSetMicBiasHwVoltage(7);
    }
    else
    {
        PioSetMicBiasHwEnabled (0);
    }
	
    LM_DEBUG(("LM: Mic e[%c]\n" , (pEnable ? 'T':'F' ))) ;
}


/*****************************************************************************/
void LEDManagerIndicateEvent ( LedTaskData * pLEDTask , MessageId pEvent ) 
{
    uint16 lEventIndex = pEvent - EVENTS_EVENT_BASE ;

    /*only indicate if LEDs are enabled*/
    if ((pLEDTask->gLEDSEnabled ) ||
        LedsEventCanOverideDisable( pLEDTask , pEvent ) ||
        LedActiveFiltersCanOverideDisable( pLEDTask ))
    {
    
        LM_DEBUG(("LM : IE[%x]\n",pEvent )) ;
            /*if there is an event configured*/
        if ( pLEDTask->gEventPatterns [lEventIndex] != NULL )
        {
                /*only update if wer are not currently indicating an event*/
            if ( ! pLEDTask->gCurrentlyIndicatingEvent )
            {
                LedsIndicateEvent ( pLEDTask , pEvent ) ;  
            }    
            else
            {
            
                /*hsTaskData * lApp = (hsTaskData *) getAppTask() ;*/

                if (1)/*lApp->features.QueueLEDEvents )*/
                {
             
                        /*try and add it to the queue*/
                    LM_DEBUG(("LM: Queue LED Event [%x]\n" , pEvent )) ;
                
                    if ( pLEDTask->Queue.Event1 == 0)
                    {
                        pLEDTask->Queue.Event1 = ( pEvent - EVENTS_EVENT_BASE) ;
                    }
                    else if ( pLEDTask->Queue.Event2 == 0)
                    {
                        pLEDTask->Queue.Event2 = ( pEvent - EVENTS_EVENT_BASE) ;
                    }
                    else if ( pLEDTask->Queue.Event3 == 0)
                    {
                        pLEDTask->Queue.Event3 = ( pEvent - EVENTS_EVENT_BASE) ;
                    }
                    else if ( pLEDTask->Queue.Event4 == 0)
                    {
                        pLEDTask->Queue.Event4 = ( pEvent - EVENTS_EVENT_BASE) ;
                    }    
                    else
                    {
                        LM_DEBUG(("LM: Err Queue Full!!\n")) ;
                    }
                }    
            }
        }
        else
        {
            LM_DEBUG(("LM: NoEvPatCfg\n")) ;
        }  
        }
    else
    {
        LM_DEBUG(("LM : No IE[%x] disabled\n",pEvent )) ;
    }
    
    /*indicate a filter if there is one present*/
    LedsCheckForFilter ( pLEDTask , pEvent ) ;
}


/*****************************************************************************/
void LEDManagerIndicateState ( LedTaskData * pLEDTask , headsetHfpState pState , headsetA2dpState pA2dpState )  
{   
    uint16 led_state = pState + (HEADSET_NUM_HFP_STATES * pA2dpState);
    
    /* only indicate if LEDs are enabled*/
    if ((pLEDTask->gLEDSEnabled ) ||
        LedsStateCanOverideDisable( pLEDTask , pState , pA2dpState ) ||
        LedActiveFiltersCanOverideDisable( pLEDTask ))
    {
            /*if there is no pattern associated with the sate then do nothing*/
        if ( pLEDTask->gStatePatterns[ led_state ] == NULL )
        {
            LM_DEBUG(("LM: NoStCfg[%x][%x]\n",pState, pA2dpState)) ;
            LedsIndicateNoState ( pLEDTask ) ;
        }
        else
        {
            LM_DEBUG(("LM : IS[%x][%x]\n", pState, pA2dpState)) ;
            LedsIndicateState ( pLEDTask , pState, pA2dpState );
        } 
    }
    else
    {
        LM_DEBUG(("LM : DIS NoStCfg[%x]\n", pState)) ;
        LedsIndicateNoState ( pLEDTask );
    }
}


/*****************************************************************************/
void LedManagerEnableLEDS ( LedTaskData * pTask )
{
    LM_DEBUG(("LM Enable LEDS\n")) ;
    
    pTask->gLEDSEnabled = TRUE ;
         
    LEDManagerIndicateState ( pTask , stateManagerGetHfpState () , stateManagerGetA2dpState () ) ;    
}


/*****************************************************************************/
void LedManagerResetLEDIndications ( LedTaskData * pTask )
{    
    LedsResetAllLeds ( pTask ) ;
    
    pTask->gCurrentlyIndicatingEvent = FALSE ;
    
    LEDManagerIndicateState (pTask , stateManagerGetHfpState() , stateManagerGetA2dpState () ) ;
}


/*****************************************************************************/
void LEDManagerResetStateIndNumRepeatsComplete  ( LedTaskData * pTask ) 
{
    /*get state*/
    uint16 led_state = stateManagerGetCombinedState();
    /*get pattern*/ 
    LEDPattern_t * lPattern = pTask->gStatePatterns[led_state] ;

    if (lPattern)
    {
        LEDActivity_t * lLED   = &pTask->gActiveLEDS[lPattern->LED_A] ;
        if (lLED)
        {
            /*reset num repeats complete to 0*/
            lLED->NumRepeatsComplete = 0 ;
        }    
    }
}


/*****************************************************************************/
#ifdef DEBUG_LM
void LMPrintPattern ( LEDPattern_t * pLED ) 
{
    const char * const lColStrings [ 5 ] =   {"LED_E ","LED_A","LED_B","ALT","Both"} ;

    LM_DEBUG(("[%d][%d] [%d][%d][%d] ", pLED->LED_A , pLED->LED_B, pLED->OnTime, pLED->OffTime, pLED->RepeatTime)) ;  
    LM_DEBUG(("[%d] [%d] [%s]\n",       pLED->NumFlashes, pLED->TimeOut, lColStrings[pLED->Colour])) ;    
    LM_DEBUG(("[%d]\n",       pLED->OverideDisable)) ;    
}
#endif


/****************************************************************************
NAME 
    LEDManagerInitActiveLEDS

DESCRIPTION
    Creates the active LED space for the number of leds the system supports.

*/
static void LEDManagerInitActiveLEDS ( LedTaskData * ptheLEDTask ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < HEADSET_NUM_LEDS ; lIndex ++ )
    {
        LedsSetLedActivity ( &ptheLEDTask->gActiveLEDS [ lIndex ] , IT_Undefined , 0 , 0 ) ;    
    }
}


/****************************************************************************
NAME 
    LEDManagerInitStatePatterns

DESCRIPTION
    Creates the state patterns space for the system states.
    
*/
static void LEDManagerInitStatePatterns ( LedTaskData * ptheLEDTask ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < (HEADSET_NUM_HFP_STATES * HEADSET_NUM_A2DP_STATES); lIndex ++ )
    {
        ptheLEDTask->gStatePatterns [ lIndex ] = NULL ;     
    }
}


/****************************************************************************
NAME 
    LEDManagerInitEventPatterns

DESCRIPTION
    Inits the Event pattern pointers.

*/
static void LEDManagerInitEventPatterns ( LedTaskData * ptheLEDTask )
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < EVENTS_MAX_EVENTS ; lIndex ++ )
    {
       ptheLEDTask->gEventPatterns [ lIndex ] = NULL ;     
    } 
}


/****************************************************************************
NAME 
    LEDManagerCreateFilterPatterns

DESCRIPTION
    Creates the Filter patterns space.
    
*/
static void LEDManagerCreateFilterPatterns ( LedTaskData * ptheLEDTask )
{
    uint16 lIndex = 0 ;
    /*create the space for the filter patterns*/
    
    ptheLEDTask->gEventFilters = (LEDFilter_t *) PanicUnlessMalloc ( sizeof (LEDFilter_t ) * LM_NUM_FILTER_EVENTS ) ;
    
    for (lIndex = 0 ; lIndex < LM_NUM_FILTER_EVENTS ; lIndex++ )
    {
        ptheLEDTask->gEventFilters [ lIndex ].Event             = 0 ;
        ptheLEDTask->gEventFilters [ lIndex ].IsFilterActive    = FALSE ;      
        ptheLEDTask->gEventFilters [ lIndex ].Speed             = 0 ;
        ptheLEDTask->gEventFilters [ lIndex ].SpeedAction       = SPEED_MULTIPLY ;
        ptheLEDTask->gEventFilters [ lIndex ].Colour            = LED_COL_EITHER ;  
        ptheLEDTask->gEventFilters [ lIndex ].OverideLED        = 0 ;  
        ptheLEDTask->gEventFilters [ lIndex ].FilterToCancel    = 0 ;
        ptheLEDTask->gEventFilters [ lIndex ].OverideLEDActive  = FALSE ;
        ptheLEDTask->gEventFilters [ lIndex ].FollowerLEDActive = FALSE ;
        ptheLEDTask->gEventFilters [ lIndex ].FollowerLEDDelay  = 0 ;
        ptheLEDTask->gEventFilters [ lIndex ].OverideDisable    = FALSE ;
    }
    
    ptheLEDTask->gLMNumFiltersUsed = 0 ;

    ptheLEDTask->gTheActiveFilters = 0x0000 ;
}


/****************************************************************************
NAME 
    LMGetPattern

DESCRIPTION
    Method to get a pointer to one of the pre allocated patterns - if there are no patterns left,
    returns NULL.

RETURNS
    LedPattern_t *
    
*/
static LEDPattern_t * LMGetPattern ( LedTaskData * ptheLEDTask ) 
{
    LEDPattern_t * lPattern = NULL ;
    
    uint16 lIndex = 0 ;
    
        /*iterate through the patterns looking for one that is unused*/
    for (lIndex = 0 ; lIndex < LM_MAX_NUM_PATTERNS ; lIndex ++ )
    {
        if ( LMIsPatternEmpty ( &ptheLEDTask->gPatterns [ lIndex ] ) )
        {
            /*then this pattern is not used and we can use it*/
            lPattern = &ptheLEDTask->gPatterns [ lIndex ] ;
            LM_DEBUG(("LM : PatFound[%d]\n", lIndex  )) ;
                /*if we have found a free pattern then there is no need to continue looking*/
            return lPattern ;
            
        }        
    }
        /*if we could not find a pattern*/
    if (lPattern == NULL)
    {
        LM_DEBUG(("LM : Pat !\n")) ;
    }
    
    return ( lPattern ) ;
}


/****************************************************************************
NAME 
    LMIsPatternUsed

DESCRIPTION
    Helper method to determine if a pattern has been used or not - checks
    whether the pattern contains valid data.

RETURNS
    bool IsUsed
    
*/
static bool LMIsPatternEmpty (LEDPattern_t * pPattern )
{
    bool lIsUsed = FALSE ;
    
     if ( pPattern->OnTime == 0 )
     {
        if ( pPattern->OffTime == 0 )
        {
            lIsUsed = TRUE ;
        }
     }
    return lIsUsed ; 
}
 

/****************************************************************************
NAME 
    LMReleasePattern

DESCRIPTION
    Method to set apattern to a known state so that it can be used by GetPattern.

*/
static void LMResetPattern ( LEDPattern_t * pPattern )
{
    pPattern->LED_A      = 0 ;
    pPattern->LED_B      = 0 ;
    pPattern->OnTime     = 0 ;
    pPattern->OffTime    = 0 ;
    pPattern->RepeatTime = 0 ;
    pPattern->NumFlashes = 0 ;
    pPattern->DimTime    = 0;
    pPattern->TimeOut    = 0 ;
    pPattern->Colour     = LED_COL_LED_A ;  
    pPattern->OverideDisable = FALSE;
}


/****************************************************************************
NAME 
    LMAddPattern

DESCRIPTION
    Adds an LED Mapping (Event / State). 

RETURNS
    The new destination ptr if there was one allocated.
    
*/
static LEDPattern_t * LMAddPattern ( LedTaskData * ptheLEDTask , LEDPattern_t * pSourcePattern , LEDPattern_t * pDestPattern ) 
{
        /*if the pattern we have been passed is empty then we want to make sure there is no pattern present*/
    if ( LMIsPatternEmpty ( pSourcePattern )  )
    {
            /*if a pattern is already defined for this Location*/
        if ( pDestPattern ) 
        {
                /*then delete the pattern*/
            LMResetPattern (  pDestPattern ) ;            
            pDestPattern = NULL ;
        }
        /*otherwise there was no pattern defined anyway*/
    }
    else
    {
           /*if a pattern is not already defined for this state*/
        if ( ! pDestPattern ) 
        {
                /*get a pattern pointer from our block*/  
            pDestPattern = LMGetPattern ( ptheLEDTask ) ;
        }
        
        /*if we have a pattern (wither from before or just given)*/
        if (pDestPattern)
        {
               /*populate the pattern*/ 
            pDestPattern->LED_A          = pSourcePattern->LED_A ;
            pDestPattern->LED_B          = pSourcePattern->LED_B ;
            pDestPattern->OnTime         = pSourcePattern->OnTime ;
            pDestPattern->OffTime        = pSourcePattern->OffTime ;
            pDestPattern->RepeatTime     = pSourcePattern->RepeatTime ;
            pDestPattern->DimTime        = pSourcePattern->DimTime ;
            pDestPattern->NumFlashes     = pSourcePattern->NumFlashes ;
            pDestPattern->TimeOut        = pSourcePattern->TimeOut ;
            pDestPattern->Colour         = pSourcePattern->Colour ;
            pDestPattern->OverideDisable = pSourcePattern->OverideDisable;
        
           #ifdef DEBUG_LM
           		LMPrintPattern ( pDestPattern ) ;
           #endif		
        }
        else
        {
            LM_DEBUG(("LM: NoPat!\n")) ;
        }
    }
        /*pass the new pointer back to the caller as we may have modified it*/
    return pDestPattern ;
}


































