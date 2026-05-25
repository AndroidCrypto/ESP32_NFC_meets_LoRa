/**
* This are the hardware definitions for a
* Heltec Vision Master T190 Development board
* The board has onboard a LoRa module 'HT-RA02' = 'SX1262', 1.9-inch TFT display with ST7789 chip and an LED
* Additionally the board has a battery connector with included voltage metering and the
* possibility to shut down external devices by setting the 3.3 power to off.
* The microcontroller is an ESP-S3 module with 16 MB Flash and 8 MB PSRAM.
*/

// LoRa module
#define SX_NSS 8      // select pin on LoRa device
#define SX_SCK 9       // SCK on SPI3
#define SX_MISO 11     // MISO on SPI3 
#define SX_MOSI 10     // MOSI on SPI3 
#define SX_NRESET 12   // reset pin on LoRa device
#define SX_RFBUSY 13   // busy line, not available
#define SX_DIO1 14     // DIO1 pin on LoRa device, used for RX and TX done 
#define LORA_DEVICE DEVICE_SX1262 // we need to define the device we are using

#define IS_TFT_ST7789_170_320 // uncomment this if not included
#define TFT_CS   39
#define TFT_RST  40
#define TFT_DC   47
#define TFT_MOSI 48 // SDA Data out
#define TFT_MISO -1 // SDI Data in, not connected
#define TFT_SCLK 38 // Clock out
// #define IS_TFT_BL
#define TFT_BL   17 // Backlight

// Control TFT display power
#define IS_TFT_V_CONTROL // uncomment this if not available
#define TFT_V_CONTROL_PIN 7

// LED, if no LED is attached or unwanted set this to -1
#define LED_PIN   0  // on board LED, high for on

// Buzzer
#define BUZZER_PIN -1   // pin for buzzer, set to -1 if not used 

// BOOT or USR or PRG button pin or any other button attached to the device
#define BOOT_BUTTON_PIN 0

// Control external power
//#define IS_VEXT_CONTROL // uncomment this if not available
#define VEXT_POWER_CONTROL_PIN 36 // pin controls power to external devices
#define BATTERY_VOLTAGE_ADC_MEASURE_CONTROL_PIN 46 // pin controls the measuring of the battery voltage
#define BATTERY_VOLTAGE_ADC_PIN 6 // adc pin to measure the battery voltage

// BME280 temperature, humidity and barometric air pressure sensor
//#define IS_BME280
// a note on the PIN definitions: usually you will use the same I2C pins as used for the display
// but as the I2C pins for the regular bus are not available we use alternative pin ones
//#define IS_SECOND_I2C_BUS
#define BME280_I2C_SDA_PIN 41
#define BME280_I2C_SCL_PIN 42
#define BME280_I2C_ADDRESS 0x76

// NEO-6M GPS module
//#define IS_NEO_6M
#define NEO6M_GPS_RX_PIN 46 // The GPS board has an RX terminal - connect it with GPIO 46
#define NEO6M_GPS_TX_PIN 45  // The GPS board has a TX terminal - connect it with GPIO 45
