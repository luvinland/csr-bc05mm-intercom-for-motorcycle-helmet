/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_buttons.c        
@brief    This is the button interpreter for bc5_stereo application.

This file extracts the button messages from the PIO subsystem and figures out
the button press type and time. It passes the information to the button manager
which is responsible for translating the button press generated into a system event
*/
#include "headset_private.h"
#include "headset_buttonmanager.h"
#include "headset_buttons.h"
#include "headset_debug.h"

#include <charger.h>
#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <pio.h>
#include <stddef.h>


#ifdef DEBUG_BUTTONS
#define B_DEBUG(x) DEBUG(x)
#else
#define B_DEBUG(x) 
#endif


typedef enum ButtonsIntMsgTag 
{
    B_DOUBLE_TIMER , 
    B_INTERNAL_TIMER , 
    B_REPEAT_TIMER  
}ButtonsIntMsg_t;


#define VREG_PIN    (24)
#define CHG_PIN     (25)

/*the mask values for the charger pins*/
#define VREG_PIN_MASK ((uint32)1 << VREG_PIN)
#define CHG_PIN_MASK ((uint32)1 << CHG_PIN)

/*the mask values for the charger pin events*/
#define CHARGER_VREG_VALUE ( (uint32)((PioGetVregEn()) ? VREG_PIN_MASK:0 ) )
#define CHARGER_CONNECT_VALUE ( (uint32)( (ButtonsIsChargerConnected())  ? CHG_PIN_MASK:0 ) )

/*
	LOCAL FUNCTION PROTOTYPES
 */
static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage )   ;
static bool ButtonsWasButtonPressed ( uint32 pOldState , uint32 pNewState) ;
static uint32 ButtonsWhichButtonChanged ( uint32 pOldState , uint32 pNewState ) ;
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask ,uint32 pButtonMask  , ButtonsTime_t pTime  ) ;
static void ButtonsEdgeDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) ;       
static void ButtonsLevelDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask )  ;
static bool ButtonsIsChargerConnected ( void ) ;
    
/****************************************************************************
DESCRIPTION
 	Initialises the Button Module parameters
*/  
void ButtonsInit ( ButtonsTaskData *pButtonsTask )
{
    
    
    pButtonsTask->gBOldState    = 0 ;
    pButtonsTask->gBTime        = B_SHORT ;
    pButtonsTask->gBDoubleTap   = FALSE ;
    pButtonsTask->gBDoubleState = 0 ;
    pButtonsTask->gButtonLevelMask   = 0 ;
    pButtonsTask->gBOldEdgeState = 0 ;
    
    pButtonsTask->task.handler = ButtonsMessageHandler;
    
        /*connect the underlying PIO task to this task*/
    MessagePioTask(&pButtonsTask->task);
      
        /*connect the underlying Charger task to this task*/
    MessageChargerTask(&pButtonsTask->task);
	
    B_DEBUG(("B : Get[%lx]\n", PioGet32() )) ;        
}

/****************************************************************************
DESCRIPTION
 	Registers buttons contained in the mask so that they will be detected by
    the button task
*/
void ButtonsRegisterButtons ( ButtonsTaskData *pButtonsTask, uint32 pButtonMask ) 
{	
	uint16 charger_events = 0 ;
    
    pButtonsTask->gButtonLevelMask |= pButtonMask ;  
    
	B_DEBUG(("B  :Reg Lvl[%lx][%lx]\n",pButtonMask , pButtonsTask->gButtonLevelMask)) ;    
    
	pButtonsTask->gBTime = B_INVALID ; 
     
    /* perform a level detect looking for transistion of recently added button definition, mask pio's 
       against level configured pios to prevent false butotn press indications */
    ButtonsLevelDetect ( ((PioGet32() | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) & pButtonsTask->gPerformLevelCheck) , pButtonsTask ) ;	
    
    /* perform an edge detect looking for transistion of recently added button definition, mask pio's 
       against edge configured pios to prevent false button press indications */
	ButtonsEdgeDetect ( ((PioGet32() | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) & pButtonsTask->gPerformEdgeCheck)  , pButtonsTask) ;

         /* Debounce required PIO lines */
    PioDebounce(pButtonsTask->gButtonLevelMask, pButtonsTask->button_config.debounce_number, pButtonsTask->button_config.debounce_period_ms );    
      
    /* Debounce required charger events - special PIO values (24 = vreg, 25 = chg) */
    if ((pButtonsTask->gButtonLevelMask) & VREG_PIN_MASK )
    	charger_events |= CHARGER_VREG_EVENT;
    if ((pButtonsTask->gButtonLevelMask) & CHG_PIN_MASK )
    	charger_events |= CHARGER_CONNECT_EVENT;
    
    B_DEBUG(("B  :Reg chg[%x]\n", charger_events));
             
    if (charger_events)
    	ChargerDebounce(  charger_events, pButtonsTask->button_config.debounce_number, pButtonsTask->button_config.debounce_period_ms );
}

/****************************************************************************
DESCRIPTION
 	the button event message handler - converts button events to the system events
*/

static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage ) 
{   
    ButtonsTaskData * lBTask = (ButtonsTaskData*)pTask ;

    B_DEBUG(("B:Message\n")) ;
    switch ( pId )
    {
	    case MESSAGE_PIO_CHANGED : 
        {
            const MessagePioChanged * lMessage = ( const MessagePioChanged * ) (pMessage ) ;
            
            /* when a pio is configured for an edge detect only there is significant performance gain to be had
               by only doing an edge detect call and not a level detect. To do this use a previously set edge
               detect mask and check this against the current pio being reported. Also need to check if a previously
               set PIO has now been removed and check for the edge transition once again. */
            if((lBTask->gPerformEdgeCheck & (uint32)( ((uint32)lMessage->state16to31 << 16) | lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) ) ||
               (lBTask->gPerformEdgeCheck & lBTask->gOldPioState))
            {
                /* check for a valid edge transition against current pio states masked with edge configured pios
                   and perform appropriate action */                
                ButtonsEdgeDetect  ( ((uint32)( ((uint32)lMessage->state16to31 << 16) | lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) & lBTask->gPerformEdgeCheck), lBTask ) ;
            }          
            
            /* only do a level detect call which is vm/messaging intensive when a pio has been configured as 
               short or long or very long or very very long, i.e. not rising or falling */
            if((lBTask->gPerformLevelCheck & (uint32)( ((uint32)lMessage->state16to31 << 16) | lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) ) ||
               (lBTask->gPerformLevelCheck & lBTask->gOldPioState))
            {
                /* perform a level detection, this call uses a number of messages and is quite slow to process */
                ButtonsLevelDetect ( ((uint32)( ((uint32)lMessage->state16to31 << 16) | lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) & lBTask->gPerformLevelCheck), lBTask ) ;            
            }         
            
            /* store current set pio state in order to be able to detect the transition of a PIO configured as edge 
               detect only */
            lBTask->gOldPioState = (uint32)( ((uint32)lMessage->state16to31 << 16) | lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE);
		}
    	break ;
        
        case MESSAGE_CHARGER_CHANGED:
	    {
		    const MessageChargerChanged *m = (const MessageChargerChanged *) (pMessage ) ;			
         
			B_DEBUG(("B:MCHG\n")) ;
			
            /* when a charger or vreg change event is detectecd perform both an edge and level detection
               passing in only those approriately masked pios for edge or level configured buttons */
            ButtonsLevelDetect ( (((uint32)m->vreg_en_high << VREG_PIN) | ((uint32)m->charger_connected << CHG_PIN) | PioGet32()) & lBTask->gPerformLevelCheck, lBTask ) ;           
            ButtonsEdgeDetect  ( (((uint32)m->vreg_en_high << VREG_PIN) | ((uint32)m->charger_connected << CHG_PIN) | PioGet32()) & lBTask->gPerformEdgeCheck , lBTask ) ;           
	    }
        break;

    	case B_DOUBLE_TIMER:
		{
				/*if we have reached here, then a double timer has been received*/
         	B_DEBUG(("B:Double[%lx][%x]\n", lBTask->gBDoubleState , B_SHORT_SINGLE)) ;
    
         	lBTask->gBDoubleTap = FALSE ;
        		/*indicate that a short button was pressed and it did not become a double press */
        	ButtonsButtonDetected ( lBTask, lBTask->gBDoubleState , B_SHORT_SINGLE ); 
		} 
        break ;
    	case B_INTERNAL_TIMER:
		{
			/*if we have reached here, then the buttons have been held longer than one of the timed messages*/
	        B_DEBUG(("B:Timer\n")) ;
            
            /* an internal timer has triggered which was initiated from the level detect function call */
        	if ( lBTask->gBTime == B_VERY_LONG )
        	{
                /* update timer state flag */
                lBTask->gBTime = B_VERY_VERY_LONG ;                
        	}
			/* a long press timer event has triggered */
            else if ( lBTask->gBTime == B_LONG )
        	{
                /* don't send very very long timer message until needed, i.e. very_long timer expired */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER , 0 ,  (lBTask->button_config.very_very_long_press_time - lBTask->button_config.very_long_press_time - lBTask->button_config.long_press_time) ) ;                   
                /* update tiemr state flag */
            	lBTask->gBTime = B_VERY_LONG ;
               	/*notify the app that the timer has expired*/
            	MessageSend( getAppTask() , EventVLongTimer , 0 ) ;    
        	}
			/* the first timer event triggered from the level detect call */
            else
        	{
                /* only send very long message when long timer expired to save messaging.                 */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER  , 0 ,  (lBTask->button_config.very_long_press_time - lBTask->button_config.long_press_time)) ;            
               	/*notify the app that the timer has expired*/
            	MessageSend( getAppTask() , EventLongTimer , 0 ) ;    
           		lBTask->gBTime = B_LONG ;
        	}    
            	/*indicate that we have received a message */
        	ButtonsButtonDetected ( lBTask, lBTask->gBOldState , lBTask->gBTime ); 
		}         
        break ;
    	case B_REPEAT_TIMER:
		{
			/*if we have reached here, the repeat time has been reached so send a new message*/
        	B_DEBUG(("B:Repeat[%lx][%x]\n", lBTask->gBOldState , B_REPEAT  )) ;
        
        	/*send another repeat message*/
        	MessageSendLater ( &lBTask->task , B_REPEAT_TIMER , 0 ,  lBTask->button_config.repeat_time ) ; 

        	ButtonsButtonDetected ( lBTask, lBTask->gBOldState , B_REPEAT ); 
		}
        break;
    	default :
           B_DEBUG(("B:?[%x]\n",pId)) ; 
        break ;
    }
}
/****************************************************************************
DESCRIPTION
 	helper method - returns true if a button was pressed
*/  
static bool ButtonsWasButtonPressed ( uint32 pOldState , uint32 pNewState)
{
    bool lWasButtonPressed = FALSE ;
 
    uint32 lButton = ButtonsWhichButtonChanged ( pOldState , pNewState ) ;
    
    if ( ( lButton & pNewState ) != 0 )
    {
        lWasButtonPressed = TRUE ;
    }
    
    return lWasButtonPressed ;
}
/****************************************************************************
DESCRIPTION
 	helper method - returns mask ofwhich button changed
*/ 
static uint32 ButtonsWhichButtonChanged ( uint32 pOldState , uint32 pNewState )
{
    uint32 lWhichButton = 0 ;
        
    lWhichButton = (pNewState ^ pOldState ) ;
	
	return lWhichButton ;
}

/****************************************************************************  
DESCRIPTION
 	function to handle a button press - informs button manager of a change
    currently makes direct call - may want to use message handling (tasks)
*/ 
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask, uint32 pButtonMask  , ButtonsTime_t pTime )
{
    B_DEBUG(("B:But Det[%lx]\n", pButtonMask)) ;
    
    if( pButtonMask == 0 )
    {
     	MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
    }
    else
    {
        BMButtonDetected ( pButtonsTask, pButtonMask, pTime ) ;             
    } 
}

/****************************************************************************
DESCRIPTION
 	function to detect level changes of buttons / multiple buttons. 
*/ 
static void ButtonsLevelDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) 
{
    uint32 lNewState = 0;

    	/*we have an indication from the PIO subsytem that a PIO has changed value*/
    lNewState = (uint32) (pState & (pButtonsTask->gButtonLevelMask) ) ;
    
    B_DEBUG(("But Lev Det|:[%lx][%lx]\n", pState , (pButtonsTask->gButtonLevelMask) )) ;
    B_DEBUG(("But Lev Det|:[%lx][%lx]\n", pButtonsTask->gBOldState , lNewState )) ;
    
    if ( ButtonsWasButtonPressed( pButtonsTask->gBOldState , lNewState )  )
    {
        /*cancel all previously timed messages*/   
        MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER ) ;
        MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
        MessageCancelAll ( &pButtonsTask->task , B_DOUBLE_TIMER ) ;           
        
        /* send new timed messages*/
        MessageSendLater ( &pButtonsTask->task , B_INTERNAL_TIMER , 0 ,  pButtonsTask->button_config.long_press_time ) ; 
        MessageSendLater ( &pButtonsTask->task , B_REPEAT_TIMER   , 0 ,  pButtonsTask->button_config.repeat_time ) ; 	
        
        /*having restrted the timers, reset the time*/
        pButtonsTask->gBTime = B_SHORT ;      
    }
    /*button was released or was masked out, check to make sure there is a pio bit change as vreg enable
      can generate an addition MSG without any pio's changing state */
    else if(pButtonsTask->gBOldState != lNewState )              
    {         /*it was only a released if there was a button actually pressed last time around - 
              buttons we have masked out still end up here but no state changes are made  */       
        if ( pButtonsTask->gBOldState != 0 )
        {
                 /*if we have had a double press in the required time 
                 and the button pressed was the same as this one*/
             if (  (pButtonsTask->gBDoubleTap ) && (pButtonsTask->gBOldState == pButtonsTask->gBDoubleState ) )
             {
                 pButtonsTask->gBTime = B_DOUBLE ;
                    /*reset the double state*/
                 pButtonsTask->gBDoubleState = 0x0000 ;
                 pButtonsTask->gBDoubleTap = FALSE ;
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_DOUBLE  ); 
             }                    

             
                /*only send a message if it was a short one - long / v long /double handled elsewhere*/
             if ( (pButtonsTask->gBTime == B_SHORT ) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_SHORT  ); 
                 
                 /*store the double state*/
                 pButtonsTask->gBDoubleState = pButtonsTask->gBOldState ;
                 pButtonsTask->gBDoubleTap = TRUE ;
        
                    /*start the double timer - only applicable to a short press*/
                 MessageSendLater ( &pButtonsTask->task , B_DOUBLE_TIMER , 0 ,pButtonsTask->button_config.double_press_time ) ;
             }  
             else if ( (pButtonsTask->gBTime == B_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_LONG_RELEASE  );                  
             }
             else if ( (pButtonsTask->gBTime == B_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_VERY_LONG_RELEASE  ); 
             }
             else if ( (pButtonsTask->gBTime == B_VERY_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_VERY_VERY_LONG_RELEASE  ); 
             }
             
             if (pButtonsTask->gBTime != B_INVALID)
             {
                MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER) ;
                MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ; 
             }    
             
			 /*removing this allows all releases to generate combination presses is this right?*/
             if ( !lNewState )
             {
                 pButtonsTask->gBTime = B_INVALID ;      
             } 
         }     
    }
    pButtonsTask->gBOldState = lNewState ;
}
  


/****************************************************************************

DESCRIPTION
 	function to detect edge changes of buttons / multiple buttons. 

*/ 
static void ButtonsEdgeDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) 
{
    uint32 lNewState = 0x0000 ;
    
    uint32 lButton = 0x0000 ;
    
    /*what has changed */
    lNewState = (uint32) ( pState & (pButtonsTask->gButtonLevelMask) ) ;
    
    lButton = ButtonsWhichButtonChanged( pButtonsTask->gBOldEdgeState , lNewState ) ;
    
    
    B_DEBUG(("But Edge Det: [%lx][%lx][%lx]\n", pButtonsTask->gBOldEdgeState , lNewState , pButtonsTask->gButtonLevelMask  )) ;
    B_DEBUG(("But Edge Det: [%lx][%lx][%lx]\n", lNewState , lButton , (lNewState & lButton) )) ;
    
        /*if a button has changed*/
    if ( lButton )
    {
            /*determine which edge has been received and process accordingly*/
        if ( lNewState & lButton )
        {  
            ButtonsButtonDetected ( pButtonsTask , lButton , B_LOW_TO_HIGH )   ;
        }
        else
        {
            ButtonsButtonDetected ( pButtonsTask , lButton , B_HIGH_TO_LOW  )   ;
        }
    }
        /*remember the last state*/
    pButtonsTask->gBOldEdgeState = lNewState;
}

  
/****************************************************************************
NAME
    ButtonsIsChargerConnected
 
DESCRIPTION
 	Returns if the charger is connected or not.

*/ 
static bool ButtonsIsChargerConnected ( void )
{
	bool connected = FALSE;
	
	switch (ChargerStatus())
	{
	case TRICKLE_CHARGE:
	case FAST_CHARGE:
	case CHARGING:
	case STANDBY:
	case DISABLED_ERROR:
		connected = TRUE;
		break;
		
	case NO_POWER:
	case NOT_CHARGING:
		break;
		
	default:
		break;
	}
	
	B_DEBUG(("B : CHARGER CONN [%d]\n",connected));
	
	return connected;
}
