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
This class encapsulates the platform-specific driver for refreshing a set of
4 LED display panels using MBI5034 chips arranged as 2 data pins per panel
(i.e. maximum of 8 data lines currently).

This version uses a fast timer interrupt to output one logical row (2 physical 
rows) per interrupt and allows up to a 5-bit colour depth to be used.
******************************************************************************/
#pragma once
#include <Arduino.h>

class ESP32_16xMBI5034
{
public:
  static void Initialise(byte *frameBuffers, uint8_t colourDepth, uint8_t planes, uint16_t bytesToSend);
  
  // Currently this mus be called after Initialise and before StartDisplay
  static void SetBrightness(uint16_t brighnessPercent);

  // Start showing the display
  static void StartDisplay();

private:  
  static void IRAM_ATTR RefreshInterrupt();
};
