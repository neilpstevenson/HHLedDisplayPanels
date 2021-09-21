/********************************************************************
This class encapsulates the platform driver for a panel
composed of MBI5034 chips, each driving 16 LEDs.
This version is for an ESP32-based board, driving up to 
4 panels.
********************************************************************/
#include "ESP32_4xMBI5034.h"
#include "ESP32_4xMBI5034_Pins.h"
//#include "HHLedPanel_4x64x16.h"

// Statics
static hw_timer_t *timer_Refresh = 0;
static hw_timer_t *timer_OE = 0;
static byte (*_frameBuffers)[1][4][24*16];
static uint8_t _colourDepth;
static uint8_t _planes;
static uint8_t _chips;

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

static constexpr const uint32_t *HardwareGpioMapping = Mapping<256>::value;

//////////////////////////////////////////////////////////////////////////


void ESP32_4xMBI5034::Initialise(byte (&frameBuffers)[1][4][24*16], uint8_t colourDepth, uint8_t planes, uint8_t chips)
{
  //interruptCallbackFn = refreshCallbackFn;
  _frameBuffers = &frameBuffers;
  _colourDepth = colourDepth;
  _planes = planes;
  _chips = chips;
  
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
  gpio_pad_select_gpio(PIN_CLK);
  gpio_set_direction(PIN_CLK, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_LAT);
  gpio_set_direction(PIN_LAT, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(PIN_OE);
  gpio_set_direction(PIN_OE, GPIO_MODE_OUTPUT);

  gpio_set_level(PIN_OE, 1);
  gpio_set_level(PIN_LAT, 0);
  gpio_set_level(PIN_CLK, 0);

  // Create the interrupts to refresh the panels
  timer_Refresh = timerBegin(0, 80, true);  // 80 for Microseconds
  timerAttachInterrupt(timer_Refresh, &RefreshInterrupt, true);
  timerAlarmWrite(timer_Refresh, REFRESH_INTERVAL_uS, true);
 
  // Timer to control the display on-time, i.e. the brightness of the panel
  timer_OE = timerBegin(1, 80, true);  // 80 for Microseconds
  timerAttachInterrupt(timer_OE, &OutpuEnableEndInterrupt, true);
}

void ESP32_4xMBI5034::StartDisplay()
{
  // Start refresh
  timerAlarmEnable(timer_Refresh);
}

void IRAM_ATTR ESP32_4xMBI5034::RefreshInterrupt()
{
	static uint8_t bank = 0;
  uint16_t bitsToSend = _chips * 16;  // Each chip drives 16 LEDs

	byte *f = (*_frameBuffers)[0][bank]; //_panel->frame[bank];
	GPIO.out_w1ts = BIT(PIN_OE);      // disable output

  // Get the current port state, so we don't change unrelated lines
  uint32_t out = GPIO.out & ~(BIT(PIN_D1) | BIT(PIN_D2) | BIT(PIN_D3) | BIT(PIN_D4) | BIT(PIN_D5) | BIT(PIN_D6) | BIT(PIN_D7) | BIT(PIN_D8) | BIT(PIN_A0) | BIT(PIN_A1) | BIT(PIN_LAT) | BIT(PIN_CLK)) | BIT(PIN_OE); 
  
	for (uint16_t n = 0; n < bitsToSend; n++) 
	{
    // Update all 4 panels using 2 data lines/panel using mapping table
    // this version takes about 39uS for all 384 outputs, i.e. 10MHz rate
    GPIO.out = out | HardwareGpioMapping[*f++];
    GPIO.out_w1ts = BIT(PIN_CLK);  

	}
	GPIO.out_w1tc = BIT(PIN_CLK);  

	uint32_t addr = ((bank & 1) << PIN_A0) | ((bank & 2) >> 1 << PIN_A1);
	GPIO.out = out | addr; // | BIT(PIN_OE);  // Set address
	GPIO.out_w1ts = BIT(PIN_LAT);   // toggle latch
	GPIO.out_w1tc = BIT(PIN_LAT); 

  // Disable (switch off) the current row after a defined interval
  timerRestart(timer_OE);
  timerAlarmWrite(timer_OE, MAX_OUTPUT_ENABLE_INTERVAL_uS - OUTPUT_ENABLE_INT_LATENCY_uS, false);
  GPIO.out_w1tc = BIT(PIN_OE);    // enable output
  timerAlarmEnable(timer_OE);

	if (++bank >= _planes) 
  {
	  bank=0;
  }
}

void IRAM_ATTR ESP32_4xMBI5034::OutpuEnableEndInterrupt()
{
  GPIO.out_w1ts = BIT(PIN_OE);      // disable output
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
void ESP32_4xMBI5034::SetBrightness(uint16_t brighnessPercent)
{
  // Two current ranges are selectable:
  // if H=0, Current = 0.125-0.488
  // if H=1, Current = 0.508-1.938, where 0b1011=1 (100%)
  
  // Calculate the approx brightness control around 100%
  uint32_t brightness = 0;
  const uint32_t brightness100pc = 0x2b;
  if(brighnessPercent <= 100)
    brightness = (brighnessPercent - 12) * brightness100pc / 88;
  else if(brighnessPercent >= 200)
  {
    brightness = 63;
  }
  else
    brightness = (brighnessPercent - 100) * (63-brightness100pc) / 100 + 0x2b;

  for(int bank = 0; bank < 4; bank++)
  {
    uint32_t addr = ((bank & 1) << PIN_A0) | ((bank & 2) >> 1 << PIN_A1);
    
    // Need to send the brightness command to each of the chips
    GPIO.out = addr | BIT(PIN_OE);  // Set address
    for(int chip = 0; chip < 24; chip++)
    {
      //GPIO.out_w1ts = BIT(PIN_LAT); // Assert LAT, so chip recoginises as a control write
      uint16_t controlRegister = 0b0111000101000000 | brightness;
      for (uint16_t n = 0; n < 16; n++, controlRegister <<= 1) 
      {
        GPIO.out = ((controlRegister & 0x8000) >> 15 << PIN_D1) | ((controlRegister & 0x8000) >> 15 << PIN_D2) | (n < 12 || chip != 23 ? 0 : BIT(PIN_LAT)) | addr;
        GPIO.out_w1ts = BIT(PIN_CLK);  
      }
      GPIO.out_w1tc = BIT(PIN_CLK);  
    }
    GPIO.out_w1tc = BIT(PIN_LAT); // Write control
  }
}
