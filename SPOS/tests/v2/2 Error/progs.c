//-------------------------------------------------
//          TestTask: Error
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
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
// Set this to 1 to check the button functionality
#define PHASE_BUTTONS 1
// Set this to 1 to check the os_error functionality
#define PHASE_ERROR 1
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
    do ATOMIC {                      \
            TEST_FAILED(reason);     \
            HALT;                    \
        }                            \
    while (0)

// Prints phase information to the LCD.
#define PHASE(n, name)                       \
    do {                                     \
        lcd_clear();                         \
        lcd_writeProgString(PSTR("Phase ")); \
        lcd_writeDec(n);                     \
        lcd_writeChar(':');                  \
        lcd_line2();                         \
        lcd_writeProgString(PSTR(name));     \
    } while (0)

// Prints OK with the phase message.
#define PHASE_SUCCESS(n, name)             \
    do {                                   \
        PHASE(n, name);                    \
        lcd_goto(2, 15);                   \
        lcd_writeProgString(PSTR("OK"));   \
        delayMs(8 * DEFAULT_OUTPUT_DELAY); \
    } while (0)

#define DO_ERROR(label)                                      \
    do {                                                     \
        errflag = false;                                     \
        stderr = wrappedStderr;                              \
        os_error("Confirm error   [" label "]");             \
        if (!errflag) TEST_FAILED_AND_HALT("Missing error"); \
    } while (0)


static bool errflag = false;

// Wrapper to be able to detect correct os_error implementation
static int stderrWrapper(const char c, FILE *stream) {
    errflag = true;
    putchar(c);
    return 0;
}

static FILE *wrappedStderr = &(FILE)FDEV_SETUP_STREAM(stderrWrapper, NULL, _FDEV_SETUP_WRITE);


// Variable to count the number of timer interrupts that occurred
static volatile uint16_t count = 0;

// Counts the number of interrupts that occured
ISR(TIMER1_COMPA_vect) {
    count++;
    if (count > 5) {
        TEST_FAILED_AND_HALT("Interrupted");
    }
}


REGISTER_AUTOSTART(program1)
void program1(void) {

#if PHASE_BUTTONS
    // 1. Checking registers for buttons
    PHASE(1, "Registers");
    delayMs(8 * DEFAULT_OUTPUT_DELAY);

    if ((DDRC & 0xC3) != 0) {
        TEST_FAILED_AND_HALT("DDR wrong");
    }

    if ((PORTC & 0xC3) != 0xC3) {
        TEST_FAILED_AND_HALT("No pull ups");
    }

    PHASE_SUCCESS(1, "Registers");

    // 2. Checking input-functions
    PHASE(2, "Button test");
    delayMs(8 * DEFAULT_OUTPUT_DELAY);

    for (uint8_t i = 1; i <= 4; i++) {
        // Test one button.
        // Test for the correct function of os_getInputs, os_waitForInput and os_waitForNoInput.
        // If an error occurs, an error message is shown.

        lcd_clear();
        lcd_writeProgString(PSTR("Press button "));
        lcd_writeDec(i);

        os_waitForInput();
        delayMs(50);

        if (os_getInput() == 0) {
            TEST_FAILED_AND_HALT("waitForInput");
        }
        if ((os_getInput() != 1 << (i - 1)) || ((PINC & 0b11000011) != (i < 3 ? (0b11000011 ^ 1 << (i - 1)) : (0b11000011 ^ 1 << (i + 3))))) {
            TEST_FAILED_AND_HALT("getInput");
        }

        lcd_clear();
        lcd_writeProgString(PSTR("Release button "));
        lcd_writeDec(i);

        os_waitForNoInput();
        delayMs(50);

        if ((os_getInput() != 0) || ((PINC & 0xC3) != 0xC3)) {
            TEST_FAILED_AND_HALT("waitForNoInput");
        }
    }

    PHASE_SUCCESS(2, "Button test");
#endif

#if PHASE_ERROR
    // 3. Checking if the global interrupt flag is disabled during os_error
    // Disable scheduler timer
    TCCR2B = 0;
    // Configure our check timer for CTC, 1024 prescaler
    TCCR1A = 0;
    TCCR1C = 0;
    OCR1A = 500;
    TIMSK1 |= (1 << OCIE1A);

    PHASE(3, "Interrupts");
    delayMs(8 * DEFAULT_OUTPUT_DELAY);

    TCCR1B = (1 << WGM12) | (1 << CS10);


    DO_ERROR("Interrupts");

    // At this point the error was confirmed and a correct os_error will reactivate
    // global interrupts, which leads to exactly one timer interrupt

    // Disable our check timer
    TCCR1B = 0;
    lcd_clear();

    // The test passes only when less or equal 5 interrupts were counted
    if (count <= 5) {
        PHASE_SUCCESS(3, "Interrupts");
    } else {
        TEST_FAILED_AND_HALT("Interrupted");
    }

    // Phase 4: Does os_error restore GIEB if it was activated?
    PHASE(4, "GIEB on");
    delayMs(8 * DEFAULT_OUTPUT_DELAY);

    // Enable global interrupts
    sei();

    // Throw an error
    DO_ERROR("GIEB on");

    // Error was confirmed, check if global interrupts are still on
    if (SREG & (1 << 7)) {
        PHASE_SUCCESS(4, "GIEB on");
    } else {
        cli();
        TEST_FAILED_AND_HALT("GIEB falsely off");
    }


    // Phase 5: Does os_error restore GIEB if it was deactivated?
    PHASE(5, "GIEB off");
    delayMs(8 * DEFAULT_OUTPUT_DELAY);

    // Disable global interrupts
    cli();

    // Throw an error
    DO_ERROR("GIEB off");

    // Error was confirmed, check if global interrupts are still off
    if (SREG & (1 << 7)) {
        TEST_FAILED_AND_HALT("GIEB falsely on");
    } else {
        PHASE_SUCCESS(5, "GIEB off");
    }
#endif

    ATOMIC {
        TEST_PASSED;
        HALT;
    }
}
