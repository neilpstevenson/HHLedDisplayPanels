/*******************************************************************************
 * PNG Image Viewer
 * This is a simple PNG image viewer example based on the original Arduino_GFX 
 * example
 * 
 * Image Source: https://github.com/logos
 *
 * Dependent libraries:
 * Pngle: https://github.com/kikuchan/pngle.git
 * 
 * Setup steps:
 * 1. Install pngle from the above url
 * 2. Upload PNG file, dependining on platform:
 *   SPIFFS (ESP32):
 *     upload SPIFFS data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/me-no-dev/arduino-esp32fs-plugin
 *   LittleFS (ESP8266):
 *     upload LittleFS data with ESP8266 LittleFS Data Upload:
 *     ESP8266: https://github.com/earlephilhower/arduino-esp8266littlefs-plugin
 *   SD:
 *     Most Arduino system built-in support SD file system.
 *     Wio Terminal require extra dependant Libraries:
 *     - Seeed_Arduino_FS: https://github.com/Seeed-Studio/Seeed_Arduino_FS.git
 *     - Seeed_Arduino_SFUD: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD.git
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

// Panel type and arrangement
#include <HHLedPanel_4x64x16_impl.h>
// Hardware driver
#include <ESP32_4xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>

#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.

// Static display panel interface
HHLedPanel<HHLedPanel_4x64x16_impl<ESP32_4xMBI5034, 6>> *panel = new HHLedPanel<HHLedPanel_4x64x16_impl<ESP32_4xMBI5034, 6>>();

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(ESP32)
#include <SPIFFS.h>
#include <SD.h>
#elif defined(ESP8266)
#include <LittleFS.h>
#include <SD.h>
#else
#include <SD.h>
#endif

#include <pngle.h>
int16_t xOffset = 0;
int16_t yOffset = 0;

// Pngle init callback
void pngleInitCallback(pngle_t *pngle, uint32_t w, uint32_t h)
{
  int16_t gfxW = panel->width();
  int16_t gfxH = panel->height();
  xOffset = (w > gfxW) ? 0 : ((gfxW - w) / 2);
  yOffset = (h > gfxH) ? 0 : ((gfxH - h) / 2);
}

// Pngle draw callback
void pngleDrawCallback(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
  if (rgba[3] > 96) // not transparent
  {
    panel->fillRect(x + xOffset, y + yOffset, w, h, panel->make_colour(rgba[0], rgba[1], rgba[2]));
  }
}

void setup()
{
  Serial.begin(115200);

  // Start the display
  panel->initialise(MAX_BRIGHTNESS);
  panel->fillScreen(BLACK);
}

void displayPng(const char *filename)
{
  
/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI, 4000000UL))
#elif defined(ESP32)
  if (!SPIFFS.begin())
  // if (!SD.begin(SS))
#elif defined(ESP8266)
  if (!LittleFS.begin())
  // if (!SD.begin(SS))
#else
  if (!SD.begin())
#endif
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    panel->println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    unsigned long start = millis();

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    File pngFile = SD.open(filename, "r");
#elif defined(ESP32)
    File pngFile = SPIFFS.open(filename, "r");
    // File pngFile = SD.open(filename, "r");
#elif defined(ESP8266)
    File pngFile = LittleFS.open(filename, "r");
    // File pngFile = SD.open(filename, "r");
#else
    File pngFile = SD.open(filename, FILE_READ);
#endif

    if (!pngFile || pngFile.isDirectory())
    {
      Serial.print(F("ERROR: Failed to open ")); Serial.print(filename); Serial.println(F(" for reading"));
      panel->print(F("ERROR: Failed to open ")); Serial.print(filename); Serial.println(F(" for reading"));
    }
    else
    {
      pngle_t *pngle = pngle_new();
      pngle_set_init_callback(pngle, pngleInitCallback);
      pngle_set_draw_callback(pngle, pngleDrawCallback);
      char buf[64]; // buffer minimum size is 16 but it can be much larger, e.g. 2048
      int remain = 0;
      int len;
      while ((len = pngFile.readBytes(buf + remain, sizeof(buf) - remain)) > 0)
      {
        int fed = pngle_feed(pngle, buf, remain + len);
        if (fed < 0)
        {
          Serial.printf("ERROR: %s\n", pngle_error(pngle));
          break;
        }

        remain = remain + len - fed;
        if (remain > 0)
        {
          memmove(buf, buf + fed, remain);
        }
      }

      pngle_destroy(pngle);
      pngFile.close();
      
      Serial.printf("Time taken: %lumS\n", millis() - start);
    }
  }
}

void loop()
{
  panel->fillScreen(panel->make_colour(255,255,255)); // background color (transparent pixels will show this through)
  displayPng("/hh_logo.png");
  delay(3000);  
  displayPng("/rgbwheel.png");
  delay(3000);  
  panel->fillScreen(panel->make_colour(128,128,128)); // background color (transparent pixels will show this through)
  displayPng("/octocat-small.png");
  delay(3000);  
}
