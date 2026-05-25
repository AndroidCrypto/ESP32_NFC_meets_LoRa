// Setup for the ESP32 CYD 1732S019 
#define USER_SETUP_ID 809

#define ST7789_DRIVER
#define TFT_SDA_READ   // Display has a bidirectional SDA pin

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define CGRAM_OFFSET      // Library will add offsets required

#define TFT_BL    21  // Display backlight control pin
#define TFT_BACKLIGHT_ON HIGH  // HIGH or LOW are options

// The ESP32 has 2 free SPI ports i.e. VSPI and HSPI, the VSPI is the default.
// If the VSPI port is in use and pins are not accessible (e.g. TTGO T-Beam)
// then uncomment the following line:
#define USE_HSPI_PORT

// #define TFT_MISO 12 // don't define this pin when using Touch and SD Card on the CYD
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

//#define SPI_FREQUENCY  27000000
//#define SPI_FREQUENCY  40000000
#define SPI_FREQUENCY  80000000

#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V
