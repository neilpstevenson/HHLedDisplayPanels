/********************************************************************
This class encapsulates the platform driver for a panel
********************************************************************/
#include <Arduino.h>



class ESP32_4xMBI5034
{
public:
  //static void Initialise(void(*refreshCallbackFn)());
  static void Initialise(byte (&frameBuffers)[1][4][24*16], uint8_t colourDepth, uint8_t planes, uint8_t chips);
  
  // Currently this mus be called after Initialise and before StartDisplay
  static void SetBrightness(uint16_t brighnessPercent);

  // Start showing the display
  static void StartDisplay();

private:  
  static void IRAM_ATTR RefreshInterrupt();
  static void IRAM_ATTR OutpuEnableEndInterrupt();
};
