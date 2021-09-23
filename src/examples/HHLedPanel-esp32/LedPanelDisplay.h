template<class PANELTYPE> class LedPanelDisplay : public Adafruit_GFX
{
  public:
    PANELTYPE _panel;
    
    LedPanelDisplay() : Adafruit_GFX(64,64)
    {
    }
    
    void initialise()
    {
      // Setup the hardware
      _panel.initialise(12);
    }
    
    void drawPixel(int16_t x, int16_t y, uint16_t color) 
    {
      _panel.drawPixel(x,y,color);
    }

  //  uint16_t newColor(uint8_t red, uint8_t green, uint8_t blue);
  //  uint16_t getColor() { return textcolor; }
  
    void drawBitmapMem(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
    {
      int16_t i, j, byteWidth = (w + 7) / 8;
    
      for(j=0; j<h; j++) {
        for(i=0; i<w; i++ ) {
          if(bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
            _panel.drawPixel(x+i, y+j, color);
          }
        }
      }
    }
    
    void FillBuffer(byte b)
    {
      _panel.FillBuffer(b);
    }
};
