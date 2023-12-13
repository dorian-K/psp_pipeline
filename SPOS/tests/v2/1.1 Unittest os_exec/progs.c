//-------------------------------------------------
//          UnitTest: os_exec
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


#if VERSUCH >= 3
void os_dispatcher(void);
#endif

//! This is a dummy program to exec during testing
void infinite_loop(void) {
    HALT;
}

/*!
 * Unit test function which is declared with
 * "__attribute__ ((constructor))" such that it
 * is executed before the main function.
 */
void __attribute__((constructor)) test_os_exec() {
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
    lcd_writeProgString(PSTR("os_exec         "));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);
    lcd_clear();

    // Check that os_exec correctly returns INVALID_PROCESS when all process slots are in use
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++) os_getProcessSlot(pid)->state = OS_PS_READY;
    TEST_ASSERT(os_exec(infinite_loop, DEFAULT_PRIORITY) == INVALID_PROCESS, "Expected invalid process");

    // Clear slots again
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++)
        *os_getProcessSlot(pid) = (Process){0};

    // Check that os_exec returns INVALID_PROCESS for a non-existing program id
    TEST_ASSERT(os_exec(NULL, DEFAULT_PRIORITY) == INVALID_PROCESS, "Expected invalid process");

    // Clear slots again
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++)
        *os_getProcessSlot(pid) = (Process){0};

    // Check that 8 processes can be scheduled, and that the first free slot is always used
    for (ProcessID expected_pid = 0; expected_pid < MAX_NUMBER_OF_PROCESSES; expected_pid++) {
        // two assertions (not invalid and actually correct id) for better feedback
        ProcessID actual_pid = os_exec(infinite_loop, DEFAULT_PRIORITY);
        TEST_ASSERT(actual_pid != INVALID_PROCESS, "Expected valid PID");
        TEST_ASSERT(actual_pid == expected_pid, "Not first free slot");
    }

    // clear one process slot
    *os_getProcessSlot(2) = (Process){0};

    // Check that process slot 2 can be used.

    ProcessID gap_proc_pid = os_exec(infinite_loop, DEFAULT_PRIORITY);
    TEST_ASSERT(gap_proc_pid != INVALID_PROCESS, "Invalid PID with gap");
    TEST_ASSERT(gap_proc_pid == 2, "Incorrect PID with gap");

    // Clear slots again
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++)
        *os_getProcessSlot(pid) = (Process){0};


    /*
     * Initialize stack of process 0 and the stack below with a mask
     * so we can check after os_exec() if the registers were initialized
     * and other stacks were not overwritten
     */
    uint16_t stackBottomProcess0 = PROCESS_STACK_BOTTOM(0);
    for (uint8_t i = 0; i < 35; i++) {
        // Write to the stack of process 0: 1, 2, 3, ...
        os_getProcessSlot(0)->sp.as_int = stackBottomProcess0 - i;
        *(os_getProcessSlot(0)->sp.as_ptr) = i + 1;

        // Write to the stack below process 0 (scheduler stack): 1, 2, 3, ...
        os_getProcessSlot(0)->sp.as_int = stackBottomProcess0 + i + 1;
        *(os_getProcessSlot(0)->sp.as_ptr) = i + 1;
    }

    // Check that os_exec correctly initializes the process structure and stack and picks the correct slot
    TEST_ASSERT(os_exec(infinite_loop, 10) == 0, "PID not 0");
    TEST_ASSERT(os_getProcessSlot(0)->priority == 10, "Priority not 10");
    TEST_ASSERT(os_getProcessSlot(0)->program == infinite_loop, "Program pointer incorrect");
    TEST_ASSERT(os_getProcessSlot(0)->state == OS_PS_READY, "State not READY");
    TEST_ASSERT(os_getProcessSlot(0)->sp.as_int == (PROCESS_STACK_BOTTOM(0) - 35), "SP invalid");

    // Check that there are 33 zeros on the stack and the program function is written in the correct order on the stack
    for (uint8_t i = 1; i <= 33; i++) {
        TEST_ASSERT(os_getProcessSlot(0)->sp.as_ptr[i] == 0, "Non-zero for register");
    }
#if VERSUCH <= 2
    Program *program = os_getProcessSlot(0)->program;
#else
    Program *program = os_dispatcher;
#endif
    TEST_ASSERT(os_getProcessSlot(0)->sp.as_ptr[34] == (uint16_t)program >> 8, "Invalid hi byte");
    TEST_ASSERT(os_getProcessSlot(0)->sp.as_ptr[35] == ((uint16_t)program & 0xFF), "Invalid lo byte");

    for (uint8_t i = 1; i <= 35; i++) {
        TEST_ASSERT(os_getProcessSlot(0)->sp.as_ptr[35 + i] == i, "Written into wrong stack");
    }

    ATOMIC {
        TEST_PASSED;
        HALT; // Note that the main function is not executed because of this eternal loop
    }
}
