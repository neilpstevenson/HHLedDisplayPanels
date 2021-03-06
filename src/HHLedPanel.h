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
This class implements the Adafruit GFX interface to allow all of the Adafruit
graphic primitives and examples to be used with the Hitchin Hackspce LED display 
panels in the same way as other Adafruit LCD and OLED display panels.
******************************************************************************/
#pragma once
//#include <Adafruit_GFX.h>
#include <Arduino_GFX.h>

template<class PANELTYPE, class BASECLASS = Arduino_GFX> class HHLedPanel : public BASECLASS
{
  private:
    PANELTYPE _panel_impl;
	uint16_t block_x, block_y, block_w, block_h;
    
  public: 
    HHLedPanel(uint16_t maxBrightnessPercent = 12) : BASECLASS(_panel_impl.getWidth(), _panel_impl.getHeight())
    {
		// Setup the hardware
      _panel_impl.initialise(maxBrightnessPercent);
    }
    
	// These are for compatibility with the Adafruit_SPITFT interface
	void begin(uint32_t freq)
	{
      _panel_impl.begin();
	}
   
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w,
                             uint16_t h) 
	{
		block_x = x;
		block_y = y;
		block_w = w;
		block_h = h;
	}
	
	void writePixels(uint16_t *colors, uint32_t len, bool block,
                                  bool bigEndian)
	{
		// Bit simplistic, but will do for now
		for(uint16_t pixel = 0; pixel < len; pixel++)
		{
			drawPixel(pixel % block_w + block_x, pixel / block_w + block_y, colors[pixel]);
		}
	}
	
	// These are for compatibility with the Arduino_GFX interface
	void begin(int32_t speed = 0)
	{
      _panel_impl.begin();
	}
	
	void writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
	{
		drawPixel(x,y,color);
	}
	
	void drawIndexedBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint16_t *color_index, int16_t w, int16_t h)
	{
		//Serial.printf("\ndib: %d,%d %d,%d\n", x, y, w, h);
		int32_t offset = 0;
		BASECLASS::startWrite();
		for (int16_t j = 0; j < h; j++, y++)
		{
			for (int16_t i = 0; i < w; i++)
			{
				//Serial.printf("%3d",bitmap[offset]);
				drawPixel(x + i, y, color_index[bitmap[offset++]]);
			}
		}
		BASECLASS::endWrite();
	}

	// These are common between the supported interfaces
    void drawPixel(int16_t x, int16_t y, uint16_t color) 
    {
		// Apply any rotation in effect
		switch (BASECLASS::getRotation()) {
		case 1:
			_panel_impl.drawPixel(_panel_impl.getWidth() - 1 - y, x, color);
			break;
		case 2:
			_panel_impl.drawPixel(_panel_impl.getWidth() - 1 - x, _panel_impl.getHeight() - 1 - y, color);
			break;
		case 3:
			_panel_impl.drawPixel(y, _panel_impl.getHeight() - 1 - x, color);
			break;
		 default:
			_panel_impl.drawPixel(x, y, color);
			break;
		}
    }

    void fillScreen(uint16_t color)
    {
      if(color == 0)
        _panel_impl.Clear();
      else if(color == 0xffff)
        _panel_impl.Clear(true);
      else
        BASECLASS::fillScreen(color);
    }
    
    void clear()
    {
	  BASECLASS::setCursor(0,0);
      _panel_impl.Clear();
    }
	
    // Construct a colour value from its RGB parts
    static uint16_t make_colour(uint8_t red, uint8_t green, uint8_t blue)
    {
      return (((blue) >> 3) | ((green) >> 2 << 5) | ((red) >> 3 << 11));
    }

};
