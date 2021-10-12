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
This class is the panel-specific implementation to support drawing and refreshing
Hitchin Hackspce LED display panels arranged as a 64x64 matrix. This is constructed 
as 4 panels of 64x16 LEDs aranged vertically and driven in parallel by the 
platform driver.
******************************************************************************/
#include <Arduino.h>

template<class PLATFORMTYPE, unsigned short COLOUR_DEPTH> class HHLedPanel_4x64x16_impl
{
  static_assert( COLOUR_DEPTH > 0 && COLOUR_DEPTH <= 5, "Maximum colour depth supported is 5" );

private:
  static const uint16_t CHIPS_PER_DATA_LINE = 24;
  static const uint16_t LEDS_PER_CHIP = 16;
  static const uint16_t ADDRESS_PLANES = 4;
  
  byte frameBuffers[COLOUR_DEPTH][ADDRESS_PLANES][CHIPS_PER_DATA_LINE * LEDS_PER_CHIP];  // [bit][plane][chip]
  
public:
  void initialise(uint16_t maxBrightnessPercent)
  {
	// Setup the hardware
	PLATFORMTYPE::Initialise(frameBuffers[0][0], COLOUR_DEPTH, ADDRESS_PLANES, CHIPS_PER_DATA_LINE * LEDS_PER_CHIP);
	
	// Set the base brighness
	PLATFORMTYPE::SetBrightness(maxBrightnessPercent);
	
	// Start refrshing the screen
	PLATFORMTYPE::StartDisplay();
  }

  // Dimension of the total panel
  inline uint32_t getWidth() const
  {
	return 64;
  }

  inline uint32_t getHeight() const
  {
    return 64;
  }

  void drawPixel(int16_t x, int16_t y, uint16_t col) 
	{
    // Clip to panel
    if(x < 0 || x >= getWidth() || y < 0 || y >= getHeight())
      return;
      
	  int16_t off = (x & 7) + (x & 0xf8)*6 + (y & 4)*2;
	  byte row = y & 3;
	  byte b = (1 << ((y & 0x3f) >> 3));
   
    for(uint8_t depth = 0; depth < COLOUR_DEPTH; depth++)
    {
  	  byte *p = & frameBuffers[depth][row][off];
      // Blue LED
      if ( col & (0x0010 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += LEDS_PER_CHIP;
  
      // Green LED
      if ( col & (0x0400 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += LEDS_PER_CHIP;
      
      // Red LED
      if ( col & (0x8000 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += LEDS_PER_CHIP;
     }
	}

  void Clear(bool toWhite = false)
  {
    FillBuffer(toWhite ? 0xff : 0);
  }

private:
  void FillBuffer(byte b = 0)
	{
      // Quick clear to solid colour (normally black or white)
      memset(frameBuffers[0][0], b, sizeof(frameBuffers));
	}	
};
