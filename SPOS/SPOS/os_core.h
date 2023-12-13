/*! \file
 *  \brief Core of the OS.
 *
 *  Contains functionality to for core OS operations.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_CORE_H
#define _OS_CORE_H

#include <avr/pgmspace.h>

//! Allowed reset sources that are not considered erroneous resetting of the MCU
#define OS_ALLOWED_RESET_SOURCES (_BV(JTRF) | _BV(BORF) | _BV(EXTRF) | _BV(PORF))

//! Handy define to specify error messages directly
#define os_error(str) os_errorPStr(PSTR(str))

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Initializes timers
void os_init_timer(void);

//! Examines the saved MCU status register and possibly prints an error if the reset source is not allowed
void os_checkResetSource(uint8_t allowedSources);

//! Initializes OS
void os_init(void);

//! Shows error on display and terminates program
void os_errorPStr(const char *str);

#endif
