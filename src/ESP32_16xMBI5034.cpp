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
This class encapsulates the platform-specific driver for refreshing a set of
4 LED display panels using MBI5034 chips arranged as 2 data pins per panel
(i.e. maximum of 8 data lines currently).

This version uses a fast timer interrupt to output one logical row (2 physical 
rows) per interrupt and allows up to a 5-bit colour depth to be used.
******************************************************************************/
#include "ESP32_16xMBI5034.h"
#include "ESP32_4xMBI5034_Pins.h"
#include <driver/rtc_io.h>

// Statics
static hw_timer_t *timer_Refresh = 0;
static byte *_frameBuffers; // Array of [depth][bank][leds]
static uint8_t _colourDepth;
static uint8_t _planes;
static uint16_t _bytesToSend;

//////////////////////////////////////////////////////////////////////////
// This piece of compile-time magic creates a lookup table to convert
// from the buffer byte values into the GPIO 32-bit values, swapping around
// the pins to the necessary phsical bit positions.
//
// Currenly only GPIO pins 0 to 31 are supported for the data and address lines
// as only a single write is performed to output the data in parallel
//
constexpr uint32_t gpio_mapping(uint16_t bits)
{
  return 
      ((bits & 1) << PIN_D1) | 
      (PIN_D2 >= 1 ? ((bits & 2) << (PIN_D2 - 1)) : ((bits & 2) >> (1 - PIN_D2))) | 
      (PIN_D3 >= 2 ? ((bits & 4) << (PIN_D3 - 2)) : ((bits & 4) >> (2 - PIN_D3))) | 
      (PIN_D4 >= 3 ? ((bits & 8) << (PIN_D4 - 3)) : ((bits & 8) >> (3 - PIN_D4))) | 
      (PIN_D5 >= 4 ? ((bits & 0x10) << (PIN_D5 - 4)) : ((bits & 0x10) >> (4 - PIN_D5))) | 
      (PIN_D6 >= 5 ? ((bits & 0x20) << (PIN_D6 - 5)) : ((bits & 0x20) >> (5 - PIN_D6))) | 
      (PIN_D7 >= 6 ? ((bits & 0x40) << (PIN_D7 - 6)) : ((bits & 0x40) >> (6 - PIN_D7))) | 
      (PIN_D8 >= 7 ? ((bits & 0x80) << (PIN_D8 - 7)) : ((bits & 0x80) >> (7 - PIN_D8)));
}

template<uint16_t N, uint32_t... Rest>
struct Mapping 
{
  static constexpr auto& value = Mapping<N-1, gpio_mapping(N), Rest...>::value;
};

template<uint32_t... Rest>
struct Mapping <0, Rest...>
{
  static constexpr uint32_t value[] = {0, Rest...};
};

template<uint32_t... Rest>
constexpr uint32_t Mapping<0, Rest...>::value[];

//static constexpr const uint32_t *HardwareGpioMapping = Mapping<256>::value;
static uint32_t *HardwareGpioMapping;

//////////////////////////////////////////////////////////////////////////


void ESP32_16xMBI5034::Initialise(byte *frameBuffers, uint8_t colourDepth, uint8_t planes, uint16_t bytesToSend)
{
  _frameBuffers = frameBuffers;
  _colourDepth = colourDepth;
  _planes = planes;
  _bytesToSend = bytesToSend;

  // ensure GPIO pin mapping is in RAM (needed if you also use SPIFFS)
  if(!HardwareGpioMapping)
  {
    HardwareGpioMapping = new uint32_t[256];
    memcpy(HardwareGpioMapping, Mapping<256>::value, sizeof(Mapping<256>::value));
  }
  
  // Set up GPIO lines
  gpio_pad_select_gpio(PIN_D1);
  gpio_set_direction(PIN_D1, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D2);
  gpio_set_direction(PIN_D2, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D3);
  gpio_set_direction(PIN_D3, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D4);
  gpio_set_direction(PIN_D4, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D5);
  gpio_set_direction(PIN_D5, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D6);
  gpio_set_direction(PIN_D6, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D7);
  gpio_set_direction(PIN_D7, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_D8);
  gpio_set_direction(PIN_D8, GPIO_MODE_OUTPUT);
 
  gpio_pad_select_gpio(PIN_A0);
  gpio_set_direction(PIN_A0, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_A1);
  gpio_set_direction(PIN_A1, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(PIN_CLK0);
  gpio_set_direction(PIN_CLK0, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_CLK1);
  gpio_set_direction(PIN_CLK1, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_CLK2);
  gpio_set_direction(PIN_CLK2, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_CLK3);
  gpio_set_direction(PIN_CLK3, GPIO_MODE_OUTPUT);

/*
  // Disable RTC if it clashes with the CLK pin allocations
  if(rtc_gpio_is_valid_gpio(PIN_CLK0)) rtc_gpio_deinit(PIN_CLK0);
  if(rtc_gpio_is_valid_gpio(PIN_CLK1)) rtc_gpio_deinit(PIN_CLK1);
  if(rtc_gpio_is_valid_gpio(PIN_CLK2)) rtc_gpio_deinit(PIN_CLK2);
  if(rtc_gpio_is_valid_gpio(PIN_CLK3)) rtc_gpio_deinit(PIN_CLK3);
*/
//  REG_CLR_BIT(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32N_MUX_SEL);
  
  gpio_pad_select_gpio(PIN_LAT);
  gpio_set_direction(PIN_LAT, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_OE);
  gpio_set_direction(PIN_OE, GPIO_MODE_OUTPUT);

  gpio_set_level(PIN_OE, 1);
  gpio_set_level(PIN_LAT, 0);
  
  gpio_set_level(PIN_CLK0, 0);
  gpio_set_level(PIN_CLK1, 0);
  gpio_set_level(PIN_CLK2, 0);
  gpio_set_level(PIN_CLK3, 0);
  
  // Create the interrupts to refresh the panels
  timer_Refresh = timerBegin(REFRESH_TIMER_NUMBER, 80, true);  // 80 for Microseconds
  timerAttachInterrupt(timer_Refresh, &RefreshInterrupt, true);
  timerAlarmWrite(timer_Refresh, (REFRESH_INTERVAL_uS >> _colourDepth >> 2), true);
}

void ESP32_16xMBI5034::StartDisplay()
{
  // Start refresh
  timerAlarmEnable(timer_Refresh);
}

void IRAM_ATTR ESP32_16xMBI5034::RefreshInterrupt()
{
	static uint8_t bank = 0;
	static uint8_t depth = 0;   // 0 = for top bit
	static uint8_t intCount = 0;

	// We ignore the interrups unless the refresh time has expired for the bit currently being displayed
	if(!(++intCount >> (_colourDepth - depth - 1)))
		return;
	intCount = 0;
	
	GPIO.out_w1ts = BIT(PIN_OE);      // disable output

	if (++bank >= _planes) 
	{
		bank=0;
		if (++depth >= _colourDepth) 
		{
		  depth=0;
		}
	}

	byte *f = _frameBuffers + (bank + depth*_planes)*_bytesToSend;

	// Get the current port state, so we don't change unrelated lines
	uint32_t out = GPIO.out
                & ~(BIT(PIN_D1) | BIT(PIN_D2) | BIT(PIN_D3) | BIT(PIN_D4) | BIT(PIN_D5) | BIT(PIN_D6) | BIT(PIN_D7) | BIT(PIN_D8) | BIT(PIN_A0) | BIT(PIN_A1) | BIT(PIN_LAT) | BIT(PIN_CLK0) | BIT(PIN_CLK1) | BIT(PIN_CLK2) | BIT(PIN_CLK3)) 
                | BIT(PIN_OE); 
  
	for (uint16_t n = 0; n < _bytesToSend/4; n++) 
	{
		// Update all 4 panels using 2 data lines/panel using mapping table
		// this version takes about 39uS for all 384 outputs, i.e. 10MHz rate
		GPIO.out = out | HardwareGpioMapping[*f++];
		GPIO.out_w1ts = BIT(PIN_CLK0);  
	}
	for (uint16_t n = 0; n < _bytesToSend/4; n++) 
	{
		// Update all 4 panels using 2 data lines/panel using mapping table
		// this version takes about 39uS for all 384 outputs, i.e. 10MHz rate
		GPIO.out = out | HardwareGpioMapping[*f++];
		GPIO.out_w1ts = BIT(PIN_CLK1);  
	}
	for (uint16_t n = 0; n < _bytesToSend/4; n++) 
	{
		// Update all 4 panels using 2 data lines/panel using mapping table
		// this version takes about 39uS for all 384 outputs, i.e. 10MHz rate
		GPIO.out = out | HardwareGpioMapping[*f++];
		GPIO.out_w1ts = BIT(PIN_CLK2);  
	}
	for (uint16_t n = 0; n < _bytesToSend/4; n++) 
	{
		// Update all 4 panels using 2 data lines/panel using mapping table
		// this version takes about 39uS for all 384 outputs, i.e. 10MHz rate
		GPIO.out = out | HardwareGpioMapping[*f++];
		GPIO.out_w1ts = BIT(PIN_CLK3);  
	}
	GPIO.out_w1tc = BIT(PIN_CLK3);

	uint32_t addr = ((bank & 1) << PIN_A0) | ((bank & 2) >> 1 << PIN_A1);
	GPIO.out = out | addr; // | BIT(PIN_OE);  // Set address
	GPIO.out_w1ts = BIT(PIN_LAT);   // toggle latch
	GPIO.out_w1tc = BIT(PIN_LAT); 

	GPIO.out_w1tc = BIT(PIN_OE);    // enable output
	timerRestart(timer_Refresh);    // To take into account the data output time
}

/*
 * The Control Port for each chip is a 16-bit value in the following format
 * 
 *  01EE00CCCCHDDDDD
 *  where E=Error detection time (11 default)
 *        C=Check bits (must be 0101)
 *        H=High current
 *        D=Current gain (000000=12.5% to 111111=200%)
 *  default
 *    0b0111000101101011 (0x716B) for 100% gain
 */
void ESP32_16xMBI5034::SetBrightness(uint16_t brighnessPercent)
{
  // Two current ranges are selectable:
  // if H=0, Current = 0.125-0.488
  // if H=1, Current = 0.508-1.938, where 0b1011=1 (100%)
  
  // Stop the refresh interrupt, this will disrupt the setting
  if(timer_Refresh)
  {
	  timerAlarmDisable(timer_Refresh);
  }

  // Calculate the approx brightness control around 100%
  uint32_t brightness = 0;
  const uint32_t brightness100pc = 0x2b;
  if(brighnessPercent <= 12)
    brightness = 0;
  else if(brighnessPercent <= 100)
    brightness = (brighnessPercent - 12) * brightness100pc / 88;
  else if(brighnessPercent >= 200)
  {
    brightness = 63;
  }
  else
    brightness = (brighnessPercent - 100) * (63-brightness100pc) / 100 + 0x2b;

  // Get the current port state, so we don't change unrelated lines
  uint32_t all_D_bits = (BIT(PIN_D1) | BIT(PIN_D2) | BIT(PIN_D3) | BIT(PIN_D4) | BIT(PIN_D5) | BIT(PIN_D6) | BIT(PIN_D7) | BIT(PIN_D8));
  uint32_t out = GPIO.out & ~all_D_bits & ~(BIT(PIN_A0) | BIT(PIN_A1) | BIT(PIN_LAT) | BIT(PIN_CLK0) | BIT(PIN_CLK1) | BIT(PIN_CLK2) | BIT(PIN_CLK3) | BIT(PIN_OE)); 
  
  for(int bank = 0; bank < 4; bank++)
  {
    uint32_t addr = ((bank & 1) << PIN_A0) | ((bank & 2) >> 1 << PIN_A1);
    
    // Need to send the brightness command to each of the chips
    GPIO.out = addr | out | BIT(PIN_OE);  // Set address
    for(int chip = 0; chip < 24; chip++)
    {
      //GPIO.out_w1ts = BIT(PIN_LAT); // Assert LAT, so chip recoginises as a control write
      uint16_t controlRegister = 0b0111000101000000 | brightness;
      for (uint16_t n = 0; n < 16; n++, controlRegister <<= 1) 
      {
        GPIO.out = ((controlRegister & 0x8000) ? all_D_bits : 0)
                    | (n < 12 || chip != 23 ? 0 : BIT(PIN_LAT)) 
                    | addr | out | BIT(PIN_OE);
        GPIO.out_w1ts = (BIT(PIN_CLK0) | BIT(PIN_CLK1) | BIT(PIN_CLK2) | BIT(PIN_CLK3));
      }
      GPIO.out_w1tc = (BIT(PIN_CLK0) | BIT(PIN_CLK1) | BIT(PIN_CLK2) | BIT(PIN_CLK3));
    }
    GPIO.out_w1tc = BIT(PIN_LAT); // Write control
  }
}
