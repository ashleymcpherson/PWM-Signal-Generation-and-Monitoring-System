//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//
// ----------------------------------------------------------------------------
#include <stdio.h>
#include "diag/Trace.h"
#include <string.h>
#include "cmsis/cmsis_device.h"
// ----------------------------------------------------------------------------
//
// STM32F0 led blink sample (trace via $(trace)).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// To demonstrate POSIX retargetting, reroute the STDOUT and STDERR to the
// trace device and display messages on both of them.
//
// Then demonstrates how to blink a led with 1Hz, using a
// continuous loop and SysTick delays.
//
// On DEBUG, the uptime in seconds is also displayed on the trace device.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the $(trace) output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//
// The external clock frequency is specified as a preprocessor definition
// passed to the compiler via a command line option (see the 'C/C++ General' ->
// 'Paths and Symbols' -> the 'Symbols' tab, if you want to change it).
// The value selected during project creation was HSE_VALUE=48000000.
//
/// Note: The default clock settings take the user defined HSE_VALUE and try
// to reach the maximum possible system clock. For the default 8MHz input
// the result is guaranteed, but for other values it might not be possible,
// so please adjust the PLL settings in system/src/cmsis/system_stm32f0xx.c
//
// ----- main() ---------------------------------------------------------------
// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Maximum possible setting for overflow */
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)
/*** This is partial code for accessing LED Display via SPI interface. ***/
//...
volatile unsigned int Freq = 0;  // Example: measured frequency value (global variable)
volatile unsigned int Res = 0;   // Example: measured resistance value (global variable)
volatile int globalSignal = 1; // Toggles the states
volatile uint8_t timerTriggered = 0;
void oled_Write(unsigned char);
void oled_Write_Cmd(unsigned char);
void oled_Write_Data(unsigned char);
void oled_config(void);
void refresh_OLED(void);
SPI_HandleTypeDef SPI_Handle;
//
// LED Display initialization commands
//
unsigned char oled_init_cmds[] =
{
0xAE,
0x20, 0x00,
0x40,
0xA0 | 0x01,
0xA8, 0x40 - 1,
0xC0 | 0x08,
0xD3, 0x00,
0xDA, 0x32,
0xD5, 0x80,
0xD9, 0x22,
0xDB, 0x30,
0x81, 0xFF,
0xA4,
0xA6,
0xAD, 0x30,
0x8D, 0x10,
0xAE | 0x01,
0xC0,
0xA0
}
//
// Character specifications for LED Display (1 row = 8 bytes = 1 ASCII character)
// Example: to display '4', retrieve 8 data bytes stored in Characters[52][X] row
//          (where X = 0, 1, ..., 7) and send them one by one to LED Display.
// Row number = character ASCII code (e.g., ASCII code of '4' is 0x34 = 52)
//
unsigned char Characters[][8] = {
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
 {0b00000000, 0b00000000, 0b01011111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // !
 {0b00000000, 0b00000111, 0b00000000, 0b00000111, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // "
 {0b00010100, 0b01111111, 0b00010100, 0b01111111, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // #
 {0b00100100, 0b00101010, 0b01111111, 0b00101010, 0b00010010,0b00000000, 0b00000000, 0b00000000},  // $
 {0b00100011, 0b00010011, 0b00001000, 0b01100100, 0b01100010,0b00000000, 0b00000000, 0b00000000},  // %
 {0b00110110, 0b01001001, 0b01010101, 0b00100010, 0b01010000,0b00000000, 0b00000000, 0b00000000},  // &
 {0b00000000, 0b00000101, 0b00000011, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // '
 {0b00000000, 0b00011100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // (
 {0b00000000, 0b01000001, 0b00100010, 0b00011100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // )
 {0b00010100, 0b00001000, 0b00111110, 0b00001000, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // *
 {0b00001000, 0b00001000, 0b00111110, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // +
 {0b00000000, 0b01010000, 0b00110000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // ,
 {0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // -
 {0b00000000, 0b01100000, 0b01100000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // .
 {0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010,0b00000000, 0b00000000, 0b00000000},  // /
 {0b00111110, 0b01010001, 0b01001001, 0b01000101, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // 0
 {0b00000000, 0b01000010, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // 1
 {0b01000010, 0b01100001, 0b01010001, 0b01001001, 0b01000110,0b00000000, 0b00000000, 0b00000000},  // 2
 {0b00100001, 0b01000001, 0b01000101, 0b01001011, 0b00110001,0b00000000, 0b00000000, 0b00000000},  // 3
 {0b00011000, 0b00010100, 0b00010010, 0b01111111, 0b00010000,0b00000000, 0b00000000, 0b00000000},  // 4
 {0b00100111, 0b01000101, 0b01000101, 0b01000101, 0b00111001,0b00000000, 0b00000000, 0b00000000},  // 5
 {0b00111100, 0b01001010, 0b01001001, 0b01001001, 0b00110000,0b00000000, 0b00000000, 0b00000000},  // 6
 {0b00000011, 0b00000001, 0b01110001, 0b00001001, 0b00000111,0b00000000, 0b00000000, 0b00000000},  // 7
 {0b00110110, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000},  // 8
 {0b00000110, 0b01001001, 0b01001001, 0b00101001, 0b00011110,0b00000000, 0b00000000, 0b00000000},  // 9
 {0b00000000, 0b00110110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // :
 {0b00000000, 0b01010110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // ;
 {0b00001000, 0b00010100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // <
 {0b00010100, 0b00010100, 0b00010100, 0b00010100, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // =
 {0b00000000, 0b01000001, 0b00100010, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // >
 {0b00000010, 0b00000001, 0b01010001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000},  // ?
 {0b00110010, 0b01001001, 0b01111001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // @
 {0b01111110, 0b00010001, 0b00010001, 0b00010001, 0b01111110,0b00000000, 0b00000000, 0b00000000},  // A
 {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000},  // B
 {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00100010,0b00000000, 0b00000000, 0b00000000},  // C
 {0b01111111, 0b01000001, 0b01000001, 0b00100010, 0b00011100,0b00000000, 0b00000000, 0b00000000},  // D
 {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001,0b00000000, 0b00000000, 0b00000000},  // E
 {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // F
 {0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b01111010,0b00000000, 0b00000000, 0b00000000},  // G
 {0b01111111, 0b00001000, 0b00001000, 0b00001000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // H
 {0b01000000, 0b01000001, 0b01111111, 0b01000001, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // I
 {0b00100000, 0b01000000, 0b01000001, 0b00111111, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // J
 {0b01111111, 0b00001000, 0b00010100, 0b00100010, 0b01000001,0b00000000, 0b00000000, 0b00000000},  // K
 {0b01111111, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // L
 {0b01111111, 0b00000010, 0b00001100, 0b00000010, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // M
 {0b01111111, 0b00000100, 0b00001000, 0b00010000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // N
 {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // O
 {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000},  // P
 {0b00111110, 0b01000001, 0b01010001, 0b00100001, 0b01011110,0b00000000, 0b00000000, 0b00000000},  // Q
 {0b01111111, 0b00001001, 0b00011001, 0b00101001, 0b01000110,0b00000000, 0b00000000, 0b00000000},  // R
 {0b01000110, 0b01001001, 0b01001001, 0b01001001, 0b00110001,0b00000000, 0b00000000, 0b00000000},  // S
 {0b00000001, 0b00000001, 0b01111111, 0b00000001, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // T
 {0b00111111, 0b01000000, 0b01000000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000},  // U
 {0b00011111, 0b00100000, 0b01000000, 0b00100000, 0b00011111,0b00000000, 0b00000000, 0b00000000},  // V
 {0b00111111, 0b01000000, 0b00111000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000},  // W
 {0b01100011, 0b00010100, 0b00001000, 0b00010100, 0b01100011,0b00000000, 0b00000000, 0b00000000},  // X
 {0b00000111, 0b00001000, 0b01110000, 0b00001000, 0b00000111,0b00000000, 0b00000000, 0b00000000},  // Y
 {0b01100001, 0b01010001, 0b01001001, 0b01000101, 0b01000011,0b00000000, 0b00000000, 0b00000000},  // Z
 {0b01111111, 0b01000001, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // [
 {0b00010101, 0b00010110, 0b01111100, 0b00010110, 0b00010101,0b00000000, 0b00000000, 0b00000000},  // back slash
 {0b00000000, 0b00000000, 0b00000000, 0b01000001, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // ]
 {0b00000100, 0b00000010, 0b00000001, 0b00000010, 0b00000100,0b00000000, 0b00000000, 0b00000000},  // ^
 {0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // _
 {0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // `
 {0b00100000, 0b01010100, 0b01010100, 0b01010100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // a
 {0b01111111, 0b01001000, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000},  // b
 {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // c
 {0b00111000, 0b01000100, 0b01000100, 0b01001000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // d
 {0b00111000, 0b01010100, 0b01010100, 0b01010100, 0b00011000,0b00000000, 0b00000000, 0b00000000},  // e
 {0b00001000, 0b01111110, 0b00001001, 0b00000001, 0b00000010,0b00000000, 0b00000000, 0b00000000},  // f
 {0b00001100, 0b01010010, 0b01010010, 0b01010010, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // g
 {0b01111111, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // h
 {0b00000000, 0b01000100, 0b01111101, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // i
 {0b00100000, 0b01000000, 0b01000100, 0b00111101, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // j
 {0b01111111, 0b00010000, 0b00101000, 0b01000100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // k
 {0b00000000, 0b01000001, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // l
 {0b01111100, 0b00000100, 0b00011000, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // m
 {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // n
 {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000},  // o
 {0b01111100, 0b00010100, 0b00010100, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // p
 {0b00001000, 0b00010100, 0b00010100, 0b00011000, 0b01111100,0b00000000, 0b00000000, 0b00000000},  // q
 {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // r
 {0b01001000, 0b01010100, 0b01010100, 0b01010100, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // s
 {0b00000100, 0b00111111, 0b01000100, 0b01000000, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // t
 {0b00111100, 0b01000000, 0b01000000, 0b00100000, 0b01111100,0b00000000, 0b00000000, 0b00000000},  // u
 {0b00011100, 0b00100000, 0b01000000, 0b00100000, 0b00011100,0b00000000, 0b00000000, 0b00000000},  // v
 {0b00111100, 0b01000000, 0b00111000, 0b01000000, 0b00111100,0b00000000, 0b00000000, 0b00000000},  // w
 {0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100,0b00000000, 0b00000000, 0b00000000},  // x
 {0b00001100, 0b01010000, 0b01010000, 0b01010000, 0b00111100,0b00000000, 0b00000000, 0b00000000},  // y
 {0b01000100, 0b01100100, 0b01010100, 0b01001100, 0b01000100,0b00000000, 0b00000000, 0b00000000},  // z
 {0b00000000, 0b00001000, 0b00110110, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // {
 {0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // |
 {0b00000000, 0b01000001, 0b00110110, 0b00001000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // }
 {0b00001000, 0b00001000, 0b00101010, 0b00011100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // ~
 {0b00001000, 0b00011100, 0b00101010, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000}   // <-
};


void SystemClock48MHz( void )
{
	//
	// Disable the PLL
	//
	RCC->CR &= ~(RCC_CR_PLLON);
	//
	// Wait for the PLL to unlock
	//
	while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );
	//
	// Configure the PLL for a 48MHz system clock
	//
	RCC->CFGR = 0x00280000;
	//
	// Enable the PLL
	//
	RCC->CR |= RCC_CR_PLLON;
	//
	// Wait for the PLL to lock
	//
	while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );
	//
	// Switch the processor to the PLL clock source
	//
	RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
	//
	// Update the system with the new clock frequency
	//
	SystemCoreClockUpdate();
}


void ADC_initialize()
{
	//Enable the ADC clock
	RCC->APB2ENR |= RCC_APB2ENR_ADCEN; //Enable clock for ADC with macro
	RCC->APB2ENR |= (1 << 9); //Sets bit 9, same effect as above but without a macro

	//Set Port A pin 5 to analog mode
	GPIOA->MODER |= (3 << (2 * 5)); //SET PA5(ADC_IN5) as an analog mode pin

	//Step 1, ADC->CFGR1 (page 10 of example PDF)
	ADC1->CFGR1 &= ~(0b11000); //Clears Bits[4:3] which creates 12 bit resolution
	ADC1->CFGR1 &= ~(0b100000); //Clears Bit[5] which creates right alignment data
	ADC1->CFGR1 |= 0b1000000000000; //Sets bit 12 which means contents are overwritten when overrun is detected
	ADC1->CFGR1 |= 0b10000000000000; //Sets bit 13 which sets continuous conversion mode

	//Step 2, Channel select register
	ADC1->CHSELR |= (1 << 5); //Set bit 5 to 1 to indicate ADC is channel 5

	//Step 3, Set the sampling time register
	ADC1->SMPR |= 0b111; //This allows as many clock cycles as needed

	//Step 4, Set control register
	ADC1->CR |= 0b1; //Enable the ADC process

	//THE BASIC INITIALIZATION OF ADC IS COMPLETE

	//Make it wait for the ADC1 -> ISR[0] to become 1
	while(!(ADC1->ISR & 0b1)); //Stuck on this line until ADC1->ISR[0] becomes 1 - This is the ADC ready flag
}


void DAC_initialize()
{
	//Enable the clock for DAC
	RCC->APB1ENR |= (1 << 29); //Sets bit 29

	//Set Port A pin 4 to analog mode
	GPIOA->MODER |= (3 << (2 * 4)); //SET PA4 to analog mode for the DAC

	DAC -> CR |= 0b1; // Enable DAC channel 1
	DAC -> CR &= ~(0b10); //Enable DAC channel tri-state buffer
	DAC -> CR &= ~(0b100); //Disable channel 1 trigger enable
}


void myTIM2_Init()
{
	/* Enable clock for TIM2 peripheral */
	// Relevant register: RCC->APB1ENR
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	/* Configure TIM2: buffer auto-reload, count up, stop on overflow,
	 * enable update events, interrupt on overflow only */
	// Relevant register: TIM2->CR1
	TIM2->CR1 = ((uint16_t)0x008C);

	/* Set clock prescaler value */
	//TIM2->PSC = myTIM2_PRESCALER;
	TIM2->PSC = ((uint16_t)0x0000);

	/* Set auto-reloaded delay */
	TIM2->ARR = myTIM2_PERIOD;

	/* Update timer registers */
	// Relevant register: TIM2->EGR
	TIM2->EGR = ((uint16_t)0x0001);

	/* Assign TIM2 interrupt priority = 0 in NVIC */
	// Relevant register: NVIC->IP[3], or use NVIC_SetPriority
	NVIC_SetPriority(TIM2_IRQn, 2);

	/* Enable TIM2 interrupts in NVIC */
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	//NVIC_EnableIRQ(TIM2_IRQn);

	/* Enable update interrupt generation */
	// Relevant register: TIM2->DIER
	TIM2->DIER |= TIM_DIER_UIE;

	/* Start Counting Timer Pulses*/
	TIM2-> CR1 |= TIM_CR1_CEN;
}


void myTIM3_Init()
{
	/* Enable clock for TIM2 peripheral */
	// Relevant register: RCC->APB1ENR
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	/* Configure TIM2: buffer auto-reload, count up, stop on overflow,
	 * enable update events, interrupt on overflow only */
	// Relevant register: TIM2->CR1
	TIM3->CR1 = ((uint16_t)0x008C);
	/* Set clock prescaler value */
	//TIM2->PSC = myTIM2_PRESCALER;
	TIM3->PSC = ((uint16_t)0x0000);
	/* Set auto-reloaded delay */
	TIM3->ARR = myTIM2_PERIOD;
	//TIM2->ARR = ((uint32_t)12000000);
	/* Update timer registers */
	// Relevant register: TIM2->EGR
	//TIM2->EGR = ((uint16_t)0x001);
	TIM3->EGR = ((uint16_t)0x0001);
	/* Assign TIM2 interrupt priority = 0 in NVIC */
	// Relevant register: NVIC->IP[3], or use NVIC_SetPriority
	NVIC_SetPriority(TIM3_IRQn, 4);
	/* Enable TIM2 interrupts in NVIC */
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	//NVIC_EnableIRQ(TIM2_IRQn);
	/* Enable update interrupt generation */
	// Relevant register: TIM2->DIER
	TIM3->DIER |= TIM_DIER_UIE;
	/* Start Counting Timer Pulses*/
	TIM3-> CR1 |= TIM_CR1_CEN;
}


void EXTI0_1_Init()
{
	GPIOA->MODER &= ~(GPIO_MODER_MODER0); //Sets Port A Pin 0 as input which corresponds to USER button
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA; //Map EXTI0 line to PA0
	EXTI->RTSR |= EXTI_RTSR_TR0; //Sensitive to rising edges
	EXTI->IMR |= EXTI_IMR_MR0; // Unmasks interrupts from EXTI0 line
	NVIC_SetPriority(EXTI0_1_IRQn, 0); //Sets priority to 0 in NVIC
	NVIC_EnableIRQ(EXTI0_1_IRQn); //Enables EXTI0 interrupts in NVIC
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PA; //Map EXTI1 line to PA1
	EXTI->RTSR |= EXTI_RTSR_TR1; // Sensitive to rising edges on EXTI1
	EXTI->IMR |= EXTI_IMR_MR1;   // Unmask interrupts from EXTI1 line
	EXTI->IMR &= ~EXTI_IMR_MR1; //Disable interrupt for EXTI1
	EXTI->IMR |= EXTI_IMR_MR2;  //Enable interrupt for EXTI2
}


void myEXTI2_3_Init()
{
	/* Map EXTI2 line to PA2 */
	// Relevant register: SYSCFG->EXTICR[0]
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PA;

	/* EXTI2 line interrupts: set rising-edge trigger */
	// Relevant register: EXTI->RTSR
	EXTI->RTSR |= EXTI_RTSR_TR2;

	/* Unmask interrupts from EXTI2 line */
	// Relevant register: EXTI->IMR
	EXTI->IMR |= EXTI_IMR_MR2;

	/* Assign EXTI2 interrupt priority = 0 in NVIC */
	// Relevant register: NVIC->IP[2], or use NVIC_SetPriority
	NVIC_SetPriority(EXTI2_3_IRQn, 1);

	/* Enable EXTI2 interrupts in NVIC */
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	NVIC_EnableIRQ(EXTI2_3_IRQn);
}


void EXTI2_3_IRQHandler()
{
	// Declare/initialize your local variables here...
	double float_frequency;
	double float_scaledPeriod;
	unsigned int frequency;
	unsigned int scaledPeriod;

	/* Check if EXTI2 interrupt pending flag is indeed set */
	if ((EXTI->PR & EXTI_PR_PR2) != 0)
	{
		//
		// 1. If this is the first edge:
		//	- Clear count register (TIM2->CNT).
		//	- Start timer (TIM2->CR1).
    if(!timerTriggered)
    {
        TIM2->CNT = 0;
        TIM2->CR1 |= TIM_CR1_CEN;
        timerTriggered = 1;
    }
		//    Else (this is the second edge):
		//	- Stop timer (TIM2->CR1).
		//	- Read out count register (TIM2->CNT).
		//	- Calculate signal period and frequency.
		//	- Print calculated values to the console.
		//	  NOTE: Function trace_printf does not work
		//	  with floating-point numbers: you must use
		//	  "unsigned int" type to print your signal
		//	  period and frequency.
    else
    {
        TIM2->CR1 &= ~(TIM_CR1_CEN);
        uint32_t timerValue = TIM2->CNT;
        float_frequency = 48000000 / timerValue;
        float_scaledPeriod = 1/float_frequency;
        frequency = float_frequency;
        Freq = float_frequency;

        Res = 0;
        timerTriggered = 0;
    }
		//
		// 2. Clear EXTI2 interrupt pending flag (EXTI->PR).
		// NOTE: A pending register (PR) bit is cleared
		// by writing 1 to it.
		//
    EXTI->PR |= EXTI_PR_PR2;
	}
}


void myGPIOA_Init()
{
	/* Enable clock for GPIOA peripheral */
	// Relevant register: RCC->AHBENR
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	/* Configure PA4 as output */
	// Relevant register: GPIOA->MODER
	GPIOA->MODER |= (0b01 << (2*4));

	/* Ensure no pull-up/pull-down for PA4 */
	// Relevant register: GPIOA->PUPDR
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR4);

	//Configure PA1 as input
	GPIOA->MODER &= ~(0b11 << 2 *1);

	//No pull-up, no pull-down
	GPIOA->PUPDR &= ~(0b11 << (2 * 1));
}


int main(int argc, char* argv[])
{
	SystemClock48MHz();
	RCC->AHBENR |= (1 << 0); // Enable clock for GPIOA
	myGPIOA_Init();
	ADC_initialize();
	DAC_initialize();
	myTIM2_Init();
	myTIM3_Init();
	oled_config();
	EXTI0_1_Init();
	myEXTI2_3_Init();


	while (1)
	{
		ADC1->CR |= ADC_CR_ADSTART; //Sets bit 2, starts ADC conversion process
		while(!(ADC1-> ISR & ADC_ISR_EOS_Msk)); // Wait for conversion to finish
		unsigned int val = ADC1->DR; //Val is between 0 and 4095
		//trace_printf("ADC Value: %d\n", val);
		refresh_OLED();
		DAC->DHR12R1 = val;
		//trace_printf("DAC Value: %d\n", converted_val);
	}
}


//
// LED Display Functions
//
void refresh_OLED( void )
{
	// Buffer size = at most 16 characters per PAGE + terminating '\0'
	unsigned char Buffer[17];

	//Line 1:
	snprintf( Buffer, sizeof( Buffer ), "R: %5u Ohms", Res );
	/* Buffer now contains your character ASCII codes for LED Display
	  - select PAGE (LED Display line) and set starting SEG (column)
	  - for each c = ASCII code = Buffer[0], Buffer[1], ...,
		  send 8 bytes in Characters[c][0-7] to LED Display
	*/
	oled_Write_Cmd(0xB0); // Select PAGE
	oled_Write_Cmd(0x02); // Lower SEG start address
	oled_Write_Cmd(0x10); // Higher SEG start address
	for(unsigned int x = 0; Buffer[x] != '\0'; x++)
	{
		unsigned char c = Buffer[x];
		for(unsigned int y = 0; y < 8; y++)
		{
			oled_Write_Data(Characters[c][y]); //
		}
	}


	//Line 2:
	snprintf( Buffer, sizeof( Buffer ), "F: %5u Hz", Freq );
	/* Buffer now contains your character ASCII codes for LED Display
	  - select PAGE (LED Display line) and set starting SEG (column)
	  - for each c = ASCII code = Buffer[0], Buffer[1], ...,
		  send 8 bytes in Characters[c][0-7] to LED Display
	*/
   oled_Write_Cmd(0xB1); // Select PAGE
   oled_Write_Cmd(0x02); // Lower SEG start address
   oled_Write_Cmd(0x10); // Higher SEG start address
   for(unsigned int x = 0; Buffer[x] != '\0'; x++)
   {
	   unsigned char c = Buffer[x];
	   for(unsigned int y = 0; y < 8; y++)
	   {
		   oled_Write_Data(Characters[c][y]);
	   }
   }

}


void oled_Write_Cmd( unsigned char cmd )
{
	//trace_printf("Made it to oled_write_cmd\n");

	GPIOB->ODR |= (1 << 6);  // make PB6 = CS# = 1
	GPIOB->ODR &= ~(1 << 7); // make PB7 = D/C# = 0
	GPIOB->ODR &= ~(1 << 6); // make PB6 = CS# = 0
	oled_Write( cmd );
	GPIOB->ODR |= (1<<6); // make PB6 = CS# = 1
}


void oled_Write_Data( unsigned char data )
{
	GPIOB->ODR |= (1 << 6); // make PB6 = CS# = 1
	GPIOB->ODR |= (1 << 7); // make PB7 = D/C# = 1
	GPIOB->ODR &= ~(1 << 6); // make PB6 = CS# = 0

	oled_Write( data );
	GPIOB->ODR |= (1 << 6); // make PB6 = CS# = 1
}


void oled_Write( unsigned char Value )
{
/* Wait until SPI1 is ready for writing (TXE = 1 in SPI1_SR) */
	while(!(SPI1->SR & SPI_SR_TXE))
		{
		trace_printf("Stuck\n");;
		}
	/* Send one 8-bit character:
	  - This function also set BIDIOE = 1 in SPI1_CR1
	*/
	HAL_SPI_Transmit( &SPI_Handle, &Value, 1, HAL_MAX_DELAY );
	while(SPI1->SR & SPI_SR_BSY); //wait for SPI BSY to not be busy
}


void oled_config( void )
{
	trace_printf("Start\n");

	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // Configure PB3 and PB5 as Alternate Function (AF0) for SPI1
	GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER5);
	GPIOB->MODER |= GPIO_MODER_MODER3_1; //alternate mode
	GPIOB->MODER |= GPIO_MODER_MODER5_1; //alternate mode
	GPIOB->AFR[0] |= (0X0 << GPIO_AFRL_AFSEL3_Pos) | (0X0 << GPIO_AFRL_AFSEL5_Pos); //AF0
	//Configure PB4 as output for RES
	GPIOB->MODER |= GPIO_MODER_MODER4_0;
	//Configure PB6 as output for CS
	GPIOB->MODER |= GPIO_MODER_MODER6_0;
	//Configure PB7 as output for D/C
	GPIOB->MODER |= GPIO_MODER_MODER7_0;
	//Enable SPI1 clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	trace_printf("Start2\n");

	SPI_Handle.Instance = SPI1;
	SPI_Handle.Init.Direction = SPI_DIRECTION_1LINE;
	SPI_Handle.Init.Mode = SPI_MODE_MASTER;
	SPI_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
	SPI_Handle.Init.CLKPolarity = SPI_POLARITY_LOW;
	SPI_Handle.Init.CLKPhase = SPI_PHASE_1EDGE;
	SPI_Handle.Init.NSS = SPI_NSS_SOFT;
	SPI_Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	SPI_Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
	SPI_Handle.Init.CRCPolynomial = 7;

	// Initialize the SPI interface
	HAL_SPI_Init( &SPI_Handle );

	// Enable the SPI
	__HAL_SPI_ENABLE( &SPI_Handle );

	/* Reset LED Display (RES# = PB4):
	  - make pin PB4 = 0, wait for a few ms
	  - make pin PB4 = 1, wait for a few ms
	*/
	GPIOB->ODR &= ~(0b10000);
	for(int x = 0; x < 100000 * 3; x++);
	trace_printf("Start3\n");
	GPIOB->ODR |= 0b10000;
	for(int x = 0; x < 100000 * 3; x++);
	trace_printf("Start4\n");

	// Send initialization commands to LED Display
	for ( unsigned int i = 0; i < sizeof( oled_init_cmds ); i++ )
	{
		  //trace_printf("oled_init_cmds size = %d\n", sizeof(oled_init_cmds[i]));
	      oled_Write_Cmd( oled_init_cmds[i] );
	}

	/* Fill LED Display data memory (GDDRAM) with zeros:
	  - for each PAGE = 0, 1, ..., 7
		  set starting SEG = 0
		  call oled_Write_Data( 0x00 ) 128 times
	*/
	//Clearing:
	for(int x = 0; x < 8; x++)
	{
		  oled_Write_Cmd(0xB0 + x);
		  oled_Write_Cmd(0x00);
		  oled_Write_Cmd(0x10);
		  for (int y = 0; y < 128; y++) {
			  oled_Write_Data(0x00);
		  }
	}
	trace_printf("End of config\n");
}


void EXTI0_1_IRQHandler(void)
{
	/*
	In EXTI0_1_IRQHandler() do the following:
	Check if the EXTI1 flag is set → 555 timer stuff
	Check if EXTI0 flag is set → button press
	Check if button is pressed
	If the global signal is 0, set it to 1 and disable EXTI1 and enable EXTI2
	Testing: print function generator enable
	Else: reset global signal to 1, disable EXTI2 and enable EXTI1
	Testing: print 555 timer enable
	Clear pending flag EXTI0 pending flag
	Clear pending flag EXTI1 pending flag
	*/
	//trace_printf("Interrupt called\n");
	if(EXTI->PR & EXTI_PR_PR1)
	{
		double float_frequency;
		double float_scaledPeriod;
		unsigned int frequency;
		unsigned int scaledPeriod;
		if(!timerTriggered)
		      {
		          TIM2->CNT = 0;
		          TIM2->CR1 |= TIM_CR1_CEN;
		          timerTriggered = 1;
		      }
				//    Else (this is the second edge):
				//	- Stop timer (TIM2->CR1).
				//	- Read out count register (TIM2->CNT).
				//	- Calculate signal period and frequency.
				//	- Print calculated values to the console.
				//	  NOTE: Function trace_printf does not work
				//	  with floating-point numbers: you must use
				//	  "unsigned int" type to print your signal
				//	  period and frequency.
		      else
		      {
					TIM2->CR1 &= ~(TIM_CR1_CEN);
					uint32_t timerValue = TIM2->CNT;
					float_frequency = 48000000 / timerValue;
					float_scaledPeriod = 1/float_frequency;
					frequency = float_frequency;
					//trace_printf("555 Frequency: %u.%02u Hz\n", frequency, frequency % 100);
					Freq = frequency;
				    Res = (ADC1->DR * 5000) / 4095;
					//trace_printf("Resistance: %.2f\n", Res);
					timerTriggered = 0;

		      }
		EXTI->PR |= EXTI_PR_PR1; //Clear pending flag
	}

	if(EXTI->PR & EXTI_PR_PR0)
	{
		if(globalSignal == 0)
		{
			globalSignal = 1;
			EXTI->IMR &= ~EXTI_IMR_MR1; //Disable interrupt for EXTI1
			EXTI->IMR |= EXTI_IMR_MR2;  //Enable interrupt for EXTI2
			trace_printf("Function generator enabled\n");
		}else if(globalSignal == 1)
		{
			globalSignal = 0;
			EXTI->IMR &= ~EXTI_IMR_MR2; //Disable interrupt for EXTI2
			EXTI->IMR |= EXTI_IMR_MR1;  //Enable interrupt for EXTI1
			trace_printf("555 timer enabled\n");
		}
		EXTI->PR |= EXTI_PR_PR0; //Clear EXTI0 Pending flag
	}

}
#pragma GCC diagnostic pop
// ----------------------------------------------------------------------------



