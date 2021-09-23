/******************************************************************************
This is an Arduino sketch to drive 4 LED panels based on MBI5034 LED drivers.

Written by Oliver Dewdney and Jon Russell

It works specifically on a Arduino Micro (ATMega32U4), as it accesses the 
registers directly. However, with a small amount of editing, it should work on 
most Arduinos.

Basic Operation:

The objective was to be able to drive four panels with a single Arduino. 
Each panel has two data lines, so four panels require 8 data lines, i.e. a 
single byte.

An Arduino Micro was selected, as it has a native USB port and slightly more
RAM than an Uno. A Mega would probably work well too.

The code has a frame buffer in RAM with 4 sets of 384 bits 
(1 bank = 64 LEDs x 2 Rows x 3 bits (RGB) = 384) for each data line. 
Four panels, two data lines each, means all four panels can be driven by a byte 
wide frame buffer, assuming 3 bit colour. This means the update can be very 
efficient. The frame buffer is about 1.5KB so fits happily in to the ATMega32U4
with some room for local variables.

The UpdateFrame loop iterates over the line of 384 bits of serial data from the
frame buffer and clocks them out quickly.

Ideally, we needed a contiguous port on the Microcontroller to be attached to 
8 external data lines for the four panels. But most Arduinos don’t have this. 
On the Arduino Micro, PortB has the RX LED on bit 0 and PortD has the TX LED on
bit 5. So, I have connected 4 data lines to the high nibble on PortB and 4 data
lines to the low nibble on PortD.

If we had a contiguous port free (If we used a Mega) we could use only one port
and the UpdateFrame loop would be 1 instruction faster ! :-) But currently I 
copy the data to both ports, ignoring the high and low nibbles I don’t need.

UpdateFrame is called by an interrupt 390 times a second 
(OCR1A = 160; // compare match register 16MHz/256/390Hz)). 
UpdateFrame needs to be called 4 times to update the entire panel, as the panel
is split in to 4 rows of 128 LEDs (as 2 rows of 64).

For each half a panel (one data line) there are 8 rows of 64 LEDs, addresses in 
banks. Bank 0x00 is row 0&4,  Bank 0x01 is row 1&5, Bank 0x02 is row 2&6, Bank 
0x03 is row 3&7.

Each call updates one bank and leaves it lit until the next interrupt.

This method updates the entire frame (1024 RGB LEDs) about 100 times a second.

Port map for Arduino Micro (ATMega32U4)
    7     6     5     4     3     2     1     0
PB  D11   D10   D9    D8    MISO  MOSI  SCK   RX/SS
PC  D13   D5    X     X     X     X     X     X
PD  D6    D12   TX    D4    D1    D0    D2    D3
PE  X     D7    X     X     X     HWB   X     X
PF  A0    A1    A2    A3    X     X     A4    A5

This sketch now inherits from the Adafruit GFX library classes. This means we
can use the graphics functions like drawCircle, drawRect, drawTriangle, plus
the various text functions and fonts.

******************************************************************************

Updated by Courty 21/07/17
fixed drawBitmap calls
added bmp.h and seperated display pages in main for rolling display
added 'macro to text commands' to store chars in Filemem and clean up somespace in RAM

******************************************************************************/

#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <fonts/FreeSerifBoldItalic9pt7b.h>
#include "hhLogoBmp.h"
#include "bmp.h"
#include "HHLedPanel_4x64x16.h"
#include "ESP32_4xMBI5034.h"
#include "LedPanelDisplay.h"

LedPanelDisplay<HHLedPanel_4x64x16<ESP32_4xMBI5034>> panel;

#define RGB565(r,g,b) (((b) >> 3) | ((g) >> 2 << 5) | ((r) >> 3 << 11))

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

/*
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define YELLOW 6
#define WHITE 7
*/

void setup() {
  //Serial.begin(115200);
  //Serial.println("Begin...");
  
  panel.FillBuffer(0x00);         // Set all LEDs off. (Black)
  panel.initialise();             // Start display

  panel.FillBuffer(0x00);
  panel.setTextSize(1);
  panel.setCursor(0, 0);
  panel.setTextColor(WHITE);  
  panel.println(F("TOP - 0,0"));
  panel.setCursor(0, 16);
  panel.setTextColor(WHITE);  
  panel.println(F("TMID - 0,16"));
  panel.setCursor(0, 32);
  panel.setTextColor(GREEN);  
  panel.println(F("BMID - 0,32"));
  panel.setCursor(0, 48);
  panel.setTextColor(MAGENTA);  
  panel.println(F("BOT - 0,48"));

  delay(2000);
  panel.fillScreen(WHITE);
  delay(1000);
}

/*
void testText1() {
  panel.FillBuffer(0x00);
  panel.setCursor(0, 0);
  panel.setTextColor(GREEN);  
  panel.println("The Quick ");
  panel.setTextColor(CYAN);  
  panel.println("Brown Fox ");
  panel.setTextColor(RED);  
  panel.println("Jumped    ");
  panel.setTextColor(MAGENTA);  
  panel.println("Over the  ");
  panel.setTextColor(RED);  
  panel.println(" South Ldn");
  panel.setTextColor(WHITE);  
  panel.println("MakerSpace");

}


void testText2() {
  panel.FillBuffer(0x00);
  panel.setTextSize(1);
  panel.setCursor(15, 17);
  panel.setTextColor(WHITE);  
  panel.println(F("COURTY"));
  panel.setCursor(8, 28);
  panel.setTextColor(MAGENTA);  
  panel.println(F("(C) 2017"));

}



void testText3() {
  panel.FillBuffer(0x00);
  panel.setCursor(3, 22);
  panel.setTextColor(YELLOW);  
  panel.setTextSize(2);
  panel.println(F("Hello"));
}


  panel.setTextColor(GREEN);  
  panel.println("The Quick ");
  panel.setTextColor(CYAN);  
  panel.println("Brown Fox ");
  panel.setTextColor(RED);  
  panel.println("Jumped    ");
  panel.setTextColor(MAGENTA);  
  panel.println("Over the  ");
  panel.setTextColor(RED);  
  panel.println(" South Ldn");
  panel.setTextColor(WHITE);  
  panel.println("MakerSpace");
}


void testText() {
  panel.fillScreen(BLACK);
  panel.setCursor(0, 0);
  panel.setTextColor(WHITE);  
  panel.setTextSize(1);
  panel.println("Hello!");
  panel.setTextColor(YELLOW); 
  panel.setTextSize(1);
  panel.println(1234.56);
  panel.setTextColor(RED);    
  panel.setTextSize(1);
  panel.println(0xDEADBEEF, HEX);
  panel.setTextColor(GREEN);
  panel.setTextSize(1);
  panel.println("Groop");
  panel.println("I implore thee...");
}
*/
void testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = panel.width(), h = panel.height();

  panel.fillScreen(BLACK);
  for(y=0; y<h; y+=4) panel.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=4) panel.drawFastVLine(x, 0, h, color2);
}

void testFilledRects(uint16_t color1, uint16_t color2) {
  int n, i, i2,
    cx = panel.width()  / 2 - 1,
    cy = panel.height() / 2 - 1;

  panel.fillScreen(BLACK);
  n = min(panel.width(), panel.height());
  for(i=n; i>0; i-=6) {
    i2 = i / 2;
    panel.fillRect(cx-i2, cy-i2, i, i, color1);
    panel.drawRect(cx-i2, cy-i2, i, i, color2);
  }
}

/*
void testFilledCircles(uint8_t radius, uint16_t color) {
  int x, y, 
    w = panel.width(), 
    h = panel.height(), 
    r2 = radius * 2;

  panel.fillScreen(BLACK);
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      panel.fillCircle(x, y, radius, color);
    }
  }
}

void testTriangles() {
  int n, i, 
    cx = panel.width()  / 2 - 1,
    cy = panel.height() / 2 - 1;

  panel.fillScreen(BLACK);
  n = min(cx, cy);
  for(i=0; i<n; i+=4) {
    panel.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      RED);
  }
}

void testRoundRects() {
  int w, i, i2,
    cx = panel.width()  / 2 - 1,
    cy = panel.height() / 2 - 1;

  panel.fillScreen(BLACK);
  w = min(panel.width(), panel.height());
  for(i=0; i<w; i+=4) {
    i2 = i / 2;
    panel.drawRoundRect(cx-i2, cy-i2, i, i, i/8, YELLOW);
  }
}

*/


void testFonts(){
  panel.setFont(&FreeSerifBoldItalic9pt7b);
  panel.setTextColor(GREEN);  
  panel.setTextSize(0);
  panel.setCursor(0, 32);
  panel.println(F("Hello"));
  delay(2000);
  panel.setFont();

}


void loop(){
      panel.FillBuffer(0x00);
      panel.setCursor(3, 18);
      panel.setTextColor(WHITE);  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(1500);
    for (int ct=0; ct <= 20; ct++){
      panel.setCursor(3, 18);
      panel.setTextColor(random(1, WHITE));  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(100);
    }
      panel.setCursor(3, 18);
      panel.setTextColor(WHITE);  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(3000); 

    for (int ct=0; ct < 256; ct += 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(ct, ct, ct));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=256-8; ct > 0; ct -= 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(ct, ct, ct));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=0; ct < 256; ct += 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(0, ct, 0));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=256-8; ct > 0; ct -= 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(0, ct, 0));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=0; ct < 256; ct += 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(0, 0, ct));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=256-8; ct > 0; ct -= 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(0, 0, ct));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=0; ct < 256; ct += 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(ct, 0, 0));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
    for (int ct=256-8; ct > 0; ct -= 8)
    {
      panel.FillBuffer(0x00);
      panel.setCursor(1, 18);
      panel.setTextColor(RGB565(ct, 0, 0));  
      panel.setTextSize(1);
      panel.printf("Fade %d", ct);
      delay(400);
    }
      delay(3000); 

    for(int h = 0; h < 48; h++)
    {
      panel.drawBitmap(0, h, (const uint8_t *)&LHSlogoBitmap, 64, 64, WHITE, BLUE);
      delay(25);
    }
    for(int h = 48; h >= 0; h--)
    {
      panel.drawBitmap(0, h, (const uint8_t *)&LHSlogoBitmap, 64, 64, WHITE, BLUE);
      delay(25);
    }

return;

      panel.FillBuffer(0x00);
      panel.setTextSize(1);
 
      panel.setCursor(24, 5);
      panel.setTextColor(YELLOW);      
      panel.println(F("And"));
      delay(300); 
      panel.setTextColor(GREEN);       
      panel.setCursor(12, 20);
      panel.println(F("Welcome"));
      delay(300);
       panel.setTextColor(CYAN);      
      panel.setCursor(24, 35);
      panel.println(F("too"));      
      delay(3000); 

   for (int ct=0; ct <= 44; ct++){
      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(0, ct);
      panel.setTextColor(WHITE);  
      panel.println(F(" South Ldn"));
      panel.setCursor(2, ct+9);
      panel.setTextColor(WHITE);  
      panel.println(F("MakerSpace"));
      delay(50);
   } 

   
      panel.drawBitmap(16, 6, (const uint8_t *)&slms32x32, 32, 32, RED, BLACK);
      delay(6000);

      panel.drawBitmap(0, 0, (const uint8_t *)&paul64, 64, 64, GREEN, BLACK);
      delay(5000);

      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(30, 5);
      panel.setTextColor(YELLOW);      
      panel.println(F("A"));
      delay(300); 
      panel.setTextColor(GREEN);       
      panel.setCursor(6, 20);
      panel.println(F("Community"));
      delay(300);
      panel.setTextColor(MAGENTA);      
      panel.setCursor(19, 35);
      panel.println(F("Owned"));

      delay(300);
      panel.setTextColor(CYAN);      
      panel.setCursor(6, 50);
      panel.println(F("Workspace"));      
      delay(3000); 


      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(2, 5);
      panel.setTextColor(WHITE);      
      panel.println(F("Electronic"));
      delay(300); 
      panel.setTextColor(YELLOW);       
      panel.setCursor(6, 20);
      panel.println(F("Wood Work"));
      delay(300);
      panel.setTextColor(MAGENTA);      
      panel.setCursor(8, 35);
      panel.println(F("Textiles"));
      delay(300);
      panel.setTextColor(GREEN);      
      panel.setCursor(8, 50);
      panel.println(F("Printing"));      
      delay(5000); 

      panel.FillBuffer(0x00);
      panel.setTextSize(2);
      
      panel.setCursor(10, 8);
      panel.setTextColor(MAGENTA);  
      panel.println(F("Join"));
      panel.setCursor(3, 38);
      panel.setTextColor(CYAN);  
      panel.println(F("Today"));

      
      delay(8000); 

int ledState;
for (int ct=0; ct <= 11; ct++){
    if (ledState == LOW) {
        panel.FillBuffer(0x00);
        panel.drawBitmap(ct*2, 20, (const uint8_t *)&Invader1, 24, 18, BLACK, GREEN);
        delay(300); 
      ledState = HIGH;
    } else {
        panel.FillBuffer(0x00);
        panel.drawBitmap(ct*2, 20, (const uint8_t *)&Invader2, 24, 18, BLACK, GREEN);
        delay(300);  
      ledState = LOW;
    } 
}

      panel.drawLine(0, 0, 30, 25, WHITE);
      delay(150);
      panel.drawLine(0, 0, 30, 25, RED);
      delay(50);
      panel.drawLine(0, 0, 30, 25, WHITE);
      delay(50);
      panel.drawLine(0, 0, 30, 25, RED);
      delay(50);
      panel.drawLine(0, 0, 30, 25, WHITE);     
      delay(100);
      panel.drawBitmap(22, 20, (const uint8_t *)&Invader2, 24, 18, BLACK, RED);
      delay(100);       
      panel.drawBitmap(22, 20, (const uint8_t *)&Invader2, 24, 18, BLACK, WHITE);
      delay(200);
   
      
for (int ct=0; ct <= 500; ct++){
      panel.drawPixel(random(0, 64), random(0, 64), random(BLACK, WHITE));
      delayMicroseconds(50);
}
for (int ct=0; ct <= 1500; ct++){
      panel.drawPixel(random(0, 64), random(0, 64), WHITE);
      delayMicroseconds(50);
}

      delay(50);
      panel.FillBuffer(0xff);
      delay(250);
      panel.FillBuffer(0x00);
      delay(150);
      panel.FillBuffer(0xff);
      delay(150);
      panel.FillBuffer(0x00);      
      
delay(4000);
}
