/*
* E131_Test.ino - Simple sketch to listen for E1.31 data on an ESP32 
*                  and print some statistics.
*
* Project: ESPAsyncE131 - Asynchronous E.131 (sACN) library for Arduino ESP8266 and ESP32
* Copyright (c) 2019 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include <ESPAsyncE131.h>
//#include <TFT_eSPI.h>   // TTGO display

// Panel type and arrangement
#include <HHLedPanel_16x64x16_impl.h>
// Hardware driver
#include <ESP32_16xMBI5034.h>
// Adafruit GFX interface
#include <HHLedPanel.h>


#define UNIVERSE 1                   // First DMX Universe to listen for
#define UNIVERSE_COUNT (UNIVERSE + display.width() - 1)  // Total number of Universes to listen for, starting at UNIVERSE
#define BUFFER_COUNT 96              // Total number packets that we can buffer. Ideally same as UNIVERSE_COUNT, but memory constrains limit this!

const char ssid[] = "xxxx";         // Replace with your SSID
const char passphrase[] = "pppp";   // Replace with your WPA2 passphrase

// ESPAsyncE131 instance with BUFFER_COUNT buffer slots
ESPAsyncE131 e131(BUFFER_COUNT);

//TFT_eSPI display = TFT_eSPI(135, 240); // TTGO display
#define MAX_BRIGHTNESS  12  // 12%-200%. At 12% four panels consume around 6 amps, at 100% around 40 amps.
HHLedPanel<HHLedPanel_16x64x16_impl<ESP32_16xMBI5034, 5>> display(MAX_BRIGHTNESS);

void setup() {
    Serial.begin(115200);
    delay(10);

    // Start the display
    //display.init();
    display.begin();
    display.setRotation(1);
//    display.fillScreen(TFT_BLACK);
    display.fillScreen(BLACK);
//    display.setTextSize(2);
    
    // Make sure you're in station mode    
    WiFi.mode(WIFI_STA);

    Serial.println("");
    Serial.print(F("Connecting to "));
    Serial.print(ssid);

//    display.setTextColor(TFT_GREEN);
    display.setTextColor(GREEN);
//    display.drawString("Connecting to:", 0, display.height() / 2 - 48);
//    display.drawString(ssid, 0, display.height() / 2 - 16);
//    display.setCursor(0, display.height() / 2 - 48);
    display.setCursor(0, 0);
    display.printf("Connecting to:\n%s", ssid);

    if (passphrase != NULL)
        WiFi.begin(ssid, passphrase);
    else
        WiFi.begin(ssid);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print(F("Connected with IP: "));
    Serial.println(WiFi.localIP());
    
//    display.drawString("Connected with IP:", 0, display.height() / 2 + 16);
//    display.drawString(WiFi.localIP().toString(), 0, display.height() / 2 + 48);
    display.printf("\nConnected with IP:\n%s", WiFi.localIP().toString().c_str());

    delay(1000);
   
    // Choose one to begin listening for E1.31 data
    //if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
        Serial.println(F("Listening for data..."));
    else 
        Serial.println(F("*** e131.begin failed ***"));
}

void loop() {
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        /*
        Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                htons(packet.universe),                 // The Universe for this packet
                htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                e131.stats.num_packets,                 // Packet counter
                e131.stats.packet_errors,               // Packet error counter
                packet.property_values[1]);             // Dimmer data for Channel 1
*/
        // All pixes are 3 RGB value triples
        unsigned int x = htons(packet.universe)-1;
        for(unsigned int c = 0; c < htons(packet.property_value_count) - 1; c += 3)
        { 
          // Treat Universe as X (0-239) and Channel/3 as Y (0-63)
          display.drawPixel(x, display.height() - c/3 - 1, display.color565(packet.property_values[c+1], packet.property_values[c+2], packet.property_values[c+3]));
        }
    }
}
