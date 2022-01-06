#include <WiFi.h>
#include "time.h"
#include <Adafruit_GFX.h> // So can use free fonts provided as part of the Adafruit_GFX library
#include <gfxfont.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>

const char* ssid       = "xxx";
const char* password   = "pppp";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; //3600;
const int   daylightOffset_sec = 0; //3600;

// Panel type and arrangement
#include <HHLedPanel_16x64x16_impl.h>
// Hardware driver
#include <ESP32_16xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>

#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.

// Static display panel interface
HHLedPanel<HHLedPanel_16x64x16_impl<ESP32_16xMBI5034, 5>, Adafruit_GFX> *panel = new HHLedPanel<HHLedPanel_16x64x16_impl<ESP32_16xMBI5034, 5>, Adafruit_GFX>(MAX_BRIGHTNESS);

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  panel->setFont(&FreeSerifBoldItalic9pt7b);
  panel->setTextSize(1);
  panel->clear();
  panel->setTextColor(GREEN);  
  panel->setCursor(0, 13);
  panel->println(&timeinfo, "%A");
  panel->setTextColor(CYAN);  
  panel->println(&timeinfo, "%d %B %Y");
  panel->setTextColor(YELLOW);  
  panel->println(&timeinfo, "%H:%M:%S");
}

void setup()
{
  Serial.begin(115200);
  
  panel->begin();
  panel->setRotation(1);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop()
{
  delay(1000);
  printLocalTime();
}
