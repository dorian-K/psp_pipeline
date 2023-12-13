/*! \file
 *  \brief Input library for the OS.
 *
 *  Contains functionalities to read user input.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_INPUT_H
#define _OS_INPUT_H

#include <stdint.h>

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Refreshes the button states
uint8_t os_getInput(void);

//! Initializes DDR and PORT for input
void os_initInput(void);

//! Waits for all buttons to be released
void os_waitForNoInput(void);

//! Waits for at least one button to be pressed
void os_waitForInput(void);

#endif
