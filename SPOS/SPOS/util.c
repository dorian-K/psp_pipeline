#include "util.h"

#include "atmega644constants.h"
#include "defines.h"
#include "lcd.h"
#include "os_core.h"
#include "os_input.h"

#include <avr/interrupt.h>
#include <avr/io.h>

/*! \file
 *
 * Contains basic utility functions used all over the system.
 *
 */

/*!
 * Variable to store TIMER0 overflows used to calculate wall-time in os_systemTime_[coarse|precise] functions.
 */
static Time os_systemTime_overflows = 0;

/*!
 * ISR that counts the number of occurred Timer 0 overflows for the os_systemTime_[coarse|precise] functions.
 */
ISR(TIMER0_OVF_vect) {
    os_systemTime_overflows++;
}

/*!
 * Function to reset os_systemTime_overflows to 0, effectively resetting the internal system time
 */
void os_systemTime_reset(void) {
    os_systemTime_overflows = 0;
}

/*!
 * Function that returns the current systemtime in ms based on the interrupt counts alone
 * (Difference ROUNDED after ~80s by 1s to os_systemTime_precise)
 *
 * \return The converted system time in ms
 */
Time os_systemTime_coarse(void) {
    /*! calculation performed:
     *   os_systemTime_overflows*1000                /(F_CPU/TC0_PRESCALER/256)
     *   timercounts            |   mult to get ms   | to get from freq to time
     */
    return os_systemTime_overflows * 1000 / (F_CPU / TC0_PRESCALER / 256);
}

/*!
 * Function augments os_systemTime_overflows to increase precision to approx 13 us (presc/f_cpu = 256/20MHz)
 *
 * \return os_systemTime_overflows scaled by cpu speed , timer prescaler as well as register size
 */
static Time os_systemTime_augment(void) {
    /*! in case Interrupts are off and the overflow flag is activated we simulate the overflow interrupt.
     *  The flag signalizes, that an overflow occurred. This would have been handled by the ISR immediately
     *  but since the interrupts are off, the controller will wait until they come back on. However,
     *  until that point yet another overflow could occur, which we would then be unable to detect.
     */

    if ((!(SREG & (1 << 7))) && (TIFR0 & (1 << TOV0))) {
        TIFR0 |= (1 << TOV0);
        os_systemTime_overflows++;
    }

    /*! here comes the actual job of this routine. We take the coarse system time which has a
     *   resolution of 3.3 ms and augment it with the current TCNT0 counter register, yielding
     *   a resolution of ~ 13 us.
     */
    return ((os_systemTime_overflows << 8) | TCNT0);
}

/*!
 * Function that returns the current systemtime in ms augmented by additional timer registers,
 * leading to higher accuracy at expense of performance. If not needed better use os_systemTime_coarse()
 * (Difference ROUNDED after ~80s by 1s to os_systemTime_coarse)
 *
 * \return The converted system time in ms augmented by TCNT0 counter register
 */
Time os_systemTime_precise(void) {
    /*! calculation performed:
     *  os_systemTime_augment()     *1000              /(F_CPU/TC0_PRESCALER/256)   / 256
     *  timercounts + TCNT0 counter | mult to get ms   | to get from freq to time    | to account for shift by os_systemTime_augment()
     *  this can be simplified to :
     */
    return (os_systemTime_augment() / (F_CPU / (TC0_PRESCALER * 1000ul)));
}


/*!
 *  Function that may be used to wait for specific time intervals.
 *  Therefore, we calculate the relative time to wait. This value is added to the current system time
 *  in order to get the destination time. Then, we wait until a temporary variable reaches this value.
 *
 *  \param ms  The time to be waited in milliseconds (max. 2^32 = 4294967296 ms ~= 7 weeks)
 */
void delayMs(Time ms) {
    Time startTime = os_systemTime_precise();
    Time destinationTime = startTime + ms;
    Time now;

    /*! We have to distinguish two cases:
     * 1.) If the startTime is smaller than the destinationTime, we can wait until the current time (= now)
     * is bigger than startTime but smaller than the destinationTime.
     * Keep waiting if the value of now is anywhere in the sections marked with +
     *    0 |----------------S+++++++++++++++D-------| max time

     * 2.) If we detect an overflow (startTime > destinationTime) then we have to wait for now reaching
     * maxtime (the overflow) and afterwards now reaching destinationTime
     * Keep waiting if now is anywhere in the sections marked with +
     *    0 |++++++++++++++++D---------------S+++++++| max time
     */
    if (startTime <= destinationTime) {
        do {
            now = os_systemTime_precise();
        } while ((startTime <= now) && (now < destinationTime));
    } else {
        do {
            now = os_systemTime_precise();
        } while ((now < destinationTime) || (startTime <= now));
    }
}

/*!
 *  Simple assertion function that is used to ensure specific behavior
 *  Note that there is a define assert(exp,errormsg) that simplifies the usage of this function.
 *
 *  \param exp The boolean expression that is expected to be true.
 *  \param errormsg The string that shall be shown as an error message if the given expression is false.
 *  \return True, iff the given expression is true
 */
bool assertPstr(bool exp, const char *errormsg) {
    if (!exp) {
        os_errorPStr(errormsg);
        return 0;
    }

    return 1;
}
