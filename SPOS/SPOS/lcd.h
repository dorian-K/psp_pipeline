/*! \file
 *  \brief Library for LCD use.
 *
 *  Contains all the essential functionalities to comfortably work with the
 *  LCD on the evaluation board.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date 2013
 *  \version 2.0
 *  \ingroup lcd_group
 */

#ifndef _LCD_H
#define _LCD_H

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Set to 1 for SPOS starting from Versuch 2
#define SPOS_CONFIG 1

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

#if SPOS_CONFIG == 0

#define sbi(ADDRESS, BIT) (ADDRESS |= (1 << BIT))
#define cbi(ADDRESS, BIT) (ADDRESS &= ~(1 << BIT))

// Before SPOS we need to use system delay
#define delayMs(TIME) _delay_ms(TIME)
#else // Versuch 2+
#include "defines.h"
#include "util.h"
#endif

//! DDR of Port connected to LCD
#define LCD_PORT_DDR DDRA

//! PORT connected to LCD
#define LCD_PORT_DATA PORTA

//! PIN connected to LCD
#define LCD_PIN PINA

//! First value for init
#define LCD_INIT 0x03

//! Init mode
#define LCD_4BIT_MODE 0x02

//! Command to set one line display
#define LCD_ONE_LINE 0x20

//! Command to set one line display
#define LCD_TWO_LINES 0x28

//! Command to set 5x7 display
#define LCD_5X7 0x20

//! Command to set 5x10 display
#define LCD_5X10 0x24

//! Command to turn on LCD
#define LCD_DISPLAY_ON 0x0C

//! Command to turn off LCD
#define LCD_DISPLAY_OFF 0x08

//! Command to clear LCD
#define LCD_CLEAR 0x01

//! Command to disable address increment
#define LCD_NO_INC_ADDR 0x04

//! Command to enable address increment
#define LCD_INC_ADDR 0x06

//! Command to disable moving display context
#define LCD_NO_MOVE 0x04

//! Command to enable moving display context
#define LCD_MOVE 0x05

//! Command to jump to first line
#define LCD_LINE_1 0x80

//! Command to jump to second line
#define LCD_LINE_2 0xC0

//! Command to show the cursor on the display
#define LCD_SHOW_CURSOR 0x0B

//! Command to hide the cursor (default)
#define LCD_HIDE_CURSOR 0x08

//! Command to move the cursor to its start position
#define LCD_CURSOR_START 0x02

//! Used to calculate cursor position command for moving right
#define LCD_CURSOR_MOVE_R 0x80

//! Used to calculate cursor position command for moving left
#define LCD_CURSOR_MOVE_L 0x00

//! Used to calculate cursor position command for next row
#define LCD_NEXT_ROW 0x40

#define LCD_RS_PIN 4

#define LCD_EN_PIN 5

//! Timeout for the busy signal of the LCD
#define LCD_BUSY_TIMEOUT 2000

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------

//! Defines a custom char out of eight rows passed as integer values
#define CUSTOM_CHAR(cc0, cc1, cc2, cc3, cc4, cc5, cc6, cc7) 0 | (((uint64_t)(cc0)) << 0 * 8) | (((uint64_t)(cc1)) << 1 * 8) | (((uint64_t)(cc2)) << 2 * 8) | (((uint64_t)(cc3)) << 3 * 8) | (((uint64_t)(cc4)) << 4 * 8) | (((uint64_t)(cc5)) << 5 * 8) | (((uint64_t)(cc6)) << 6 * 8) | (((uint64_t)(cc7)) << 7 * 8)

//----------------------------------------------------------------------------
// Custom Chars
//----------------------------------------------------------------------------

// Note: 8===0
#define LCD_CC_IXI 0
#define LCD_CC_IXI_BITMAP (CUSTOM_CHAR( \
    0x00,                               \
    0x00,                               \
    0x01,                               \
    0x00,                               \
    0x15,                               \
    0x09,                               \
    0x15,                               \
    0x00                                \
))

// Note: 9===1
#define LCD_CC_TILDE 1
#define LCD_CC_TILDE_BITMAP (CUSTOM_CHAR( \
    0x00,                                 \
    0x08,                                 \
    0x15,                                 \
    0x02,                                 \
    0x00,                                 \
    0x00,                                 \
    0x00,                                 \
    0x00                                  \
))

#define LCD_CC_BACKSLASH 2
#define LCD_CC_BACKSLASH_BITMAP (CUSTOM_CHAR( \
    0x00,                                     \
    0x10,                                     \
    0x08,                                     \
    0x04,                                     \
    0x02,                                     \
    0x01,                                     \
    0x00,                                     \
    0x00                                      \
))

#define LCD_CC_MU 3
#define LCD_CC_MU_BITMAP (CUSTOM_CHAR( \
    0x00,                              \
    0x00,                              \
    0x09,                              \
    0x09,                              \
    0x09,                              \
    0x0F,                              \
    0x08,                              \
    0x00                               \
))

//----------------------------------------------------------------------------
// Function headers and global variables
//----------------------------------------------------------------------------

//! LCD output stream
extern FILE *lcdout;

//! Initialize LCD
void lcd_init(void);

//! Next output will be written to line 1
void lcd_line1(void);

//! Next output will be written to line 2
void lcd_line2(void);

//! Next output will be written at (row, column)
void lcd_goto(unsigned char row, unsigned char column);

//! Next output will be written at (currRow+row, currCol+column)
void lcd_move(char row, char column);

//! Go back one character (does not remove the character)
void lcd_back(void);

//! Go to the first character in line
void lcd_home(void);

//! Go one character right (does not remove the character)
void lcd_forward(void);

//! Send command to LCD
void lcd_command(uint8_t command);

//! Clear all data from display
void lcd_clear(void);

//! Erases one line
void lcd_erase(uint8_t line);

//! Write one character
void lcd_writeChar(char character);

//! Write a half-byte (a nibble)
void lcd_writeHexNibble(uint8_t number);

//! Write one hexadecimal byte
void lcd_writeHexByte(uint8_t number);

//! Write one hexadecimal word
void lcd_writeHexWord(uint16_t number);

//! Write one hexadecimal word without prefixes
void lcd_writeHex(uint16_t number);

//! Write a byte as a decimal number without prefixes
void lcd_writeDec(uint16_t number);

//! Write text string
void lcd_writeString(const char *text);

//! Write char PROGMEM* string
void lcd_writeProgString(const char *string);

//! Write char PROGMEM* string as an error
void lcd_writeErrorProgString(const char *string);

//! Write a draw bar
void lcd_drawBar(uint8_t percent);

//! Register a custom designed character with the LCD.
void lcd_registerCustomChar(uint8_t addr, uint64_t chr);

//! Write a 32 bit number
void lcd_write32bitHex(uint32_t number);

//! Write a voltage with valueUpperBound as float voltage with voltUpperBound
void lcd_writeVoltage(uint16_t voltage, uint16_t valueUpperBound, uint8_t voltUpperBound);

#endif
