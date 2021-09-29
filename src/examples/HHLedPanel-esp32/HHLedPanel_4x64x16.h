/********************************************************************
This class encapsulates the 64x16 panels
********************************************************************/
#include <Arduino.h>

template<class PLATFORMTYPE, int COLOUR_DEPTH> class HHLedPanel_4x64x16
{
private:
  byte frameBuffers[COLOUR_DEPTH][4][24*16];  // [bit][plane][chip]
  
public:
  void initialise(uint16_t maxBrightnessPercent)
  {
    // Setup the hardware
    PLATFORMTYPE::Initialise(frameBuffers[0][0], COLOUR_DEPTH, 4, 24*16);
    
    // Set the base brighness
    PLATFORMTYPE::SetBrightness(maxBrightnessPercent);
    
    // Start refrshing the screen
    PLATFORMTYPE::StartDisplay();
  }
  
	uint32_t getPanelWidth() const
	{
		return 64;
	}

	// Height per data row
	uint32_t getPanelRowHeight() const
	{
		return 8;
	}

	// Data rows supported
	uint32_t getPanelRows() const
	{
		return 2;
	}

  // Number of panels supported stacked vertically
  uint32_t getVerticalPanels() const
  {
    return 4;
  }

/*	void drawPixel(int16_t x, int16_t y, uint16_t color) 
	{
	  setpixel(x,y,color);
	}

	// Set a pixel to a specific 3 bit colour (8 colours)
	// 0b000 = black (off), 0b001 = Blue, 0b010 = Green, 0b100 = Red, 0b011 = Cyan, 0b101 = Magenta, 0b110 = Yellow, 0b111 = White, etc.
	void setpixel(byte x, byte y, byte col) 
*/  
  void drawPixel(int16_t x, int16_t y, uint16_t col) 
	{
		//static const byte bb[8] = {0x04, 0x08,  0x01, 0x02, 0x40, 0x80, 0x10, 0x20 };

	  int16_t off = (x&7) + (x & 0xf8)*6 + ((y & 4)*2);
	  byte row = y & 3;
	  byte b = (1 << ((y&0x3f) >> 3));
   
    for(uint8_t depth = 0; depth < COLOUR_DEPTH; depth++)
    {
  	  byte *p = & frameBuffers[depth][row][off];
      // Blue LED
      if ( col & (0x0010 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += 16;
  
      // Green LED
      if ( col & (0x0400 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += 16;
      
      // Red LED
      if ( col & (0x8000 >> depth) ) 
      {
        *p |= b;
      } 
      else 
      {
        *p &= ~b;
      }
      p += 16;
     }
	}

  void FillBuffer(byte b = 0)
	{
      // Quick clear to solid colour (normally black or white)
      memset(frameBuffers[0][0], b, sizeof(frameBuffers));
	}	
};
