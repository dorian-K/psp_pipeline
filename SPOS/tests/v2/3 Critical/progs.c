//-------------------------------------------------
//          TestTask: Critical
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <util/atomic.h>

#if VERSUCH < 2
#error "Please fix the VERSUCH-define"
#endif

//---- Adjust here what to test -------------------
#define PHASE_1 1
#define PHASE_2 1
#define PHASE_3 1
#define PHASE_4 1
//-------------------------------------------------

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


// Handy error message define
#define TEST_FAILED_AND_HALT(reason) \
    do {                             \
        ATOMIC {                     \
            TEST_FAILED(reason);     \
            HALT;                    \
        }                            \
    } while (0)

// Handy assertion define
#define TEST_ASSERT(condition, message)        \
    do {                                       \
        if (!(condition)) ATOMIC {             \
                TEST_FAILED_AND_HALT(message); \
            }                                  \
    } while (0)

// Declarations used for checking correct error usages
static bool errflag = false;

static int stderrWrapper(const char c, FILE *stream) {
    errflag = true;
    putchar(c);
    return 0;
}
static FILE *wrappedStderr = &(FILE)FDEV_SETUP_STREAM(stderrWrapper, NULL, _FDEV_SETUP_WRITE);

#define EXPECT_ERROR(label)                            \
    do {                                               \
        lcd_clear();                                   \
        lcd_writeProgString(PSTR("Please confirm  ")); \
        lcd_writeProgString(PSTR(label ":"));          \
        delayMs(DEFAULT_OUTPUT_DELAY * 10);            \
        errflag = false;                               \
    } while (0)

#define ASSERT_ERROR(label)                               \
    do {                                                  \
        if (!errflag) {                                   \
            TEST_FAILED_AND_HALT("No error (" label ")"); \
        }                                                 \
    } while (0)


// Flag is used to signal to phase 1 that the button was pressed
static volatile bool flag;

// Acquire criticalSectionCount
extern uint8_t criticalSectionCount;

// Check for a pin change on Enter button
ISR(PCINT2_vect) {
    flag = true;
}


void dummy_program(void) {
    TEST_FAILED_AND_HALT("Other process scheduled");
}

REGISTER_AUTOSTART(main_program)
void main_program(void) {
#if PHASE_1
    //-------------------------------------------------------------
    // Phase 1: Check if critical sections are wrongly implemented
    //          using the global interrupt enable bit
    //-------------------------------------------------------------
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 1"));
    lcd_line2();
    lcd_writeProgString(PSTR("Interrupt Bits"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    // Setup Pin Change interrupt for Enter button
    PCICR = (1 << PCIE2);
    PCMSK2 = (1 << PCINT16);

    os_enterCriticalSection();

    // Preserve original process states
    Process processes_backup[MAX_NUMBER_OF_PROCESSES];

    for (int i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        processes_backup[i] = *os_getProcessSlot(i);
    }

    os_exec(dummy_program, DEFAULT_PRIORITY);

    lcd_clear();
    lcd_writeProgString(PSTR("Please wait"));

    // if the scheduler is still active, Program PROG_PHASE_1_HELPER will indicate the error when its scheduled during this delay
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    flag = false;

    lcd_clear();
    lcd_writeProgString(PSTR("Press Enter"));

    // Wait 15 seconds or until button is pressed
    for (uint8_t i = 0; i < 15 * 1000 / 100; i++) {
        if (flag) break;
        lcd_goto(1, 14);

        uint8_t sec = 15 - i * 100 / 1000;
        if (sec < 10) lcd_writeChar(' ');
        lcd_writeDec(sec);
        lcd_writeChar('s');

        delayMs(100);
    }
    lcd_clear();

    // Check if button was either not pressed or pin change interrupt
    // was blocked by critical section
    TEST_ASSERT(flag, "No button press detected");

    lcd_writeProgString(PSTR("Phase 1 complete"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    // restore process array, this removes PROG_PHASE_1_HELPER from the process array allowing the testtask to continue
    for (int i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        *os_getProcessSlot(i) = processes_backup[i];
    }
    os_leaveCriticalSection();
#endif

#if PHASE_2
    //-------------------------------------------------------------
    // Phase 2: Check overflow/underflow detection
    //-------------------------------------------------------------
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 2"));
    lcd_line2();
    lcd_writeProgString(PSTR("Over-/Underflow"));

    delayMs(DEFAULT_OUTPUT_DELAY * 10);
    FILE *tmpStderr = stderr;
    stderr = wrappedStderr;
    {
        // set critical section count to max value
        criticalSectionCount = ~(uint8_t)0;
        EXPECT_ERROR("Overflow");
        // try to enter another critical section (this should fail)
        os_enterCriticalSection();
        ASSERT_ERROR("Overflow");

        // set critical section count to lowest value
        criticalSectionCount = 0;
        EXPECT_ERROR("Underflow");
        // try to leave a critical section (this should fail)
        os_leaveCriticalSection();
        ASSERT_ERROR("Underflow");
    }
    stderr = tmpStderr;
#endif

#if PHASE_3
    //-------------------------------------------------------------
    // Phase 3: Check correct save and restore of GIEB
    //-------------------------------------------------------------
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 3"));
    lcd_line2();
    lcd_writeProgString(PSTR("Restore GIEB 0"));

    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    // Zero out GIEB and check whether enterCriticalSection and leaveCriticalSection will keep it off
    cbi(SREG, SREG_I);

    os_enterCriticalSection();
    TEST_ASSERT(!gbi(SREG, SREG_I), "Enter Crit. set wrong GIEB");

    // If the assert passed, we still have GIEB 0 and expect it to stay that way
    os_leaveCriticalSection();
    TEST_ASSERT(!gbi(SREG, SREG_I), "Leave Crit. set wrong GIEB");

    // Enable Interrupts again for following tests
    sbi(SREG, SREG_I);

    lcd_clear();
    lcd_writeProgString(PSTR("Restore GIEB 1"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    // Set GIEB and check whether enter and leave will keep it on
    // Strictly speaking, this test is not that necessary, since
    // os_enterCriticalSection clearing the GIEB will already fail the Interrupt test
    // and os_leaveCriticalSection clearing the GIEB will most likely lead to fails in multiprocess tests
    // but it is the same code and gives nicer feedback
    os_enterCriticalSection();
    TEST_ASSERT(gbi(SREG, SREG_I), "Enter Crit. cleared GIEB");

    // If the assert passed, we still have GIEB 0 and expect it to stay that way
    os_leaveCriticalSection();
    TEST_ASSERT(gbi(SREG, SREG_I), "Leave Crit. cleared GIEB");

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 3 complete"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

#endif

#if PHASE_4
    //-------------------------------------------------------------
    // Phase 4: Check correct usage of critical sections in os_exec
    //-------------------------------------------------------------
    lcd_clear();
    lcd_writeProgString(PSTR("Phase 4"));
    lcd_line2();
    lcd_writeProgString(PSTR("Save os_exec"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    os_enterCriticalSection();

    // and occupy all slots
    for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        os_getProcessSlot(i)->state = OS_PS_READY;
    }

    // Check that os_exec leaves critical section when all process slots are in use
    // Here is program irrelevant as it cannot be executed anyway
    TEST_ASSERT(os_exec(dummy_program, DEFAULT_PRIORITY) == INVALID_PROCESS, "Error in os_exec");
    TEST_ASSERT(criticalSectionCount == 1, "Crit. section not closed");

    // Clear slots again
    for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        os_getProcessSlot(i)->state = OS_PS_UNUSED;
    }

    // Check that os_exec leaves critical section after non existing program
    TEST_ASSERT(os_exec(NULL, DEFAULT_PRIORITY) == INVALID_PROCESS, "Error in os_exec");
    TEST_ASSERT(criticalSectionCount == 1, "Crit. section not closed");

    lcd_clear();
    lcd_writeProgString(PSTR("Phase 4 complete"));
    delayMs(DEFAULT_OUTPUT_DELAY * 10);

    os_getProcessSlot(os_getCurrentProc())->state = OS_PS_READY;
    os_leaveCriticalSection();
#endif

    // Show PASSED message
    ATOMIC {
        TEST_PASSED;
        HALT;
    }
}
