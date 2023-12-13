//-------------------------------------------------
//          TestTask: Multiple
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


#define TEST_ASSERT(predicate, reason) \
    do ATOMIC {                        \
            if (!(predicate)) {        \
                TEST_FAILED(reason);   \
                HALT;                  \
            }                          \
        }                              \
    while (0)


#define TOTAL_SIBLINGS (5)               // How many siblings of program 2 should exist
#define MIN_SIBLING_APPEARANCE_COUNT (2) // How many times each sibling must appear
#define TEST_TIMEOUT_SECONDS (2 * 60)    // 2 minutes

volatile uint8_t siblings = 0;
volatile uint8_t siblingAppearanceCounters[TOTAL_SIBLINGS] = {};

void checkTimeout(Time startTime) {
    ATOMIC {
        TEST_ASSERT(os_systemTime_coarse() - startTime < TIME_S_TO_MS(TEST_TIMEOUT_SECONDS), "Timeout");
    }
}

void checkState(void) {
    ATOMIC {
        // Check running states of processes
        for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++) {
            if (pid == os_getCurrentProc()) {
                // The current process should have state OS_PS_RUNNING
                TEST_ASSERT(os_getProcessSlot(pid)->state == OS_PS_RUNNING, "Current   proc not running");
            } else {
                // No other process besides ourselves should have the state OS_PS_RUNNING
                TEST_ASSERT(os_getProcessSlot(pid)->state != OS_PS_RUNNING, "Other proc is running");
            }
        }
    }
}

REGISTER_AUTOSTART(program_multiple)
void program_multiple(void) {
    uint8_t whoami;
    ATOMIC {
        // Mark each instance with a unique number
        whoami = siblings++;

        // Check siblings count
        TEST_ASSERT(siblings <= TOTAL_SIBLINGS, "Too many started");
    }

    while (1) {
        ATOMIC {
            checkState();

            // Output identity
            lcd_writeChar(' ');
            lcd_writeChar('2');
            lcd_writeChar('a' + whoami);
            lcd_writeChar(';');

            // Increment counter for this sibling
            siblingAppearanceCounters[whoami]++;
            TEST_ASSERT(siblingAppearanceCounters[whoami] != 0xFF, "Other not started");
        }
        delayMs(DEFAULT_OUTPUT_DELAY * 5);
    }
}

/*!
 * Program that spawns several instances of the same
 * program
 */
REGISTER_AUTOSTART(singleton_program)
void singleton_program(void) {
    // Save the time for timeout
    Time startTime = os_systemTime_coarse();

    // Start 'TOTAL_SIBLINGS - 1' instances of program 2
    for (uint8_t x = 0; x < 3 * (TOTAL_SIBLINGS - 1); x++) {
        ATOMIC {
            // Output identity
            lcd_writeChar(' ');
            lcd_writeChar('1');

            // Spawn a new process every third time
            if (!(x % 3)) {
                os_exec(program_multiple, DEFAULT_PRIORITY);
                lcd_writeChar('!');
                lcd_writeChar(';');
            }

            checkTimeout(startTime);
        }
        delayMs(DEFAULT_OUTPUT_DELAY * 5);
    }

    while (1) {
        ATOMIC {
            checkState();

            // Check sibling appearence counters
            for (uint8_t x = 0; x <= TOTAL_SIBLINGS; x++) {
                if (x == TOTAL_SIBLINGS) {
                    TEST_PASSED;
                    HALT;
                }
                if (siblingAppearanceCounters[x] < MIN_SIBLING_APPEARANCE_COUNT) break;
            }

            checkTimeout(startTime);
        }

        lcd_writeChar(' ');
        lcd_writeChar('1');
        lcd_writeChar(';');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}
