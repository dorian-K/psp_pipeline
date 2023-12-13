#include "os_core.h"

#include "defines.h"
#include "lcd.h"
#include "os_input.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdio.h>

void os_initScheduler(void);

// variable for saving the original MCUSR so that we can examine it later
uint8_t savedMCUSR __attribute__((section(".noinit")));

/*! \file
 *
 * The main system core with initialization functions and error handling.
 *
 */

/*
 * Certain key-functionalities of SPOS do not work properly if optimization is
 * disabled (O0). It will still compile with O0, but you may encounter
 * run-time errors. This check is supposed to remind you on that.
 */
#ifndef __OPTIMIZE__
#error "Compiler optimizations disabled; SPOS and Testtasks may not work properly."
#endif

/*!
 *  This method automatically runs to handle special initialization conditions:
 *   - Saving the MCUSR for later examination
 *   - Disabling the watchdog timer in case it is enabled to keep the controller usable
 */
void os_preInit(void) __attribute__((naked)) __attribute__((section(".init3")));
void os_preInit(void) {
    savedMCUSR = MCUSR;
    MCUSR = 0; // reset the register

    wdt_disable(); // ensure watchdog timer is disabled
}

/*!
 * Examines the saved MCU status register and possibly prints an error if the reset source is not allowed.
 *
 * \param allowedSources Bitmask of reset sources which are accepted without pausing.
 */
void os_checkResetSource(uint8_t allowedSources) {
    lcd_line2();
    // JTag reset Register only set when AVR_Reset is recieved
    if (gbi(savedMCUSR, JTRF)) {
        lcd_writeProgString(PSTR("JT "));
    }
    // Watchdog reset register
    if (gbi(savedMCUSR, WDRF)) {
        lcd_writeProgString(PSTR("WATCHDOG "));
    }
    // Brown out detection register
    if (gbi(savedMCUSR, BORF)) {
        lcd_writeProgString(PSTR("BO "));
    }
    // External reset register (external reset button pressed)
    if (gbi(savedMCUSR, EXTRF)) {
        lcd_writeProgString(PSTR("EXT "));
    }
    // Power reset register mains power failed
    if (gbi(savedMCUSR, PORF)) {
        lcd_writeProgString(PSTR("POW"));
    }
    // The absence of a reset source indicates a software jump to the reset address
    if (!savedMCUSR) {
        lcd_writeProgString(PSTR("SOFT RESET"));
    }
    // Check if the reset source is allowed
    if (!(savedMCUSR & allowedSources)) {
        lcd_line1();
        lcd_writeProgString(PSTR("SYSTEM ERROR:   "));
        // not allowed sources must be confirmed by the user
        os_waitForInput();
        os_waitForNoInput();
    }
}

/*!
 *  Initializes the used timers.
 */
void os_init_timer(void) {
    // Init timer 2 (Scheduler)
    sbi(TCCR2A, WGM21); // Clear on timer compare match

    sbi(TCCR2B, CS22);   // Prescaler 1024  1
    sbi(TCCR2B, CS21);   // Prescaler 1024  1
    sbi(TCCR2B, CS20);   // Prescaler 1024  1
    sbi(TIMSK2, OCIE2A); // Enable interrupt
    OCR2A = 60;

    // Init timer 0 with prescaler 256
    cbi(TCCR0B, CS00);
    cbi(TCCR0B, CS01);
    sbi(TCCR0B, CS02);

    sbi(TIMSK0, TOIE0);
}

/*!
 *  Readies stack, scheduler and heap for first use. Additionally, the LCD is initialized. In order to do those tasks,
 *  it calls the sub function os_initScheduler().
 */
void os_init(void) {
    // Init timer 0 and 2
    os_init_timer();

    // Init buttons
    os_initInput();

    // Init LCD display
    lcd_init();
    stdout = lcdout;
    stderr = lcdout;

    lcd_writeProgString(PSTR("Booting SPOS ..."));
    os_checkResetSource(OS_ALLOWED_RESET_SOURCES);
    delayMs(DEFAULT_OUTPUT_DELAY * 20);

    os_initScheduler();

    os_systemTime_reset();
}

/*!
 *  Terminates the OS and displays a corresponding error on the LCD.
 *
 *  \param str  The error to be displayed
 */
void os_errorPStr(const char *str) {
#error IMPLEMENT STH. HERE
}
