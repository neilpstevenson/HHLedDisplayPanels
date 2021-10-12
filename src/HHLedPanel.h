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
#include <Adafruit_GFX.h>

template<class PANELTYPE> class HHLedPanel : public Adafruit_GFX
{
  private:
    PANELTYPE _panel_impl;
    
  public: 
    HHLedPanel() : Adafruit_GFX(_panel_impl.getWidth(), _panel_impl.getHeight())
    {
    }
    
    // Setup the hardware
    void initialise(uint16_t maxBrightnessPercent)
    {
      _panel_impl.initialise(maxBrightnessPercent);
    }
    
    void drawPixel(int16_t x, int16_t y, uint16_t color) 
    {
      _panel_impl.drawPixel(x,y,color);
    }

    void fillScreen(uint16_t color)
    {
      if(color == 0)
        _panel_impl.Clear();
      else if(color == 0xffff)
        _panel_impl.Clear(true);
      else
        Adafruit_GFX::fillScreen(color);
    }
    
    void clear()
    {
      _panel_impl.Clear();
    }
	
    // Construct a colour value from its RGB parts
    static uint16_t make_colour(uint8_t red, uint8_t green, uint8_t blue)
    {
      return (((blue) >> 3) | ((green) >> 2 << 5) | ((red) >> 3 << 11));
    }

};
