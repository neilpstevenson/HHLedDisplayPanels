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
This is an exmple on how to use the Hitchin Hackspce LED display panel class
on an ESP32-DevKitC board connected to 4 panels as a 64x64 square.
******************************************************************************/

// Panel type and arrangement
#include <HHLedPanel_4x64x16_impl.h>
// Hardware driver
#include <ESP32_4xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>

#include "hhLogoBmp.h"

#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

// Static display panel interface
HHLedPanel<HHLedPanel_4x64x16_impl<ESP32_4xMBI5034, 6>> panel(MAX_BRIGHTNESS);

void showTestScreen()
{
  panel.clear();
  panel.setTextSize(1);
  panel.setCursor(0, 0);
  panel.setTextColor(WHITE);  
  panel.println(F(" TOP(0,0)"));
  panel.setCursor(0, 16);
  panel.setTextColor(WHITE);  
  panel.println(F("   UP-MID"));
  panel.setCursor(0, 40);
  panel.setTextColor(WHITE);  
  panel.println(F("  LOW-MID"));
  panel.setCursor(0, 56);
  panel.setTextColor(WHITE);  
  panel.println(F("  BOTTOM"));
  panel.drawLine(0,0, panel.width()-1, panel.height()-1, RED);
  panel.drawLine(0,panel.height()-1, panel.width()-1, 0, GREEN);
  delay(2000);
  panel.fillScreen(WHITE);
  delay(2000);
}

void setup() {
  // Start the display
  panel.begin();

  // Test screen once only
  showTestScreen();
}

void showHHLogo()
{
    panel.clear();
    panel.setTextSize(1);
    panel.setTextColor(WHITE);  
    for(int h = 0; h < 32; h++)
    {
      panel.drawBitmap(0, h, (const uint8_t *)&LHSlogoBitmap, 64, 64, WHITE, BLUE);
      panel.fillRect(0, 0, panel.width(), h, BLACK);
      panel.setCursor(0, h-24);
      panel.println(F("  Hitchin"));
      panel.setCursor(2, h-24+9);
      panel.println(F(" Hackspace"));
      delay(25);
    }
}

void testFade()
{
    panel.clear();
    panel.drawLine(0,16, panel.width(), 16, WHITE);
    panel.drawLine(0,34, panel.width(), 34, WHITE);
    for (int ct=0; ct < 256; ct += 2)
    {
      panel.fillRect(0, 18, panel.width(), 16, BLACK);
      panel.setCursor(1, 18);
      panel.setTextColor(panel.make_colour(ct, ct, ct));  
      panel.setTextSize(1);
      panel.printf("Fade Up   %4d", ct);
      delay(50);
    }
    for (int ct=256-8; ct > 0; ct -= 2)
    {
      panel.fillRect(0, 18, panel.width(), 16, BLACK);
      panel.setCursor(1, 18);
      panel.setTextColor(panel.make_colour(ct, ct, ct));  
      panel.setTextSize(1);
      panel.printf("Fade Down %4d", ct);
      delay(50);
    }
}

void loop()
{
   showHHLogo();
   delay(4000); 
   testFade();
}
