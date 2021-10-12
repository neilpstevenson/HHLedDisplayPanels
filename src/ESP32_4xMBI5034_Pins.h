/******************************************************************************
MIT License

Copyright (c) 2021 Neil Stevenson (Twitter: @mediablip)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

/******************************************************************************
This file contains the pin assignments and performance criteria used to refresh 
a set of 4 LED display panels using MBI5034 chips arranged as 2 data pins per 
panel (i.e. maximum of 8 data lines currently).
******************************************************************************/

// These are for the ESP32-DevKitC board with WROOM chip
// Only unused GPIO pins 0-31 are usable as the PIN_Dx or PIN_Ax pins
#define PIN_D1    GPIO_NUM_18  // D1 on Panel 1 (Top)
#define PIN_D2    GPIO_NUM_5   // D2 on Panel 1
#define PIN_D3    GPIO_NUM_17  // D1 on Panel 2
#define PIN_D4    GPIO_NUM_16  // D2 on Panel 2
#define PIN_D5    GPIO_NUM_4   // D1 on Panel 3
#define PIN_D6    GPIO_NUM_0   // D2 on Panel 3
#define PIN_D7    GPIO_NUM_2   // D1 on Panel 4
#define PIN_D8    GPIO_NUM_15  // D2 on Panel 4 (Bottom)

// PIN_Ax can also share the PIN_Dx data line pins to save outputs required
#define PIN_A0    GPIO_NUM_5   // A0 on all Panels
#define PIN_A1    GPIO_NUM_18  // A1 on all Panels

// The control pins can be on any unused GPIO pins 0-36
#define PIN_CLK   GPIO_NUM_22  // CLK on all Panels
#define PIN_LAT   GPIO_NUM_21  // LAT on all Panels
#define PIN_OE    GPIO_NUM_19  // OE on all Panels

// Target total refresh rate in microseconds. 6000uS gives a good flicker-free display at around 150Hz
static const uint64_t REFRESH_INTERVAL_uS = 6000;
