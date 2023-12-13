#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"

REGISTER_AUTOSTART(autostart_a);
void autostart_a(void) {
    while (1) {
        lcd_writeChar('A');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

REGISTER_AUTOSTART(autostart_b);
void autostart_b(void) {
    while (1) {
        lcd_writeChar('B');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

REGISTER_AUTOSTART(autostart_c);
void autostart_c(void) {
    while (1) {
        lcd_writeChar('C');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

REGISTER_AUTOSTART(autostart_d);
void autostart_d(void) {
    while (1) {
        lcd_writeChar('D');
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}
