// adafruit_gfx_memory
//
// Example sketch which shows how to display an
// animated GIF image stored in FLASH memory
// using the Adafruit_GFX_TFT library
//
// written by Larry Bank
// bitbank@pobox.com
//
// To display a GIF from memory, a single callback function
// must be provided - GIFDRAW
// This function is called after each scan line is decoded
// and is passed the 8-bit pixels, RGB565 palette and info
// about how and where to display the line. The palette entries
// can be in little-endian or big-endian order; this is specified
// in the begin() method.
//
// The AnimatedGIF class doesn't allocate or free any memory, but the
// instance data occupies about 22.5K of RAM.
// 
#include <AnimatedGIF.h>

#include "SPI.h"
#include "test_images/badgers.h"

// Panel type and arrangement
#include <HHLedPanel_4x64x16_impl.h>
// Hardware driver
#include <ESP32_4xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>

#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.

// Static display panel interface
HHLedPanel<HHLedPanel_4x64x16_impl<ESP32_4xMBI5034, 6>> *panel = new HHLedPanel<HHLedPanel_4x64x16_impl<ESP32_4xMBI5034, 6>>(MAX_BRIGHTNESS);

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 64

AnimatedGIF gif;

// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth + pDraw->iX > DISPLAY_WIDTH)
       iWidth = DISPLAY_WIDTH - pDraw->iX;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
       return; 
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
      for (x=0; x<iWidth; x++)
      {
        if (s[x] == pDraw->ucTransparent)
           s[x] = pDraw->ucBackground;
      }
      pDraw->ucHasTransparency = 0;
    }

    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      int x, iCount;
      pEnd = s + iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while(x < iWidth)
      {
        c = ucTransparent-1;
        d = usTemp;
        while (c != ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          }
          else // opaque
          {
             *d++ = usPalette[c];
             iCount++;
          }
        } // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          panel->startWrite();
          panel->setAddrWindow(pDraw->iX+x, y, iCount, 1);
          panel->writePixels(usTemp, iCount, false, false);
          panel->endWrite();
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent)
             iCount++;
          else
             s--; 
        }
        if (iCount)
        {
          x += iCount; // skip these
          iCount = 0;
        }
      }
    }
    else
    {
      s = pDraw->pPixels;
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      for (x=0; x<iWidth; x++)
        usTemp[x] = usPalette[*s++];
      panel->startWrite();
      panel->setAddrWindow(pDraw->iX, y, iWidth, 1);
      panel->writePixels(usTemp, iWidth, false, false);
      panel->endWrite();
    }
} /* GIFDraw() */


void setup() {
  Serial.begin(115200);

  // put your setup code here, to run once:
  panel->begin();
  panel->setRotation(1);
  panel->fillScreen(0);
  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (gif.open((uint8_t *)ucBadgers, sizeof(ucBadgers), GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    while (gif.playFrame(true, NULL))
    {      
    }
    gif.close();
  }
}
