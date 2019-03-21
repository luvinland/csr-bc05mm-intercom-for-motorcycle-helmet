/****************************************************************************
 Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_leddata.h
@brief    data structures  /defines for use with the LED module
*/
#ifndef HEADSET_LED_DATA_H
#define HEADSET_LED_DATA_H

#include "headset_events.h"


/****************************************************************************
Types
*/
    /*the number of LEDS (including pin outputs that we support*/
#define HEADSET_NUM_LEDS (16)

#define LM_MAX_NUM_PATTERNS (35)
#define LM_NUM_FILTER_EVENTS (20)


typedef enum LEDSpeedActionTag
{
    SPEED_MULTIPLY = 0,
    SPEED_DIVIDE  
}LEDSpeedActionTag ;

typedef enum LEDColTag
{
    LED_COL_EITHER ,
    LED_COL_LED_A ,
    LED_COL_LED_B ,
    LED_COL_LED_ALT ,    /*Alternate the colours*/
    LED_COL_LED_BOTH    /*use Both LEDS*/
}LEDColour_t ;

typedef struct LEDFilterTag
{
    headsetEvents_t     Event ;      /*The event to action the filter upon*/
    
	unsigned            IsFilterActive:1 ;
    unsigned            Speed:8 ;      /*speed multiple o apply - 0 =  no speed multiple*/
    unsigned            SpeedAction:2 ;/*which action to perform on the multiple  multiply or divide */
    unsigned            Colour:3 ;     /*Force LED to this colour pattern no matter what is defined in the state pattern*/    
	unsigned            OverideLEDActive:1 ;
    unsigned            FollowerLEDActive:1 ;/*whether this filter defines a follower led*/

    unsigned            FilterToCancel:7 ;
    unsigned            OverideDisable:1 ; /* overide LED disable flag when filter active */
   
	unsigned            FollowerLEDDelay:4 ; /*the Delay before the following pattern starts*/ /*50ms (0 - 750ms)*/
    unsigned            OverideLED:4;
}LEDFilter_t ;


    /*the led pattern type */
typedef struct LEDPatternTag
{
    
    unsigned          OnTime:16     ; /*ms*/
    unsigned          OffTime:16    ; /*ms*/
    unsigned          RepeatTime:16 ; /*ms*/ 
    
    unsigned          TimeOut:8     ; /*number of repeats*/
    unsigned          DimTime:8     ; /*Time to Dim this LED*/       
    
    unsigned          LED_A:4      ; /*default first LED to use*/
    unsigned          LED_B:4      ; /*second LED to use*/     
    unsigned          NumFlashes:4 ; /*how many flashes in the pattern*/       
    unsigned          OverideDisable:1; /* overide LED disable flag for this pattern */
    unsigned          Colour:3     ; /*which of the LEDS to use*/     
}LEDPattern_t ;


typedef enum IndicationTypeTag
{
    IT_Undefined = 0 ,
    IT_StateIndication,
    IT_EventIndication    
    
}IndicationType_t ;

    /*the information required for a LED to be updated*/
typedef struct LEDActivityTag
{  
    unsigned         Index:7; /*what this led is displaying*/
    unsigned         NumFlashesComplete:8 ; /*how far through the pattern we currently are*/        
    unsigned         OnOrOff:1 ;
    
    unsigned         FilterIndex:4 ;/*the filter curently attached to this LED (0-15)*/    
    unsigned         Type:2 ; /*what this LED is displaying*/
    unsigned         NumRepeatsComplete:10;
        /*dimming*/
    unsigned         DimState:7  ; /*how far through the dim pattern we are*/
    unsigned         DimDir:1    ; /*which direction we are going*/
    unsigned         DimTime:8   ;
    
}LEDActivity_t ;


    /*the event message sent on completeion of an event */
typedef struct 
{
    uint16 Event ;  
    bool PatternCompleted ;
    
} LMEndMessage_t;
    

typedef struct LEDEventQueueTag
{
    unsigned Event1:8 ;
    unsigned Event2:8 ;
    unsigned Event3:8 ;
    unsigned Event4:8 ;    
} LEDEventQueue_t;


/*the tricolour led information*/
typedef struct PioTriColLedsTag
{
	unsigned TriCol_a:4;
	unsigned TriCol_b:4;
	unsigned TriCol_c:4;
	unsigned Unused1 :4;
}PioTriColLeds_t ;


   /*The LED task type*/
typedef struct
{
  	TaskData                task;
    LEDPattern_t * *        gStatePatterns ;  /*the array of pointers to the state patterns */
    LEDPattern_t * *        gEventPatterns  ; /*the array of pointers to the event patterns */
 
    LEDPattern_t *          gPatterns ; /*the actual storage for he LED patterns pointed to by the configurable event * *     */
    
    LEDFilter_t *           gEventFilters  ;/*pointer to the array of LED Filter patterns */
    uint16                  gLMNumFiltersUsed ;
    
    uint16                  gTheActiveFilters ; /*Mask of Filters Active*/
    
    LEDActivity_t *         gActiveLEDS ; /* the array of LED Activities*/
    
    
    unsigned                gLED_0_STATE:1 ;
    unsigned                gLED_1_STATE:1 ;
    
    unsigned                gLEDSStateTimeout:1 ; /*this is set to true if a state pattern has completed - reset if new event occurs*/
    unsigned                gLEDSEnabled:1 ;      /*global LED overide  - event drivedn to enable / disable all LED Indications*/  
    
    unsigned                gCurrentlyIndicatingEvent:1; /*if we are currently indicating an event*/
    
    unsigned 				gFollowing:1 ; /**do we currently have a follower active*/
    
    unsigned                Dummy:10;
    
    LEDEventQueue_t         Queue ;
    /*PioTriColLeds_t         gTriColLeds ;*/
    
} LedTaskData;  

 
#endif

