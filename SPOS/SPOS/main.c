#include "defines.h"
#include "lcd.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/pgmspace.h>

//----------------------------------------------------------------------------
// Operation System Booting
//----------------------------------------------------------------------------

//! Program's entry point
int main(void) {
    // Give the operating system a chance to initialize its private data.
    // This also registers and starts the idle program.
    os_init();

    // os_init shows a boot message
    // Wait and clear the LCD
    delayMs(600);
    lcd_clear();

    // Start the operating system
    os_startScheduler();

    return 1;
}

/*!
 *  \mainpage SPOS Manual
 *  Praktikum Systemprogrammierung: Operating System (SPOS)\n
 *  SPOS is an operation system that was developed for education purposes at
 *  the Embedded Systems Laboratory (i11), RWTH Aachen University.\n
 *  All provided materials are copyrighted by the i11 and licensed only for
 *  use in the course 'Praktikum Systemprogrammierung' .\n\n
 *
 *  This documentation describes the functionalities of the given
 *  SPOS framework.
 *  It contains some basic information about the data types,
 *  variables and functions of SPOS.
 *  This document is designed as a guideline to help you with your own
 *  implementation of SPOS. It is not necessary that you implement every data
 *  type, variable or function in this Doxygen documentation.
 *  You just need to implement the entities mentioned and explained in the PDF
 *  lab documentation for the respective lab term.
 *  This Doxygen document may contain additional help functions or variables
 *  that can, but do not have to be implemented and used by you in your
 *  implementation.\n
 *
 *  However, the signatures for the functions in your PDF lab documentation are
 *  consistent with this document, so you can use it as a reference for your
 *  homework.
 *  The following list mentions files that contain key SPOS modules.
 *  Note that this document is generated for a version of SPOS after exercise 2.
 *
 *    - os_core.h
 *    - os_scheduler.h
 *    - os_scheduling_strategies.h
 *    - os_input.h
 *
 *  After that you may want to know about the lcd.h in order to write your
 *  own programs in progs.c. It is vital for you to precisely study defines.h.
 */
