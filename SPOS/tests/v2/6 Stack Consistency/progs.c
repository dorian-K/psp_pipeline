//-------------------------------------------------
//          TestTask: Stack Consistency
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>
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


#define TEST_ASSERT(predicate, reason) \
    do {                               \
        ATOMIC {                       \
            if (!(predicate)) {        \
                TEST_FAILED(reason);   \
                HALT;                  \
            }                          \
        }                              \
    } while (0)

// Check that SP has the expected value, but only if VERSUCH < 3 as this doesn't work with dispatcher
#define VALIDATE_SP(PID)                                                             \
    do {                                                                             \
        if (VERSUCH < 3) TEST_ASSERT(SP == PROCESS_STACK_BOTTOM(PID), "Invalid SP"); \
    } while (0)


static bool errflag = false;

static int stderrWrapper(const char c, FILE *stream) {
    errflag = true;
    putchar(c);
    return 0;
}

static FILE *wrappedStderr = &(FILE)FDEV_SETUP_STREAM(stderrWrapper, NULL, _FDEV_SETUP_WRITE);

void yielding_program(void) {
    VALIDATE_SP(2);

    // push some value so that there is no critical data (i.e. return address) at PROCESS_STACK_BOTTOM
    // this way, we can test for flips in PROCESS_STACK_BOTTOM without soft resetting
    __asm__ volatile("push r0");
    while (1) {
        TIMER2_COMPA_vect();
    }
}

REGISTER_AUTOSTART(main_program)
void main_program(void) {
    VALIDATE_SP(1);

    stderr = wrappedStderr;
    ProcessID pid = os_exec(yielding_program, DEFAULT_PRIORITY);
    Process *proc = os_getProcessSlot(pid);
    TEST_ASSERT(!errflag, "Errflag after init");
    errflag = false;

    // bitflip below SP is irrelevant
    *((uint8_t *)PROCESS_STACK_BOTTOM(pid) + 1) ^= 0x01;
    TIMER2_COMPA_vect();
    TEST_ASSERT(!errflag, "Bit below stack detected");
    errflag = false;

    // bitflip above SP is irrelevant
    *proc->sp.as_ptr ^= 0x01;
    TIMER2_COMPA_vect();
    TEST_ASSERT(!errflag, "Bit above stack detected");
    errflag = false;

    // two bitflips in the same position of different bytes
    // should not be possible to detected with our checksum algorithm
    *(proc->sp.as_ptr + 1) ^= 0x01;
    *(proc->sp.as_ptr + 2) ^= 0x01;
    TIMER2_COMPA_vect();
    TEST_ASSERT(!errflag, "Double bitflip detected");
    errflag = false;

    // single bitflip (top) must be detected
    // use topmost stack address as this does not lead to a soft reset
    lcd_clear();
    lcd_writeProgString(PSTR("Please confirm  "));
    lcd_writeProgString(PSTR("checksum error: "));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);
    *(proc->sp.as_ptr + 1) ^= 0x01;

    TIMER2_COMPA_vect();
    TEST_ASSERT(errflag, "Top Change not detected");
    errflag = false;


    // single bitflip (bottom) must be detected
    lcd_clear();
    lcd_writeProgString(PSTR("Please confirm  "));
    lcd_writeProgString(PSTR("checksum error: "));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);
    *((uint8_t *)PROCESS_STACK_BOTTOM(pid)) ^= 0x01;
    TIMER2_COMPA_vect();
    TEST_ASSERT(errflag, "Bottom Change not detected");


    ATOMIC {
        TEST_PASSED;
        HALT;
    }
}
