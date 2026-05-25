// Setup813_Heltec_VMT190_170_320.h

// ST7789 using SPI on Heltec Vision Master T190

#define USER_SETUP_ID 813

#define ST7789_DRIVER
// #define INIT_SEQUENCE_3 // Using this initialisation sequence improves the display image

#define CGRAM_OFFSET
// #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
#define TFT_RGB_ORDER TFT_BGR // Colour order Blue-Green-Red

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define TFT_INVERSION_ON
//#define TFT_INVERSION_OFF
#define TFT_BACKLIGHT_ON 1

// FSPI port will !!! NOT !!! be used unless the following is defined
// define USE_HSPI_PORT for pins 10-13 and USE_FSPI_PORT for pins (34)-37
#define USE_HSPI_PORT    // HSPI
//#define USE_FSPI_PORT  // FSPI

// ESP32-S3        HSPI

#define TFT_BL 17
#define TFT_BACKLIGHT_ON HIGH

#define TFT_CS     39
#define TFT_MOSI   48 // = SDA
#define TFT_SCLK   38   
#define TFT_MISO   37

#define TFT_DC     47
//#define TFT_BL      38 // LED back-light
#define TFT_RST    40 // Set TFT_RST to -1 if display RESET is connected to ESP32 board EN

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

//#define SPI_FREQUENCY        27000000
#define SPI_FREQUENCY        40000000
//#define SPI_FREQUENCY          80000000

// #define SPI_READ_FREQUENCY      6000000  // 6 MHz is the maximum SPI read speed for the ST7789V

// #define SPI_TOUCH_FREQUENCY  2500000

// #define SUPPORT_TRANSACTIONS