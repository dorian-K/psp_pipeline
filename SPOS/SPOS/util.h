/*! \file
 *  \brief Little helpers that don't fit elsewhere.
 *
 *  Tools, Utilities and other useful stuff that we didn't bother to
 *  categorize.
 *
 *  \author  Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date    2013
 *  \version 2.0
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t Time;

#define TC0_PRESCALER 256

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Resets system time back to 0
void os_systemTime_reset(void);

//! Coarse system time in ms
Time os_systemTime_coarse(void);

//! Precise system time in ms
Time os_systemTime_precise(void);

//! Waits for some milliseconds
void delayMs(Time ms);

//! Simple assertion function that calls os_error if given expression is not true
bool assertPstr(bool exp, const char *errormsg);

//! Handy define to specify assert calls directly (without PSTR(..))
#define assert(exp, errormsg) assertPstr(exp, PSTR(errormsg))

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------

// general purpose macros
#define sbi(x, b) x |= (1 << (b))
#define cbi(x, b) x &= ~(1 << (b))
#define gbi(x, b) (((x) >> (b)) & 1)

// time related macros
#define TIME_MS_TO_S(s) (s / 1000ul)
#define TIME_MS_TO_M(s) (TIME_MS_TO_S(s) / 60ul)
#define TIME_MS_TO_H(s) (TIME_MS_TO_M(s) / 60ul)

#define TIME_S_TO_MS(s) (s * 1000ul)
#define TIME_M_TO_MS(m) (TIME_S_TO_MS(m * 60ul))
#define TIME_H_TO_MS(h) (TIME_M_TO_MS(h * 60ul))

//----------------------------------------------------------------------------
// Assembler macros
//----------------------------------------------------------------------------

/*!
 * \brief Saves the register context on the stack
 *
 * All registers are saved to the proc's stack. Please note, that all processes
 * need to have their own stack, because we support preemptive scheduling.
 * So collisions with other stacks or the heap must be dealt with before making
 * this call, or data may be lost, resulting in weird effects.
 */
#define saveContext()                              \
    __asm__ volatile(                              \
        "push  r31                           \n\t" \
        "in    r31, __SREG__                 \n\t" \
        "cli                                 \n\t" \
        "push  r31                           \n\t" \
        "push  r30                           \n\t" \
        "push  r29                           \n\t" \
        "push  r28                           \n\t" \
        "push  r27                           \n\t" \
        "push  r26                           \n\t" \
        "push  r25                           \n\t" \
        "push  r24                           \n\t" \
        "push  r23                           \n\t" \
        "push  r22                           \n\t" \
        "push  r21                           \n\t" \
        "push  r20                           \n\t" \
        "push  r19                           \n\t" \
        "push  r18                           \n\t" \
        "push  r17                           \n\t" \
        "push  r16                           \n\t" \
        "push  r15                           \n\t" \
        "push  r14                           \n\t" \
        "push  r13                           \n\t" \
        "push  r12                           \n\t" \
        "push  r11                           \n\t" \
        "push  r10                           \n\t" \
        "push  r9                            \n\t" \
        "push  r8                            \n\t" \
        "push  r7                            \n\t" \
        "push  r6                            \n\t" \
        "push  r5                            \n\t" \
        "push  r4                            \n\t" \
        "push  r3                            \n\t" \
        "push  r2                            \n\t" \
        "push  r1                            \n\t" \
        "clr   r1                            \n\t" \
        "push  r0                            \n\t" \
    );


/*!
 * \brief Restores the register context on the stack
 *
 * All registers are saved to the proc's stack. Please note, that all processes
 * need to have their own stack, because we support preemptive scheduling.
 * So collisions with other stacks or the heap must be dealt with before making
 * this call, or data may be lost, resulting in weird effects.
 */
#define restoreContext()                           \
    __asm__ volatile(                              \
        "pop  r0                             \n\t" \
        "pop  r1                             \n\t" \
        "pop  r2                             \n\t" \
        "pop  r3                             \n\t" \
        "pop  r4                             \n\t" \
        "pop  r5                             \n\t" \
        "pop  r6                             \n\t" \
        "pop  r7                             \n\t" \
        "pop  r8                             \n\t" \
        "pop  r9                             \n\t" \
        "pop  r10                            \n\t" \
        "pop  r11                            \n\t" \
        "pop  r12                            \n\t" \
        "pop  r13                            \n\t" \
        "pop  r14                            \n\t" \
        "pop  r15                            \n\t" \
        "pop  r16                            \n\t" \
        "pop  r17                            \n\t" \
        "pop  r18                            \n\t" \
        "pop  r19                            \n\t" \
        "pop  r20                            \n\t" \
        "pop  r21                            \n\t" \
        "pop  r22                            \n\t" \
        "pop  r23                            \n\t" \
        "pop  r24                            \n\t" \
        "pop  r25                            \n\t" \
        "pop  r26                            \n\t" \
        "pop  r27                            \n\t" \
        "pop  r28                            \n\t" \
        "pop  r29                            \n\t" \
        "pop  r30                            \n\t" \
        "pop  r31                            \n\t" \
        "out  __SREG__, r31                  \n\t" \
        "pop  r31                            \n\t" \
        "reti                                \n\t" \
    );


#define HALT \
    do {     \
    } while (1)

// Used in testtasks
#define ATOMIC ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

#endif
