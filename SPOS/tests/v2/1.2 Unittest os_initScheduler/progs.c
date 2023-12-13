//-------------------------------------------------
//          UnitTest: os_initScheduler
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/atomic.h>

#if VERSUCH < 2
#error "Please fix the VERSUCH-define"
#endif

#ifndef WRITE
#define WRITE(str) lcd_writeProgString(PSTR(str))
#endif
#define TEST_PASSED                    \
    do {                               \
        ATOMIC {                       \
            lcd_clear();               \
            WRITE("  TEST PASSED   "); \
        }                              \
    } while (0)
#define TEST_FAILED(reason)  \
    do {                     \
        ATOMIC {             \
            lcd_clear();     \
            WRITE("FAIL  "); \
            WRITE(reason);   \
        }                    \
    } while (0)


/*!
 * Prints an error if the given condition is false
 */
#define TEST_ASSERT(condition, message) \
    do {                                \
        if (!(condition)) ATOMIC {      \
                TEST_FAILED(message);   \
                HALT;                   \
            }                           \
    } while (0)

//! This is a dummy program
REGISTER_AUTOSTART(noop)
void noop(void) {}

/*!
 * Unit test function which is declared with
 * "__attribute__ ((constructor))" such that it
 * is executed before the main function.
 */
void __attribute__((constructor)) test_os_initScheduler() {
    os_init_timer();
    os_initInput();
    lcd_init();
    stdout = lcdout;
    stderr = lcdout;

    lcd_clear();
    lcd_writeProgString(PSTR("Booting Unittest"));
    os_checkResetSource(_BV(JTRF) | _BV(BORF) | _BV(EXTRF) | _BV(PORF));
    delayMs(DEFAULT_OUTPUT_DELAY * 6);

    lcd_clear();
    lcd_writeProgString(PSTR("    Unittest    "));
    lcd_writeProgString(PSTR("os_initScheduler"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);
    lcd_clear();

    // This is the function generated from the above REGISTER_AUTOSTART macro
    register_autostart_noop();

    os_initScheduler();

    // Check that idle and program 1 were started with the correct priority
    TEST_ASSERT(os_getProcessSlot(0)->state == OS_PS_READY, "Idle not ready");
    TEST_ASSERT(os_getProcessSlot(0)->priority == DEFAULT_PRIORITY, "Idle not default priority");
    TEST_ASSERT(os_getProcessSlot(1)->state == OS_PS_READY, "Program 1 not started");
    TEST_ASSERT(os_getProcessSlot(1)->priority == DEFAULT_PRIORITY, "Prog. 1 not default prio.");
    for (uint8_t i = 2; i < MAX_NUMBER_OF_PROCESSES; i++) {
        TEST_ASSERT(os_getProcessSlot(i)->state == OS_PS_UNUSED, "Other slots not unused");
    }

    ATOMIC {
        TEST_PASSED;
        HALT; // Note that the main function is not executed because of this eternal loop
    }
}
