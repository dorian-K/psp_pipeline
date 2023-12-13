/*! \file
 *  \brief Constants for ATMega644 and EVA board.
 *  Defines several useful constants for the ATMega644
 *
 *  \author  Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date    2013
 *  \version 2.0
 */

#ifndef _ATMEGA644CONSTANTS_H
#define _ATMEGA644CONSTANTS_H

#include <avr/io.h>


//----------------------------------------------------------------------------
// Board properties
//----------------------------------------------------------------------------

//! Clock frequency on evaluation board in HZ (20 MHZ)
#define AVR_CLOCK_FREQUENCY 20000000ul

#ifdef F_CPU
#undef F_CPU
#endif
//! Clock frequency for WinAVR delay function
#define F_CPU AVR_CLOCK_FREQUENCY


//----------------------------------------------------------------------------
// MC properties
//----------------------------------------------------------------------------

//! Starting address of SRAM
#define AVR_SRAM_START RAMSTART

//! Ending address of SRAM -- First invalid address
#define AVR_SRAM_END (RAMEND + 1)

//! Last address of SRAM
#define AVR_SRAM_LAST (RAMEND)

//! SRAM available on AVR (in bytes) (4 KB)
#define AVR_MEMORY_SRAM (AVR_SRAM_END - AVR_SRAM_START)

//! EEPROM memory available on AVR (in bytes) (2 KB)
#define AVR_MEMORY_EEPROM (E2END + 1)

//! FLASH memory available on AVR (in bytes) (64 KB)
#define AVR_MEMORY_FLASH (FLASHEND + 1ul)

#endif
