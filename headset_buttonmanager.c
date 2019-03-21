/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_buttonmanager.c
@brief    Implementation of the button manager.
    
This file provides a configurable wrapper for the button messages and
converts them into the standard system messages which are passed to the
main message handler - main.c
*/
#include "headset_private.h"
#include "headset_buttonmanager.h"
#include "headset_statemanager.h"
#include "headset_buttons.h"
#include "headset_volume.h"
#include "headset_debug.h"

#include <stddef.h>
#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>


#include "headset_events.h"

#ifdef DEBUG_BUT_MAN
    #define BM_DEBUG(x) DEBUG(x)
    
    const char * const gDebugTimeStrings[13] = {"Inv" , 
    											"Short", 
                                                "Long" ,
                                                "VLong" , 
                                                "Double" ,
                                                "Rpt" , 
                                                "LToH" , 
                                                "HToL" , 
                                                "ShSingle",
                                                "Long Release",
                                                "VLong Release",
                                                "V V Long" ,
                                                "VV Long Release"} ;
                 
#else
    #define BM_DEBUG(x) 
#endif

/*
 LOCAL FUNCTION PROTOTYPES
 */
static void BMCheckForButtonMatch ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask , ButtonsTime_t  pDuration  )  ;

static ButtonEvents_t * BMGetButtonEvent ( ButtonsTaskData *pButtonsTask ) ;

static void BMCheckForButtonPatternMatch ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask ) ;

/****************************************************************************
VARIABLES  
*/

#define BM_EVENTS_PER_BLOCK (20)
#define BM_NUM_BLOCKS (2)
#define BM_NUM_CONFIGURABLE_EVENTS (BM_EVENTS_PER_BLOCK * BM_NUM_BLOCKS)

#define BUTTON_PIO_DEBOUNCE_NUM_CHECKS  (4)
#define BUTTON_PIO_DEBOUNCE_TIME_MS     (15)

/****************************************************************************
  FUNCTIONS
*/


/****************************************************************************
NAME 
 	buttonManagerInit
    
DESCRIPTION
 	Initialises the Button Module parameters
RETURNS
 	void
*/  
void buttonManagerInit ( ButtonsTaskData *pButtonsTask, Task pClient )
{   
	uint16 lIndex = 0 ;
    uint16 lButtonIndex = 0; 

    pButtonsTask->client = pClient;
 
        /*the button events*/
    pButtonsTask->gButtonEvents [0]  = NULL ;
    pButtonsTask->gButtonEvents [1]  = NULL ;
    
    /* initialise the edge and level detect mask values */
    pButtonsTask->gPerformEdgeCheck = 0;
    pButtonsTask->gPerformLevelCheck = 0;    
  
	/*create the button patterns*/	
    /*get the memory for the button Match Patterns*/
    pButtonsTask->gButtonPatterns[0] = PanicUnlessMalloc ( (sizeof(ButtonMatchPattern_t) * BM_NUM_BUTTON_MATCH_PATTERNS) ) ;
    
    
    for (lIndex = 0 ; lIndex < BM_NUM_BUTTON_MATCH_PATTERNS ; lIndex++ )
    {
        pButtonsTask->gButtonPatterns[ lIndex + 1 ] =  pButtonsTask->gButtonPatterns[ lIndex ] + 1  ;
 	
		/*set the progress to the beginning*/
        pButtonsTask->gButtonMatchProgress[lIndex] = 0 ;
        
            /*set the button match patterns to known vals*/
        pButtonsTask->gButtonPatterns[lIndex]->NumberOfMatches = 0 ;
        pButtonsTask->gButtonPatterns[lIndex]->EventToSend = 0 ;        
        
        for (lButtonIndex = 0 ; lButtonIndex < BM_NUM_BUTTONS_PER_MATCH_PATTERN ; lButtonIndex++)
        {
            pButtonsTask->gButtonPatterns[lIndex]->ButtonToMatch[lButtonIndex] = B_INVALID ;
        }   
    }
	
		/*create the array of Button Events that we are going to poulate*/    
    pButtonsTask->gButtonEvents[0] = (ButtonEvents_t * ) ( PanicUnlessMalloc( sizeof( ButtonEvents_t ) * BM_EVENTS_PER_BLOCK ) ) ;
    pButtonsTask->gButtonEvents[1]= (ButtonEvents_t * ) ( PanicUnlessMalloc( sizeof( ButtonEvents_t ) * BM_EVENTS_PER_BLOCK ) ) ;
    
      /*init the PIO button routines with the Button manager Task data */ 
    ButtonsInit( pButtonsTask ) ; 
}

/****************************************************************************

DESCRIPTION
	Wrapper method for the button Duration Setup
	configures the button durations to the user values

*/   
void buttonManagerConfigDurations ( ButtonsTaskData *pButtonsTask, button_config_type pButtons )
{
    pButtonsTask->button_config = pButtons ;
	
	if ((pButtons.debounce_number == 0 ) || ( pButtons.debounce_period_ms == 0 ) )
	{
		/*use defaults*/
		BM_DEBUG(("BM: DEFAULT button debounce\n")) ;
		pButtonsTask->button_config.debounce_number =  BUTTON_PIO_DEBOUNCE_NUM_CHECKS;
		pButtonsTask->button_config.debounce_period_ms = BUTTON_PIO_DEBOUNCE_TIME_MS ;		
	}
	else
	{
		BM_DEBUG(("BM: Debounce[%x][%x]\n" , pButtonsTask->button_config.debounce_number ,pButtonsTask->button_config.debounce_period_ms)) ;
	}
}

/****************************************************************************
NAME	
	buttonManagerAddMapping

DESCRIPTION
	Maps a button event to a system event
        
    pButtonMask - 
    mask of the buttons that when pressed will generate an event
    e.g.  0x0001 = button PIO 0
    
          0x0003 = combination of PIO 0  and PIO 1
    pSystemEvent
        The Event to be signalled as define in headset_events.h
    pStateMask
        the states as defined in headset_states that the event will be generated in
    pDuration
        the Duration of the button press as defined in headset_buttons.h
        B_SHORT , B_LONG , B_VLONG, B_DOUBLE
          
RETURNS
 bool to indicate success of button being added to map
    
*/     
bool buttonManagerAddMapping ( ButtonsTaskData *pButtonsTask,
							   uint32        pButtonMask , 
                               headsetEvents_t     pSystemEvent , 
                               uint16        pHfpStateMask , 
							   uint16        pA2dpStateMask ,
                               ButtonsTime_t pDuration ) 
{
    ButtonEvents_t * lButtonEvent ;

    bool lAddOk = FALSE;
    
    BM_DEBUG(("BM : nB[%lx]E[%x]HS[%x]AS[%x] [%s]",pButtonMask, pSystemEvent, pHfpStateMask, pA2dpStateMask, gDebugTimeStrings[pDuration])) ;
    
    
    lButtonEvent =  BMGetButtonEvent( pButtonsTask) ;
        /**if we were given a button event**/
    if ( lButtonEvent )
    {
    
        lButtonEvent->ButtonMask = pButtonMask ;
        lButtonEvent->Duration   = pDuration ;
        lButtonEvent->Event      = pSystemEvent ;
        lButtonEvent->HfpStateMask  = pHfpStateMask ;
		lButtonEvent->A2dpStateMask  = pA2dpStateMask ;
    
        /* look for edge detect config and add the pio's used to the check for edge detect */
        if((pDuration == B_LOW_TO_HIGH)||(pDuration == B_HIGH_TO_LOW))
        {
            /* add pio mask bit to U16 mask store */
            pButtonsTask->gPerformEdgeCheck |= pButtonMask;
        }
        /* otherwise must be a level check */
        else
        {
            pButtonsTask->gPerformLevelCheck |= pButtonMask;
        }
                
            /*inc the global index*/
        pButtonsTask->gNumEventsConfigured++ ;
        
            /*register the buttons we are interested in with the buttons task*/             
        ButtonsRegisterButtons (pButtonsTask , pButtonMask ) ;
        
        lAddOk = TRUE ;

    }
    else
    {
        BM_DEBUG(("_![%x]\n", pButtonsTask->gNumEventsConfigured)) ;
    }
       return lAddOk ;
}

/****************************************************************************
DESCRIPTION
 	Method to get a ButtonEvent if there are any free ones available

RETURNS
    ButtonEvents_t * if there is one available - NULL otherwise
*/

static ButtonEvents_t * BMGetButtonEvent ( ButtonsTaskData *pButtonsTask )
{
    ButtonEvents_t * lButtonEvent = NULL ;
    /*iterate through the button Eventts looking for an empty block*/
    uint16 lBlockIndex = 0 ;    
    uint16 lEvIndex = 0 ;
     
        /*each block*/
    for (lBlockIndex = 0 ; lBlockIndex < BM_NUM_BLOCKS ; lBlockIndex++)
    {       /*Each Entry*/        
        for (lEvIndex = 0 ; lEvIndex < BM_EVENTS_PER_BLOCK ; lEvIndex ++)
        { 
            lButtonEvent = &pButtonsTask->gButtonEvents[lBlockIndex][lEvIndex];
                /*if this event is empty*/
            if ( lButtonEvent->Event == (EventInvalid - EVENTS_EVENT_BASE) )
            {
                BM_DEBUG(("BM: BuF[%d][%d]\n" , lBlockIndex , lEvIndex )) ;    
                    /*if we find an empty one then do not continue looking*/
                return lButtonEvent ;
            }
        }
    }    
        /*if there were no events available return NULL*/
    return NULL ;
}

/****************************************************************************
DESCRIPTION
 	add a new button pattern mapping
*/
bool buttonManagerAddPatternMapping ( ButtonsTaskData *pButtonsTask, uint16 pSystemEvent , button_pattern_type * pButtonsToMatch ) 
{
    uint16 lMapIndex = 0 ;
    uint16 lButtonIndex = 0 ;

    /*adds a button pattern map*/
    for (lMapIndex = 0 ; lMapIndex < BM_NUM_BUTTON_MATCH_PATTERNS ; lMapIndex++)
    {
        if (pButtonsTask->gButtonPatterns[lMapIndex]->EventToSend == B_INVALID )
        {
            pButtonsTask->gButtonPatterns[lMapIndex]->EventToSend = pSystemEvent ;
        
            for (lButtonIndex = 0 ; lButtonIndex < BM_NUM_BUTTONS_PER_MATCH_PATTERN ; lButtonIndex++)
            {
                pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[lButtonIndex] = ((uint32)pButtonsToMatch[lButtonIndex].pio_mask_16_to_31 << 16) | pButtonsToMatch[lButtonIndex].pio_mask_0_to_15;
            
                if (pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[lButtonIndex] != 0)
                {
                    pButtonsTask->gButtonPatterns[lMapIndex]->NumberOfMatches = lButtonIndex + 1;
                }
            }
            
            BM_DEBUG(("BM: But Pat Added[%d] [%x] ,[%lx][%lx][%lx][%lx][%lx][%lx] [%d]\n" , lMapIndex , pButtonsTask->gButtonPatterns[lMapIndex]->EventToSend
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[0]
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[1]
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[2]
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[3]
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[4]
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->ButtonToMatch[5]
                                                                            
                                                                            , pButtonsTask->gButtonPatterns[lMapIndex]->NumberOfMatches
                                                                            
                                                                                )) ;
            return TRUE ;
        }    
    }
    
    return FALSE ;
}

/****************************************************************************
NAME	
	BMButtonDetected

DESCRIPTION
	function call for when a button has been detected 
RETURNS
	void
    
*/
void BMButtonDetected ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask , ButtonsTime_t pTime  )
{
 
    BM_DEBUG(("BM : But [%lx] [%s]\n" ,pButtonMask ,  gDebugTimeStrings[pTime]  )) ;
 
        /*perform the search over both blocks*/
    BMCheckForButtonMatch ( pButtonsTask, pButtonMask  , pTime ) ;
    
        /*only use regular button presses for the pattern matching to make life simpler*/
    if ( ( pTime == B_SHORT ) || (pTime == B_LONG ) )
    {
        BMCheckForButtonPatternMatch ( pButtonsTask, pButtonMask ) ;
    }   
}


/****************************************************************************
NAME 
 BMCheckForButtonMatch
    
DESCRIPTION
 function to check a button for a match in the button events map - sends a message
    to a connected task with the corresponding event
    
RETURNS

    void
*/   
static void BMCheckForButtonMatch ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask , ButtonsTime_t  pDuration ) 
{
    uint16 lHfpStateBit = ( 1 << stateManagerGetHfpState () ) ; 
    uint16 lA2dpStateBit = ( 1 << stateManagerGetA2dpState () ) ;  
    uint16 lBlockIndex = 0 ; 
    uint16 lEvIndex = 0 ;
	
	BM_DEBUG(("BM : BMCheckForButtonMatch [%x][%x][%lx][%x]\n" , lHfpStateBit , lA2dpStateBit , pButtonMask, pDuration)) ;
    
        /*each block*/
    for (lBlockIndex = 0 ; lBlockIndex < BM_NUM_BLOCKS ; lBlockIndex++)
    {       /*Each Entry*/        
        for (lEvIndex = 0 ; lEvIndex < BM_EVENTS_PER_BLOCK ; lEvIndex ++)
        { 
            ButtonEvents_t * lButtonEvent = &pButtonsTask->gButtonEvents [lBlockIndex] [ lEvIndex ] ;
            /*if the event is valid*/
            if ( lButtonEvent != NULL)
            {            
                if (lButtonEvent->ButtonMask == pButtonMask )
                {                          
                    /*we have a button match*/
                    if ( lButtonEvent->Duration == pDuration )
                    {           
                        if ( ((lButtonEvent->HfpStateMask) & (lHfpStateBit)) && ((lButtonEvent->A2dpStateMask) & (lA2dpStateBit)) )
                        {
                            BM_DEBUG(("BM : State Match [%lx][%x]\n" , pButtonMask , lButtonEvent->Event)) ;
                            
                            /* due to the slow processing of messages when trying to change volume
                               check for volume up and down events and call the volume change functions
                               directly instead of using messages, this gives up to approx 1 second 
                               quicker turn around */
                            if(lButtonEvent->Event == EventVolumeUp)
                            {
                                /* obtain pointer to the main headset app */
                                hsTaskData * theHeadset =  (hsTaskData *) getAppTask();
								
                                if (!theHeadset->buttons_locked || (stateManagerGetHfpState() == headsetActiveCall))
                                	VolumeUp( theHeadset ) ;
								else
									BM_DEBUG(("BM : Buttons Locked\n"));
                            }
                            /* also check for volume down presses */
                            else if(lButtonEvent->Event == EventVolumeDown) /* R100 */
                            {
                                /* obtain pointer to the main headset app */
                                hsTaskData * theHeadset =  (hsTaskData *) getAppTask();
                                
								if (!theHeadset->buttons_locked || (stateManagerGetHfpState() == headsetActiveCall))
                                	VolumeDown( theHeadset ) ; 
								else
									BM_DEBUG(("BM : Buttons Locked\n"));
                            }
                            else
                            {
                                /*we have fully matched an event....so tell the main task about it*/
                                MessageSend( pButtonsTask->client, lButtonEvent->Event , 0 ) ;								
                            }
                        }
                    }
                }
            }
        }
    }
}
  
/****************************************************************************
DESCRIPTION
 	check to see if a button pattern has been matched
*/
static void BMCheckForButtonPatternMatch ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask  ) 
{
    uint16 lIndex = 0 ;
    
    BM_DEBUG(("BM: Pat[%lx]\n", pButtonMask )) ;
    
    for (lIndex = 0; lIndex < BM_NUM_BUTTON_MATCH_PATTERNS ; lIndex++ )
    {

        if ( pButtonsTask->gButtonPatterns[lIndex]->ButtonToMatch[pButtonsTask->gButtonMatchProgress[lIndex]] == pButtonMask )
        {
                    /*we have matched a button*/
            pButtonsTask->gButtonMatchProgress[lIndex]++ ;
            
            BM_DEBUG(("BM: Pat Prog[%d][%x]\n", lIndex , pButtonsTask->gButtonMatchProgress[lIndex]  )) ;
                    
                
            if (pButtonsTask->gButtonMatchProgress[lIndex] >= pButtonsTask->gButtonPatterns[lIndex]->NumberOfMatches)
            {
                        /*we have matched a pattern*/
                BM_DEBUG(("BM: Pat Match[%d] Ev[%x]\n", lIndex ,pButtonsTask->gButtonPatterns[lIndex]->EventToSend)) ;
                
                pButtonsTask->gButtonMatchProgress[lIndex] = 0 ;
                
                MessageSend( pButtonsTask->client, pButtonsTask->gButtonPatterns[lIndex]->EventToSend , 0 ) ;
            }
            
        }       
        else
        {
            pButtonsTask->gButtonMatchProgress[lIndex] = 0 ;
                /*special case = if the last button pressed was the same as the first button*/
            if ( pButtonsTask->gButtonPatterns [ lIndex ]->ButtonToMatch[0]== pButtonMask)            
            {
                pButtonsTask->gButtonMatchProgress[lIndex] = 1 ;
            
            }
        }
    }
}

