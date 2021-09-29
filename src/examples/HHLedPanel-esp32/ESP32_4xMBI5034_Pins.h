/********************************************************************
This class encapsulates the platform driver for a panel
********************************************************************/
// These are for the ESP32-DevKitC board with WROOM chip
// Only GPIO pins 0-31 are usable as the PIN_Dx or PIN_Ax pins
#define PIN_D1    GPIO_NUM_18//19  // D1 on Panel 1 (Bottom)
#define PIN_D2    GPIO_NUM_5//18  // D2 on Panel 1
#define PIN_D3    GPIO_NUM_17//5   // D1 on Panel 2
#define PIN_D4    GPIO_NUM_16//17  // D2 on Panel 2
#define PIN_D5    GPIO_NUM_4//16  // D1 on Panel 3
#define PIN_D6    GPIO_NUM_0//4   // D2 on Panel 3
#define PIN_D7    GPIO_NUM_2//0   // D1 on Panel 4
#define PIN_D8    GPIO_NUM_15//2   // D2 on Panel 4 (Top)

// PIN_Ax can also share the PIN_Dx data line pins to save outputs required
#define PIN_A0    GPIO_NUM_5//19 //25  // A0 on all Panels
#define PIN_A1    GPIO_NUM_18//18 //26  // A1 on all Panels

#define PIN_CLK   GPIO_NUM_22//15  // CLK on all Panels
#define PIN_LAT   GPIO_NUM_21//14  // LAT on all Panels
#define PIN_OE    GPIO_NUM_19//22//12  // OE on all Panels


// These define the refresh rate for the platform-specific settings
static const uint64_t REFRESH_INTERVAL_uS = 1500;            // microseconds
static const uint64_t MAX_OUTPUT_ENABLE_INTERVAL_uS = 256;   // microseconds, must be a multiple of the bit depth and at least 50uS less than REFRESH_INTERVAL_uS (for an ESP32 @ 240MHz)
static const uint64_t OUTPUT_ENABLE_INT_LATENCY_uS = 2;      // microseconds, interrupt latency correction

static const uint16_t MAX_BRIGHTNESS_PERCENT = 12; // 12-200% allowed
