/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2008
*/

/*!
@file    headset_buttons.h
@brief   Interface to the headset button functionality. 
*/
#ifndef HEADSET_BUTTONS_H
#define HEADSET_BUTTONS_H

#include "headset_buttonmanager.h"

/****************************************************************************
NAME 
 buttonManagerInit

DESCRIPTION
 Initialises the button event 

RETURNS
 void
    
*/
void ButtonsInit (  ButtonsTaskData *pButtonsTask ) ;

/*************************************************************
NAME 
 ButtonsRegisterButtons

DESCRIPTION
 Registers buttons contained in the mask so that they will be detected by
    the button task

RETURNS
	void
    
*/
void ButtonsRegisterButtons (ButtonsTaskData *pButtonsTask, uint32 pButtonMask ) ;

#endif
