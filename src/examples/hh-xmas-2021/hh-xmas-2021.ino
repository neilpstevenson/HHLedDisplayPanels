#include <AnimatedGIF.h>

#define USE_SDCARD 1


/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(ESP32)
#ifndef USE_SDCARD
#include <SPIFFS.h>
#else
#include <SD.h>
#endif
#elif defined(ESP8266)
#include <LittleFS.h>
#include <SD.h>
#else
#include <SD.h>
#endif

// Panel type and arrangement
#include <HHLedPanel_16x64x16_impl.h>
// Hardware driver
#include <ESP32_16xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>

#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.

// Static display panel interface
HHLedPanel<HHLedPanel_16x64x16_impl<ESP32_16xMBI5034, 5>> *panel = new HHLedPanel<HHLedPanel_16x64x16_impl<ESP32_16xMBI5034, 5>>(MAX_BRIGHTNESS);


// Demo sketch to play all GIF files in a directory
// Tested on ESP32-DevBoardC/NodeMCU

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 64

AnimatedGIF gif;
File f;
int x_offset, y_offset;

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
    y = y_offset + pDraw->iY + pDraw->y; // current line
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
          //Serial.printf("%d,%d  %d\n", x_offset + pDraw->iX + x, y, iCount);
          panel->startWrite();
          panel->setAddrWindow(x_offset + pDraw->iX + x, y, iCount, 1);
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
      panel->setAddrWindow(x_offset + pDraw->iX, y, iWidth, 1);
      panel->writePixels(usTemp, iWidth, false, false);
      panel->endWrite();
    }
} /* GIFDraw() */

void * GIFOpenFile(const char *fname, int32_t *pSize)
{
/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    f = SD.open(fname, "r");
#elif defined(ESP32)
#ifndef USE_SDCARD
    f = SPIFFS.open(fname, "r");
#else    
    f = SD.open(fname, "r");
#endif
#elif defined(ESP8266)
    f = LittleFS.open(fname, "r");
    // f = SD.open(fname, "r");
#else
    f = SD.open(fname, FILE_READ);
#endif
  //f = SD.open(fname);
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
     f->close();
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0)
       return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    //printf("Read %d\n", iBytesRead);
    return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{ 
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
//  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

void setup() {
  Serial.begin(115200);
  while (!Serial);

  delay(1000);

  // Start the display
  panel->begin();
  panel->setRotation(1);

  // Allow time for SD card to initialise
  delay(200);

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI, 4000000UL))
#elif defined(ESP32)
#ifndef USE_SDCARD
  if (!SPIFFS.begin())
#else
  static SPIClass mySPI(HSPI);
  //mySPI.begin(GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13);
  while (!SD.begin(GPIO_NUM_23, mySPI, 4000000UL))
#endif
#elif defined(ESP8266)
  if (!LittleFS.begin())
  // if (!SD.begin(SS))
#else
  if (!SD.begin())
#endif
  {
    Serial.println("SD card init failed!");
    panel->clear();
    panel->println("No SD card found");
    //while (1); // SD initialisation failed so wait here
    delay(500);
  }
  Serial.println("SD Card init success!");

 // spilcdInit(&lcd, LCD_ST7789, FLAGS_NONE, 40000000, CS_PIN, DC_PIN, RESET_PIN, LED_PIN, MISO_PIN, MOSI_PIN, SCK_PIN);
 // gif.begin(BIG_ENDIAN_PIXELS);
  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void ShowGIF(char *name)
{
  panel->clear();
  //spilcdFill(&lcd, 0, DRAW_TO_LCD);
  
  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    x_offset = (DISPLAY_WIDTH - gif.getCanvasWidth())/2;
    if (x_offset < 0) x_offset = 0;
    y_offset = (DISPLAY_HEIGHT - gif.getCanvasHeight())/2;
    if (y_offset < 0) y_offset = 0;
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    Serial.flush();
    int pfr;
    while ((pfr = gif.playFrame(true, NULL)) > 0)
    {     
       //Serial.printf("%d,", pfr); 
       //Serial.flush();
    }
    if(pfr < 0)
    {
       Serial.printf("playFrame returned error %d\n", pfr); 
       Serial.flush();
    }
    gif.close();
  }
  else
  {
    Serial.printf("Failed to open GIF file.\n");
    Serial.flush();
  }

} /* ShowGIF() */

//
// Return true if a file's leaf name starts with a "." (it's been erased)
//
int ErasedFile(char *fname)
{
int iLen = strlen(fname);
int i;
  for (i=iLen-1; i>0; i--) // find start of leaf name
  {
    if (fname[i] == '/')
       break;
  }
  return (fname[i+1] == '.'); // found a dot?
}


void loop() {

char *szDir = "/GIF"; // play all GIFs in this directory on the SD card
char fname[256];
File root, temp;

//   while (1) // run forever
   {
      //root = SD.open(szDir);
/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    root = SD.open(szDir, "r");
#elif defined(ESP32)
#ifndef USE_SDCARD
    root = SPIFFS.open(szDir, "r");
#else
    root = SD.open(szDir, "r");
#endif
#elif defined(ESP8266)
    root = LittleFS.open(szDir, "r");
    // root = SD.open(szDir, "r");
#else
    root = SD.open(szDir, FILE_READ);
#endif
      if (root)
      {
         temp = root.openNextFile();
            while (temp)
            {
              if (!temp.isDirectory()) // play it
              {
                strcpy(fname, temp.name());
                if (!ErasedFile(fname))
                {
                  Serial.printf("Playing %s\n", temp.name());
                  Serial.flush();
                  ShowGIF((char *)temp.name());
                }
              }
              temp.close();
              temp = root.openNextFile();
            }
         root.close();
      } // root
      
      //delay(4000); // pause before restarting
   } // while
} /* loop() */
