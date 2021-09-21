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
#include "bmp.h"
//#include "digitalWriteFast.h"
//#include <Ticker.h>
#include "HHLedPanel_4x64x16.h"
#include "ESP32_4xMBI5034.h"
#include "LedPanelDisplay.h"

/*
class LedPanel : public Adafruit_GFX
{
  public:
    HHLedPanelHardware64x8x2 _panel;
    HHLedPanelPlatformESP32 _platform;
    
    LedPanel() : Adafruit_GFX(64,64)
    {}
    void drawPixel(int16_t x, int16_t y, uint16_t color);
  //  uint16_t newColor(uint8_t red, uint8_t green, uint8_t blue);
  //  uint16_t getColor() { return textcolor; }
    void drawBitmapMem(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void FillBuffer(byte b)
    {
      _panel.FillBuffer(b);
    }
};
*/

LedPanelDisplay<HHLedPanel_4x64x16<ESP32_4xMBI5034>> panel;
//template<class ESP32_4xMBI5034> byte HHLedPanel_4x64x16<ESP32_4xMBI5034>::frame[4][24*16];

/*
#define PIN_D1    3   //PD0 - D1 on Panel 1
#define PIN_D2    2   //PD1 - D2 on Panel 1
#define PIN_D3    0   //PD2 - D1 on Panel 2
#define PIN_D4    1   //PD3 - D2 on Panel 2

#define PIN_D5    8   //PB4 - D1 on Panel 3
#define PIN_D6    9   //PB5 - D2 on Panel 3
#define PIN_D7    10  //PB6 - D1 on Panel 4
#define PIN_D8    11  //PB7 - D2 on Panel 4

// bb describes which data lines drive which of the 4 panels.
// By adjusting the order of the bits in the array, you can change the panel order visually.
byte bb[8] = { 0x40, 0x80, 0x10, 0x20, 0x04, 0x08, 0x01, 0x02 };

#define PIN_A0    A4  //PF1 - A0 on all Panels
#define PIN_A1    A1  //PF6 - A1 on all Panels
#define PIN_CLK   A3  //PF4 - CLK on all Panels
#define PIN_LAT   A2  //PF5 - LAT on all Panels
#define PIN_OE    A5  //PF0 - OE on all Panels
*/

/*
// These are for Arduino Nano:
#define PIN_D1    4   //PD4 - D1 on Panel 1 (Bottom)
#define PIN_D2    5   //PD5 - D2 on Panel 1
#define PIN_D3    6   //PD6 - D1 on Panel 2
#define PIN_D4    7   //PD7 - D2 on Panel 2

#define PIN_D5    8   //PB0 - D1 on Panel 3
#define PIN_D6    9   //PB1 - D2 on Panel 3
#define PIN_D7    10  //PB2 - D1 on Panel 4
#define PIN_D8    11  //PB3 - D2 on Panel 4 (Top)

// bb describes which data lines drive which of the 4 panels.
// By adjusting the order of the bits in the array, you can change the panel order visually.
static const byte bb[8] = {0x04, 0x08,  0x01, 0x02, 0x40, 0x80, 0x10, 0x20 };

#define PIN_A0    A4  //PF1 - A0 on all Panels
#define PIN_A1    A1  //PF6 - A1 on all Panels
#define PIN_CLK   A3  //PF4 - CLK on all Panels
#define PIN_LAT   A2  //PF5 - LAT on all Panels
#define PIN_OE    A5  //PF0 - OE on all Panels
*/

// These are for the ESP32-DevKitC board with WROOM chip
/*
#define PIN_D1    GPIO_NUM_23   //PD4 - D1 on Panel 1 (Bottom)
#define PIN_D2    GPIO_NUM_19   //PD5 - D2 on Panel 1
#define PIN_D3    GPIO_NUM_6   //PD6 - D1 on Panel 2
#define PIN_D4    GPIO_NUM_7   //PD7 - D2 on Panel 2

#define PIN_D5    GPIO_NUM_8   //PB0 - D1 on Panel 3
#define PIN_D6    GPIO_NUM_9   //PB1 - D2 on Panel 3
#define PIN_D7    GPIO_NUM_10  //PB2 - D1 on Panel 4
#define PIN_D8    GPIO_NUM_11  //PB3 - D2 on Panel 4 (Top)

// bb describes which data lines drive which of the 4 panels.
// By adjusting the order of the bits in the array, you can change the panel order visually.
//static const byte bb[8] = {0x04, 0x08,  0x01, 0x02, 0x40, 0x80, 0x10, 0x20 };

#define PIN_A0    GPIO_NUM_16  //PF1 - A0 on all Panels
#define PIN_A1    GPIO_NUM_4  //PF6 - A1 on all Panels
#define PIN_CLK   GPIO_NUM_15  //PF4 - CLK on all Panels
#define PIN_LAT   GPIO_NUM_14  //PF5 - LAT on all Panels
#define PIN_OE    GPIO_NUM_12  //PF0 - OE on all Panels
*/

#define LED_BLACK 0
#define LED_BLUE 1
#define LED_GREEN 2
#define LED_CYAN 3
#define LED_RED 4
#define LED_MAGENTA 5
#define LED_YELLOW 6
#define LED_WHITE 7

/*
byte frame[4][384];

void FillBuffer(byte b){
  for(uint8_t x=0; x<4; x++){
    for(uint16_t y=0; y<384; y++){
      frame[x][y] = b;
    }
  }
}
*/
/*
void LedPanel::drawPixel(int16_t x, int16_t y, uint16_t color) {
  _panel.setpixel(x,y,color);
}
*/
//uint16_t LedPanel::newColor(uint8_t red, uint8_t green, uint8_t blue) {
//  return (blue>>7) | ((green&0x80)>>6) | ((red&0x80)>>5);
//}
/*
void LedPanel::drawBitmapMem(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
        panel.drawPixel(x+i, y+j, color);
      }
    }
  }
}
*/

/*
// Set a pixel to a specific 3 bit colour (8 colours)
// 0b000 = black (off), 0b001 = Blue, 0b010 = Green, 0b100 = Red, 0b011 = Cyan, 0b101 = Magenta, 0b110 = Yellow, 0b111 = White, etc.
void setpixel(byte x, byte y, byte col) {
  int16_t off = (x&7) + (x & 0xf8)*6 + ((y & 4)*2);
//  int16_t off = (x&7) + (x >> 3)*48 + ((y & 4)*2);
  byte row = y & 3;
  byte b = bb[(y&0x3f) >> 3];
  byte *p = & frame[row][off];
  for(byte c = 0; c < 3;c++) {
    if ( col & 1 ) {
      *p |= b;
    } else {
      *p &= ~b;
    }
    col >>= 1;
    p += 16;
  }
}
*/


/*
void IRAM_ATTR UpdateFrame() {
  byte * f = frame[bank];
  for (uint16_t n = 0; n<384; n++) {
    gpio_set_level(PIN_D1, (*f & 1) != 0);
    gpio_set_level(PIN_D2, (*f++ & 2) != 0);
    gpio_set_level(PIN_CLK, 0);
    gpio_set_level(PIN_CLK, 1);

    /*
    PORTD = *f;      // We use the low nibble on PortD for Panel 1 & 2
    PORTB = *f++;    // We use the high nibble on PortB for Panel 3 & 4
    digitalWriteFast(PIN_CLK, LOW);
    digitalWriteFast(PIN_CLK, HIGH);
    */
/*    }


  REG_WRITE(GPIO_OUT_W1TS_REG, 1<<PIN_OE);     // disable output
  //gpio_set_level(PIN_OE,1);     // disable output
  if (bank & 0x01) {
    gpio_set_level(PIN_A0, 1);
  } else {
    gpio_set_level(PIN_A0, 0);
  }
  if (bank & 0x02) {
    gpio_set_level(PIN_A1, 1);
  } else {
    gpio_set_level(PIN_A1, 0);
  }
  gpio_set_level(PIN_LAT, 1);   // toggle latch
  gpio_set_level(PIN_LAT, 0);
  gpio_set_level(PIN_OE, 0);     // enable output

  //Neil - dimming
  if(MAX_BRIGHTNESS < 100)
  {
    delayMicroseconds(MAX_BRIGHTNESS*4);
    //GPIO.out_w1ts = PIN_OE;
    //REG_WRITE(GPIO_OUT_W1TS_REG, 1<<PIN_OE);
    //gpio_set_level(PIN_OE, 1);     // disable output
  }
  //Neil


/*
  digitalWriteFast(PIN_OE,HIGH);     // disable output
  if (bank & 0x01) {
    digitalWriteFast(PIN_A0, HIGH);
  } else {
    digitalWriteFast(PIN_A0, LOW);
  }
  if (bank & 0x02) {
    digitalWriteFast(PIN_A1, HIGH);
  } else {
    digitalWriteFast(PIN_A1, LOW);
  }
  digitalWriteFast(PIN_LAT, HIGH);   // toggle latch
  digitalWriteFast(PIN_LAT, LOW);
  digitalWriteFast(PIN_OE, LOW);     // enable output

  //Neil - dimming
  if(MAX_BRIGHTNESS < 100)
  {
    delayMicroseconds(MAX_BRIGHTNESS*4);
    digitalWriteFast(PIN_OE,HIGH);     // disable output
  }
  //Neil
*/
/*
  if (++bank>3) bank=0;
}
*/
/*
void IRAM_ATTR UpdateFrame() {
  byte * f = panel._panel.frame[bank];
  GPIO.out_w1ts = BIT(PIN_OE);      // disable output
  for (uint16_t n = 0; n<384; n++) 
  {
/*    gpio_set_level(PIN_D1, (*f & 1) != 0);
    gpio_set_level(PIN_D2, (*f++ & 2) != 0);
    gpio_set_level(PIN_CLK, 0);
    gpio_set_level(PIN_CLK, 1);
   */    
  /*  GPIO.out = ((*f & 1) << PIN_D1) | ((*(f++) & 2) >> 1 << PIN_D2) | BIT(PIN_OE);
    GPIO.out_w1ts = BIT(PIN_CLK);  

   }

/*  
  REG_WRITE(GPIO_OUT_W1TS_REG, 1<<PIN_OE);     // disable output
  //gpio_set_level(PIN_OE,1);     // disable output
  if (bank & 0x01) {
    gpio_set_level(PIN_A0, 1);
  } else {
    gpio_set_level(PIN_A0, 0);
  }
  if (bank & 0x02) {
    gpio_set_level(PIN_A1, 1);
  } else {
    gpio_set_level(PIN_A1, 0);
  }
  gpio_set_level(PIN_LAT, 1);   // toggle latch
  gpio_set_level(PIN_LAT, 0);
  gpio_set_level(PIN_OE, 0);     // enable output

  //Neil - dimming
  if(MAX_BRIGHTNESS < 100)
  {
    delayMicroseconds(MAX_BRIGHTNESS*4);
    //GPIO.out_w1ts = PIN_OE;
    //REG_WRITE(GPIO_OUT_W1TS_REG, 1<<PIN_OE);
    //gpio_set_level(PIN_OE, 1);     // disable output
  }
  //Neil
*/
  /*
  uint32_t addr = ((bank & 1) << PIN_A0) | ((bank & 2) >> 1 << PIN_A1);
  GPIO.out = addr | BIT(PIN_OE);  // Set address
  GPIO.out_w1ts = BIT(PIN_LAT);   // toggle latch
  GPIO.out_w1tc = BIT(PIN_LAT); 
  GPIO.out_w1tc = BIT(PIN_OE);    // enable output

  //Neil - dimming
  if(MAX_BRIGHTNESS < 100)
  {
    delayMicroseconds(MAX_BRIGHTNESS*4);
    GPIO.out_w1ts = BIT(PIN_OE);      // disable output
  }
  //Neil

  if (++bank>3) bank=0;
}
*/

//Ticker timer1(UpdateFrame, 100, -1);

void setup() {
  //Serial.begin(115200);
  //Serial.println("Begin...");
  /*
  gpio_pad_select_gpio(PIN_D1);
  gpio_set_direction(PIN_D1, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D2);
  gpio_set_direction(PIN_D2, GPIO_MODE_OUTPUT);
 
 /* pinMode(PIN_D1, OUTPUT);
  pinMode(PIN_D2, OUTPUT);
  pinMode(PIN_D3, OUTPUT);
  pinMode(PIN_D4, OUTPUT);

  pinMode(PIN_D5, OUTPUT);
  pinMode(PIN_D6, OUTPUT);
  pinMode(PIN_D7, OUTPUT);
  pinMode(PIN_D8, OUTPUT);
*/
/*
  gpio_pad_select_gpio(PIN_A0);
  gpio_set_direction(PIN_A0, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_A1);
  gpio_set_direction(PIN_A1, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_CLK);
  gpio_set_direction(PIN_CLK, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_LAT);
  gpio_set_direction(PIN_LAT, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_OE);
  gpio_set_direction(PIN_OE, GPIO_MODE_OUTPUT);
/*
  pinMode(PIN_A0, OUTPUT);
  pinMode(PIN_A1, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_LAT, OUTPUT);
  pinMode(PIN_OE, OUTPUT);
*/
/*
  gpio_set_level(PIN_D1, 0);
  gpio_set_level(PIN_D2, 0);
  
  gpio_set_level(PIN_A0, 0);
  gpio_set_level(PIN_A1, 0);
  gpio_set_level(PIN_OE, 1);
  gpio_set_level(PIN_LAT, 1);
  gpio_set_level(PIN_CLK, 0);
*/
/*  digitalWrite(PIN_D1, LOW);
  digitalWrite(PIN_D2, LOW);
  digitalWrite(PIN_D3, LOW);
  digitalWrite(PIN_D4, LOW);

  digitalWrite(PIN_A0, LOW);
  digitalWrite(PIN_A1, LOW);

  digitalWrite(PIN_OE, HIGH);
  digitalWrite(PIN_LAT, LOW);
  digitalWrite(PIN_CLK, LOW);
*/

  //  FillBuffer(0xFF);         // Set all LEDs on. (White)
  panel.FillBuffer(0x00);         // Set all LEDs off. (Black)

panel.initialise();

  panel.FillBuffer(0x00);
  panel.setTextSize(1);
  panel.setCursor(0, 0);
  panel.setTextColor(LED_WHITE);  
  panel.println(F("TOP - 0,0"));
  panel.setCursor(0, 16);
  panel.setTextColor(LED_WHITE);  
  panel.println(F("TMID - 0,16"));
  panel.setCursor(0, 32);
  panel.setTextColor(LED_GREEN);  
  panel.println(F("BMID - 0,32"));
  panel.setCursor(0, 48);
  panel.setTextColor(LED_MAGENTA);  
  panel.println(F("BOT - 0,48"));

  delay(2000);
  panel.fillScreen(LED_WHITE);
  delay(1000);

  //timer1.setCallback(UpdateFrame);
  //timer1.setInterval(100);
  //timer1.start(); 
}

/*
ISR(TIMER1_COMPA_vect){     // timer compare interrupt service routine
  UpdateFrame();
}
*/

/*
void testText1() {
  panel.FillBuffer(0x00);
  panel.setCursor(0, 0);
  panel.setTextColor(LED_GREEN);  
  panel.println("The Quick ");
  panel.setTextColor(LED_CYAN);  
  panel.println("Brown Fox ");
  panel.setTextColor(LED_RED);  
  panel.println("Jumped    ");
  panel.setTextColor(LED_MAGENTA);  
  panel.println("Over the  ");
  panel.setTextColor(LED_RED);  
  panel.println(" South Ldn");
  panel.setTextColor(LED_WHITE);  
  panel.println("MakerSpace");

}


void testText2() {
  panel.FillBuffer(0x00);
  panel.setTextSize(1);
  panel.setCursor(15, 17);
  panel.setTextColor(LED_WHITE);  
  panel.println(F("COURTY"));
  panel.setCursor(8, 28);
  panel.setTextColor(LED_MAGENTA);  
  panel.println(F("(C) 2017"));

}



void testText3() {
  panel.FillBuffer(0x00);
  panel.setCursor(3, 22);
  panel.setTextColor(LED_YELLOW);  
  panel.setTextSize(2);
  panel.println(F("Hello"));
}


  panel.setTextColor(LED_GREEN);  
  panel.println("The Quick ");
  panel.setTextColor(LED_CYAN);  
  panel.println("Brown Fox ");
  panel.setTextColor(LED_RED);  
  panel.println("Jumped    ");
  panel.setTextColor(LED_MAGENTA);  
  panel.println("Over the  ");
  panel.setTextColor(LED_RED);  
  panel.println(" South Ldn");
  panel.setTextColor(LED_WHITE);  
  panel.println("MakerSpace");
}


void testText() {
  panel.fillScreen(LED_BLACK);
  panel.setCursor(0, 0);
  panel.setTextColor(LED_WHITE);  
  panel.setTextSize(1);
  panel.println("Hello!");
  panel.setTextColor(LED_YELLOW); 
  panel.setTextSize(1);
  panel.println(1234.56);
  panel.setTextColor(LED_RED);    
  panel.setTextSize(1);
  panel.println(0xDEADBEEF, HEX);
  panel.setTextColor(LED_GREEN);
  panel.setTextSize(1);
  panel.println("Groop");
  panel.println("I implore thee...");
}
*/
void testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = panel.width(), h = panel.height();

  panel.fillScreen(LED_BLACK);
  for(y=0; y<h; y+=4) panel.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=4) panel.drawFastVLine(x, 0, h, color2);
}

void testFilledRects(uint16_t color1, uint16_t color2) {
  int n, i, i2,
    cx = panel.width()  / 2 - 1,
    cy = panel.height() / 2 - 1;

  panel.fillScreen(LED_BLACK);
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

  panel.fillScreen(LED_BLACK);
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

  panel.fillScreen(LED_BLACK);
  n = min(cx, cy);
  for(i=0; i<n; i+=4) {
    panel.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      LED_RED);
  }
}

void testRoundRects() {
  int w, i, i2,
    cx = panel.width()  / 2 - 1,
    cy = panel.height() / 2 - 1;

  panel.fillScreen(LED_BLACK);
  w = min(panel.width(), panel.height());
  for(i=0; i<w; i+=4) {
    i2 = i / 2;
    panel.drawRoundRect(cx-i2, cy-i2, i, i, i/8, LED_YELLOW);
  }
}

*/


void testFonts(){
  panel.setFont(&FreeSerifBoldItalic9pt7b);
  panel.setTextColor(LED_GREEN);  
  panel.setTextSize(0);
  panel.setCursor(0, 32);
  panel.println(F("Hello"));
  delay(2000);
  panel.setFont();

}


void loop(){
      panel.FillBuffer(0x00);
      panel.setCursor(3, 18);
      panel.setTextColor(LED_WHITE);  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(1500);
    for (int ct=0; ct <= 20; ct++){
      panel.setCursor(3, 18);
      panel.setTextColor(random(1, 7));  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(100);
    }
      panel.setCursor(3, 18);
      panel.setTextColor(LED_WHITE);  
      panel.setTextSize(2);
      panel.println(F("Hello"));
      delay(3000); 


      panel.FillBuffer(0x00);
      panel.setTextSize(1);
 
      panel.setCursor(24, 5);
      panel.setTextColor(LED_YELLOW);      
      panel.println(F("And"));
      delay(300); 
      panel.setTextColor(LED_GREEN);       
      panel.setCursor(12, 20);
      panel.println(F("Welcome"));
      delay(300);
       panel.setTextColor(LED_CYAN);      
      panel.setCursor(24, 35);
      panel.println(F("too"));      
      delay(3000); 

   for (int ct=0; ct <= 44; ct++){
      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(0, ct);
      panel.setTextColor(LED_WHITE);  
      panel.println(F(" South Ldn"));
      panel.setCursor(2, ct+9);
      panel.setTextColor(LED_WHITE);  
      panel.println(F("MakerSpace"));
      delay(50);
   } 
   
      panel.drawBitmap(16, 6, (const uint8_t *)&slms32x32, 32, 32, LED_RED, LED_BLACK);
      delay(6000);

      panel.drawBitmap(0, 0, (const uint8_t *)&paul64, 64, 64, LED_GREEN, LED_BLACK);
      delay(5000);

      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(30, 5);
      panel.setTextColor(LED_YELLOW);      
      panel.println(F("A"));
      delay(300); 
      panel.setTextColor(LED_GREEN);       
      panel.setCursor(6, 20);
      panel.println(F("Community"));
      delay(300);
      panel.setTextColor(LED_MAGENTA);      
      panel.setCursor(19, 35);
      panel.println(F("Owned"));

      delay(300);
      panel.setTextColor(LED_CYAN);      
      panel.setCursor(6, 50);
      panel.println(F("Workspace"));      
      delay(3000); 


      panel.FillBuffer(0x00);
      panel.setTextSize(1);
      panel.setCursor(2, 5);
      panel.setTextColor(LED_WHITE);      
      panel.println(F("Electronic"));
      delay(300); 
      panel.setTextColor(LED_YELLOW);       
      panel.setCursor(6, 20);
      panel.println(F("Wood Work"));
      delay(300);
      panel.setTextColor(LED_MAGENTA);      
      panel.setCursor(8, 35);
      panel.println(F("Textiles"));
      delay(300);
      panel.setTextColor(LED_GREEN);      
      panel.setCursor(8, 50);
      panel.println(F("Printing"));      
      delay(5000); 

      panel.FillBuffer(0x00);
      panel.setTextSize(2);
      
      panel.setCursor(10, 8);
      panel.setTextColor(LED_MAGENTA);  
      panel.println(F("Join"));
      panel.setCursor(3, 38);
      panel.setTextColor(LED_CYAN);  
      panel.println(F("Today"));

      
      delay(8000); 

int ledState;
for (int ct=0; ct <= 11; ct++){
    if (ledState == LOW) {
        panel.FillBuffer(0x00);
        panel.drawBitmap(ct*2, 20, (const uint8_t *)&Invader1, 24, 18, LED_BLACK, LED_GREEN);
        delay(300); 
      ledState = HIGH;
    } else {
        panel.FillBuffer(0x00);
        panel.drawBitmap(ct*2, 20, (const uint8_t *)&Invader2, 24, 18, LED_BLACK, LED_GREEN);
        delay(300);  
      ledState = LOW;
    } 
}

      panel.drawLine(0, 0, 30, 25, LED_WHITE);
      delay(150);
      panel.drawLine(0, 0, 30, 25, LED_RED);
      delay(50);
      panel.drawLine(0, 0, 30, 25, LED_WHITE);
      delay(50);
      panel.drawLine(0, 0, 30, 25, LED_RED);
      delay(50);
      panel.drawLine(0, 0, 30, 25, LED_WHITE);     
      delay(100);
      panel.drawBitmap(22, 20, (const uint8_t *)&Invader2, 24, 18, LED_BLACK, LED_RED);
      delay(100);       
      panel.drawBitmap(22, 20, (const uint8_t *)&Invader2, 24, 18, LED_BLACK, LED_WHITE);
      delay(200);
   
      
for (int ct=0; ct <= 500; ct++){
      panel.drawPixel(random(0, 64), random(0, 64), random(0, 7));
      delayMicroseconds(50);
}
for (int ct=0; ct <= 1500; ct++){
      panel.drawPixel(random(0, 64), random(0, 64), LED_WHITE);
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
