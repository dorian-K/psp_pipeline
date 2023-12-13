//-------------------------------------------------
//          TestTask: Resume
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#if VERSUCH < 2
#error "Please fix the VERSUCH-define"
#endif

#ifndef WRITE
#define WRITE(str) lcd_writeProgString(PSTR(str))
#endif
#define TEST_PASSED                    \
    do ATOMIC {                        \
            lcd_clear();               \
            WRITE("  TEST PASSED   "); \
        }                              \
    while (0)
#define TEST_FAILED(reason)  \
    do ATOMIC {              \
            lcd_clear();     \
            WRITE("FAIL  "); \
            WRITE(reason);   \
        }                    \
    while (0)


#define TEST_TIMEOUT_SECONDS (3 * 60) // 3 minutes

volatile uint8_t i = 0;
volatile uint8_t iFlag = 0;
volatile uint8_t iPassed = 0;   // Program 1 and 3 have counted to at least 9
volatile uint8_t jExpected = 0; // The expected value for j in program 2. Must reach at least 'z'

/*!
 * Prog 1 and 3 take turns printing.
 * Doing some simple communication
 */
REGISTER_AUTOSTART(program1)
void program1(void) {
    while (1) {
        while (iFlag) continue;
        lcd_writeChar('0' + i);
        iFlag = 1;
        delayMs(DEFAULT_OUTPUT_DELAY * 5);
    }
}

/*!
 * Program that prints Characters in ascending order
 */
REGISTER_AUTOSTART(program2)
void program2(void) {
    // Declare register bound variable to test recovery of register values after scheduling
    register uint8_t j = 0;

    // Save the time for timeout
    Time startTime = os_systemTime_coarse();

    while (1) {
        lcd_writeChar('a' + j);
        j = (j + 1) % ('z' - 'a' + 1);

        ATOMIC {
            if (jExpected == 'z' - 'a' && iPassed) {
                TEST_PASSED;
                HALT;
            }
            // Check if j is the expected value
            jExpected++;
            if (jExpected != j) {
                TEST_FAILED("Prog 2 registers");
                HALT;
            }
            // Check timeout
            if (os_systemTime_coarse() - startTime >= TIME_S_TO_MS(TEST_TIMEOUT_SECONDS)) {
                TEST_FAILED("Timeout");
                HALT;
            }
        }

        delayMs(DEFAULT_OUTPUT_DELAY * 5);
    }
}

/*!
 * Prog 1 and 3 take turns printing.
 * Doing some simple communication.
 */
REGISTER_AUTOSTART(program3)
void program3(void) {
    uint8_t k;
    while (1) {
        if (iFlag) {
            k = i + 1;
            i = k % 10;
            iFlag = 0;
            lcd_writeChar(' ');
            if (i == 9) iPassed = 1;
        }
        delayMs(DEFAULT_OUTPUT_DELAY * 5);
    }
}
