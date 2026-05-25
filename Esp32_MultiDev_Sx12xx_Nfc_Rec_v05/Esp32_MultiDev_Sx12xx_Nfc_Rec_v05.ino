/*
  This sketch is a 'Proof Of Concept' program with these features:
  - connect to your local Wi-Fi router and synchronize the ESP32
    Real Time Clock (RTC) with the Network Time Protocol (NTP)
  - calculate and display the local time by a POSIX string
  - send the local time by LoRa to all other LoRa devices
  - shows an 'Idle' page for waiting of a LoRa transmission with
    an access request

*/

/*
  Version Management
24.05.2026 V05 Calculation of timestamp was wrong, now corrected
               code cleaning 
23.05.2026 V04 Display an idle page when waiting for a new tag  
23.05.2026 V03 Sending the timestamp to all LoRa devices
23.05.2026 V02 Displays the received data including the current time  
22.05.2026 V01 Initial programming, receives the data from the Transmitter
               'Esp32_Tft_ST7789_Adafruit_PN532_MifareClassic1K_LoRa_v02'  

*/

/**
* Please uncomment just one hardware definition file that reflects your hardware combination
* for Heltec Vision Master T190 boards use HELTEC_VMT190
* for Heltec WiFi LoRa 32 V2 boards use HELTEC_V2
* for Heltec WiFi LoRa 32 V3 boards use HELTEC_V3
* for LilyGo T3S3 LoRa boards use LILYGO_T3S3_SX1262
* for ESP32 Development boards with attached LoRa module SX1276 module and OLED use ESP32_SX1276_OLED
* for ESP32 Development boards with attached LoRa module SX1276 module and TFT use ESP32_SX1276_TFT
* for all other boards and hardware combination you should consider to modify an existing one to your needs
*
* Don't forget to change the Board in Arduino:
* for Heltec Vision Master T190: Heltec Vision Master T190 
* for Heltec V2: Heltec WiFi LoRa 32(V2)
* for Heltec V3: Heltec WiFi LoRa 32(V3) / Wireless shell (V3) / ...
* for LilyGo T3S3 LoRa: ESP32S3 Dev Module
* or ESP32 Development Boards: ESP32-WROOM-DA Module
*
* - or in Tools menue:
* for Heltec Vision Master T190: Tools - Board - esp32 - Heltec Vision Master T190 
* for Heltec V2: Tools - Board - esp32 - Heltec WiFi LoRa 32(V2)
* for Heltec V3: Tools - Board - esp32 - Heltec WiFi LoRa 32(V3) / Wireless shell (V3) / ...
* for LilyGo T3S3 LoRa: Tools - Board - esp32 - ESP32S3 Dev Module
* for ESP32 Development Boards: Tools - Board - esp32 - ESP32-WROOM-DA Module
*
*/

#define HELTEC_VMT190
//#define HELTEC_V2
//#define HELTEC_V3
//#define LILYGO_T3S3_SX1262
//#define ESP32_SX1276_OLED
//#define ESP32_SX1276_TFT

// ------------------------------------------------------------------
// include the hardware definition files depending on the uncommenting
#ifdef HELTEC_VMT190
#include "Heltec_Vision_Master_T190_Hardware_Settings.h"
#endif

#ifdef HELTEC_V2
#include "Heltec_V2_Hardware_Settings.h"
#endif

#ifdef HELTEC_V3
#include "Heltec_V3_Hardware_Settings.h"
#endif

#ifdef LILYGO_T3S3_SX1262
#include "LilyGo_T3S3_LoRa_SX1262_Hardware_Settings.h"
#endif

#ifdef ESP32_SX1276_OLED
#include "ESP32_SX1276_OLED_Hardware_Settings.h"
#endif

#ifdef ESP32_SX1276_TFT
#include "ESP32_SX1276_TFT_Hardware_Settings.h"
#endif

// ------------------------------------------------------------------

// when using the (default) OLED display SSD1306 128 * 64 px the maximum length is 25 chars
const char* PROGRAM_VERSION = "NFC Reader Receiv.  V05";

// ------------------------------------------------------------------
// internal or external OLED SSD1306 128 * 64 px display

#ifdef IS_OLED
#include "FONT_MONOSPACE_9.h"
// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>
#include "SSD1306.h"  // https://github.com/ThingPulse/esp8266-oled-ssd1306
SSD1306Wire display(OLED_I2C_ADDRESS, OLED_I2C_SDA_PIN, OLED_I2C_SCL_PIN);
#endif

#ifdef IS_TFT
// ------------------------------------------------------------------
// TFT display ST7735 1.8' 128 * 160 RGB
#include "FONT_MONOSPACE_9.h"
#include <SPI.h>
#include <Adafruit_GFX.h>                                        // Core graphics library, https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_ST7735.h>                                     // Hardware-specific library for ST7735, https://github.com/adafruit/Adafruit-ST7735-Library
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);  // hardware SPI
#endif

#if defined(IS_TFT_ST7789_170_320)
// ------------------------------------------------------------------
// TFT display ST7789 1.9' 170 * 320 RGB
// in Landscape give 23 characters by 7 ? lines
#include "FONT_MONOSPACE_9.h"
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define TFT_ST7789_170_320_DISPLAY_ORIENTATION 0  // 0 = portrait USB bottom, 2 = portrait USB top
//#define TFT_ST7789_170_320_DISPLAY_ORIENTATION // 1 = landscape USB left, 3 = landscape USB right
#endif

// vars for displaying line 1 to 5 to display in a loop
String display1 = "";
String display2 = "";
String display3 = "";
String display4 = "";
String display5 = "";
// for TFT only
String display6, display7, display8, display9, display10, display11, display12, display13, display14, display15, display16;

bool showDisplay = false;
bool isDisplayOn = false;  // this bool is needed for switching the display off after timer exceeds

const long DISPLAY_UPDATE_DURATION_MILLIS = 1000;  // update every second
long lastDisplayUpdateMillis = 0;

const long CLOCK_UPDATE_DURATION_MILLIS = 1000; // each second
long lastClockUpdateMillis = 0;

const long REMOTE_TIME_UPDATE_DURATION_MILLIS = 60000;  // one minute
long lastRemoteTimeUpdateMillis = 0;

bool isShowingAccessStatus = false;
const long SHOW_ACCESS_STATUS_DURATION_MILLIS = 4000;  // 4 seconds
long lastShowAccessStatusMillis = 0;

// -----------------------------------------------------------------------
// https://github.com/StuartsProjects/SX12XX-LoRa

#include <SPI.h>

#ifdef HELTEC_V2
#include <SX127XLT.h>  //include the appropriate library
SX127XLT LT;           //create a library class instance called LT
#endif

#if defined(HELTEC_V3) || defined(HELTEC_WIRELESS_STICK_V3) || defined(HELTEC_WIRELESS_STICK_LITE_V3) || defined(HELTEC_VMT190)
#include <SX126XLT.h>  //include the appropriate library
SX126XLT LT;           //create a library class instance called LT
#endif

#ifdef LILYGO_T3S3_SX1262
#include <SX126XLT.h>  //include the appropriate library
SX126XLT LT;           //create a library class instance called LT
#endif

#ifdef ESP32_SX1276_OLED
#include <SX127XLT.h>  //include the appropriate library
SX127XLT LT;           //create a library class instance called LT
#endif

#ifdef ESP32_SX1276_TFT
#include <SX127XLT.h>  //include the appropriate library
SX127XLT LT;           //create a library class instance called LT
#endif

#include <arrayRW.h>        // routines for reading and writing varaibles to an array
#include "LoRa_Settings.h"  // include the setttings file, LoRa frequencies, txpower etc
#include "Node_Settings.h"  // include the node/sketch specific settings

uint8_t RXPacketL;   // stores length of packet received
int16_t PacketRSSI;  // stores RSSI of received packet
int8_t PacketSNR;    // stores signal to noise ratio of received packet
uint8_t RXPayloadL;  // stores length of payload received

uint8_t loRaSpreadingFactor = SPREADING_FACTOR;  // default setting

// prepairing the buffer
// we define an RXBuffer with the maximum packet size of 252
const uint8_t RXBUFFER_SIZE = 252;
uint8_t RXBuffer[RXBUFFER_SIZE];

// transmission
uint8_t TXPacketL;
const uint8_t TXBUFFER_SIZE = 24;
uint8_t txPacket[TXBUFFER_SIZE];


const uint8_t loraNameLength = 20;
const uint8_t loraRoomLength = 2;
const uint8_t loraValidLength = 8;
const uint8_t loraDepositLength = 8;
uint8_t loraName[loraNameLength];
uint8_t loraRoom[loraRoomLength];
uint8_t loraValidFrom[loraValidLength];
uint8_t loraValidTo[loraValidLength];
uint8_t loraDeposit[loraDepositLength];

uint32_t rxTxPacketCounter;
bool accessStatus = false;  // true = access granted

// -----------------------------------------------------------------------
// PRG/Boot button
// #define BOOT_BUTTON_PIN 0 // see settings or hardware settings
boolean isBootButtonPressed = false;
uint8_t modeCounter = 0;  // just a counter

void IRAM_ATTR bootButtonPressed() {
  modeCounter++;
  isBootButtonPressed = true;
  // deactivate the interrupt to avoid bouncing
  detachInterrupt(BOOT_BUTTON_PIN);
}

// -----------------------------------------------------------------------
// User button - this is available on Heltec Vision Master T190
#ifdef IS_TFT_ST7789_170_320
#define USER_BUTTON_PIN 21  // see settings or hardware settings
#include <HotButton.h>      // https://github.com/ropg/HotButton
HotButton userButton(USER_BUTTON_PIN);
bool userButtonState = true;  // display on
#endif

// LilyGo T3S3 support for battery mode
#include "LilyGoLoRaBoard.h"

// -----------------------------------------------------------------------
// Time Management
#include "TIME_MANAGEMENT.h"

// -----------------------------------------------------------------------
// Wi-Fi Management
#include "WIFI_MANAGEMENT.h"

// -----------------------------------------------------------------------

void updateClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString(timeStr, 85, 10, 4);
}

uint32_t getLocalTimestampForLoRa() {
  time_t now;
  if (!time(&now)) return 0;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return 0;

  // Debug
  Serial.printf("Local Time and Date is %02d:%02d:%02d %02d.%02d.%04d\n", 
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
                timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

  struct tm tm_local;
  localtime_r(&now, &tm_local);
  
  struct tm tm_utc;
  gmtime_r(&now, &tm_utc);

  long offsetSeconds = (tm_local.tm_hour - tm_utc.tm_hour) * 3600L + 
                       (tm_local.tm_min - tm_utc.tm_min) * 60L;
  
  if (tm_local.tm_mday > tm_utc.tm_mday || tm_local.tm_mon > tm_utc.tm_mon) {
      offsetSeconds += 86400L;
  } else if (tm_local.tm_mday < tm_utc.tm_mday || tm_local.tm_mon < tm_utc.tm_mon) {
      offsetSeconds -= 86400L;
  }

  return (uint32_t)(now + offsetSeconds);
}

void drawStaticUI() {

  String name = uint8ToString(loraName, 20);
  String room = uint8ToString(loraRoom, 2);
  String validFrom = uint8ToString(loraValidFrom, 8);
  String validTo = uint8ToString(loraValidTo, 8);
  String deposit = uint8ToString(loraDeposit, 8);

  int xPos = 10;  // Jetzt hier definiert
  tft.drawFastHLine(0, 40, 170, TFT_DARKGREY);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  // Name
  tft.setCursor(xPos, 50);
  tft.println("NAME:");
  tft.println();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String restName = name;
  restName.trim();
  int wordCount = 1;
  for (int i = 0; i < restName.length(); i++)
    if (restName[i] == ' ') wordCount++;

  tft.setTextSize(wordCount > 4 ? 1 : 2);
  while (restName.length() > 0) {
    int spaceIndex = restName.indexOf(' ');
    tft.setCursor(10, tft.getCursorY());
    if (spaceIndex == -1) {
      tft.println(restName);
      break;
    } else {
      tft.println(restName.substring(0, spaceIndex));
      restName = restName.substring(spaceIndex + 1);
      restName.trim();
    }
  }

  int currentY = tft.getCursorY() + 15;
  tft.setTextSize(1);
  tft.setCursor(10, currentY);
  tft.setTextColor(TFT_CYAN);
  tft.print("ROOM: ");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.println(room);

  currentY = tft.getCursorY() + 15;
  tft.setTextSize(1);
  tft.setCursor(10, currentY);
  tft.setTextColor(TFT_CYAN);
  tft.println("VALIDITY:");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, tft.getCursorY() + 5);
  tft.print("F: ");
  tft.println(validFrom);
  tft.setCursor(10, tft.getCursorY() + 5);
  tft.print("T: ");
  tft.println(validTo);

  int footerY = 260;
  tft.drawFastHLine(0, footerY - 5, 170, TFT_WHITE);
  tft.setCursor(10, footerY);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.println("DEPOSIT:");
  tft.setCursor(10, footerY + 15);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.print(deposit);
  tft.print(" EUR");
}

// transmit the local time to all remote LoRa devices
void sendLoRaTimeUpdate() {
  uint32_t localTimestamp = getLocalTimestampForLoRa();
  if (localTimestamp < 1769526983) {
    Serial.printf("The localTimestamp %d is not set up, request regarded\n", localTimestamp);
    return;
  } else {
    Serial.printf("Send the localTimestamp %d by LoRa\n", localTimestamp);
  }
  uint8_t dataArraySize = 0;
  beginarrayRW(txPacket, 0);
  arrayWriteUint8(0x59);  // 1 byte static for identifying 'Y'
  arrayWriteUint32(localTimestamp);
  arrayWriteUint8(0x52);             // 1 byte static for identifying 'R'
  dataArraySize = endarrayRW() + 1;  // this returns the number of array bytes written, is 6 bytes
  Serial.printf("TX (%d): ", dataArraySize);
  if (dataArraySize < 100) {
    LT.printHEXPacket(txPacket, dataArraySize);
  }
  Serial.println();
  Serial.flush();
  // fill the LoRa transmission buffer
  LT.startWriteSXBuffer(0);  // initialise buffer write at address 0
  //for (int i = 0; i < dataArraySize; i++) {
  for (int i = 0; i < 6; i++) { // a fixed value
    LT.writeUint8(txPacket[i]);
  }
  uint8_t txBufferSize = LT.endWriteSXBuffer();  // close buffer write
  TXPacketL = LT.transmitSXBuffer(0, txBufferSize, 10000, TX_POWER, WAIT_TX);

  if (TXPacketL == 0) {
    Serial.println(F("Error when transmitting the data"));
    Serial.flush();
  }
}

void sendLoraAccessStatus(uint32_t txCounter, bool accessGranted) {
  uint8_t dataArraySize = 0;
  beginarrayRW(txPacket, 0);
  arrayWriteUint8(0x43);  // 1 byte static for identifying
  arrayWriteUint32(txCounter);
  if (accessGranted) {
    arrayWriteUint8(0x01);
  } else {
    arrayWriteUint8(0x00);
  }
  arrayWriteUint8(0x52);             // 1 byte static for identifying 'R'
  dataArraySize = endarrayRW() + 1;  // this returns the number of array bytes written, is 3 bytes
  Serial.printf("TX (%d): ", dataArraySize);
  if (dataArraySize < 60) {
    LT.printHEXPacket(txPacket, dataArraySize);
  }
  Serial.println();
  Serial.flush();
  // fill the LoRa transmission buffer
  LT.startWriteSXBuffer(0);  // initialise buffer write at address 0
  for (int i = 0; i < dataArraySize; i++) {
    LT.writeUint8(txPacket[i]);
  }
  uint8_t txBufferSize = LT.endWriteSXBuffer();  // close buffer write
  TXPacketL = LT.transmitSXBuffer(0, txBufferSize, 10000, TX_POWER, WAIT_TX);

  if (TXPacketL == 0) {
    Serial.println(F("Error when transmitting the data"));
    Serial.flush();
  }
}

// this is a very basic check for access
bool checkAccess() {
  String room = uint8ToString(loraRoom, 2);
  Serial.printf("Requesting access to room %s: ", room);
  if (room == "12") {
    // allowed
    Serial.println("granted");
    return true;
  } else {
    // denied
    Serial.println("denied");
    return false;
  }
}

void showIdlePage() {
  tft.fillScreen(TFT_BLACK);

  int centerX = 170 / 2;
  int centerY = 80;

  tft.drawCircle(centerX, centerY, 40, TFT_BLUE);
  tft.drawCircle(centerX, centerY, 30, TFT_SKYBLUE);
  tft.fillCircle(centerX, centerY, 15, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  
  int y = 145; 
  tft.setTextSize(1);
  tft.drawString("Waiting for", centerX, y, 4); 
  y += 40;
  tft.setTextColor(TFT_GOLD);
  tft.setTextSize(2); // Skalierung 2
  tft.drawString("NFC", centerX, y, 2); 
  y += 40;
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Tag", centerX, y, 2);
  y += 45;
  tft.setTextColor(TFT_MAGENTA);
  tft.setTextSize(2);
  tft.drawString("& LoRa", centerX, y, 2);
  y += 40;
  tft.setTextColor(TFT_LIGHTGREY);
  tft.setTextSize(1);
  tft.drawString("Transmission", centerX, y, 4);
}

void loop() {

  // User Button is the second, upper button of the Heltec Vision Master T190 device (see when USB connector is at the right side)
#ifdef IS_TFT_ST7789_170_320
  // It is GPIO 21.
  // The PRG Button
  userButton.update();
  if (userButton.isSingleClick()) {
    // toggle user button state
    userButtonState = !userButtonState;
    if (userButtonState) {
      setTftVControl(true);
      digitalWrite(TFT_BL, HIGH);  // full brightness
      Serial.println("userButtonState is TRUE, display should be ON");
    } else {
      setTftVControl(false);
      digitalWrite(TFT_BL, LOW);  // full brightness
      Serial.println("userButtonState is FALSE, display should be OFF");
    }
  }
#endif

  // the BOOT button was pressed
  if (isBootButtonPressed) {
    isBootButtonPressed = false;

    // this is left in case you want to add something on Boot button pressing...

    // activate the interrupt again
    attachInterrupt(BOOT_BUTTON_PIN, bootButtonPressed, RISING);
  }

  LT.receive((uint8_t*)&RXBuffer, RXBUFFER_SIZE, 0, NO_WAIT);  // this is a non blocking call

  while (!readDioRx()) {
    // all UI updates are placed here
#ifdef IS_TFT_ST7789_170_320
    userButton.update();
    if (userButton.isSingleClick()) {
      // toggle user button state
      userButtonState = !userButtonState;
      if (userButtonState) {
        setTftVControl(true);
        digitalWrite(TFT_BL, HIGH);  // full brightness
        Serial.println("userButtonState is TRUE, display should be ON");
      } else {
        setTftVControl(false);
        digitalWrite(TFT_BL, LOW);  // full brightness
        Serial.println("userButtonState is FALSE, display should be OFF");
      }
    }
#endif

    // during this idle time the other elements in loop are called here
    // the BOOT button was pressed
    if (isBootButtonPressed) {
      isBootButtonPressed = false;
      // this is left in case you want to add something on Boot button pressing...
      // activate the interrupt again
      attachInterrupt(BOOT_BUTTON_PIN, bootButtonPressed, RISING);
      // reactivate the call
      LT.receive((uint8_t*)&RXBuffer, RXBUFFER_SIZE, 0, NO_WAIT);  // this is a non blocking call
    }

    // display update
    if (millis() - lastDisplayUpdateMillis > DISPLAY_UPDATE_DURATION_MILLIS) {
      //updateDisplay();
      lastDisplayUpdateMillis = millis();
    }

    // clock update
    if (millis() - lastClockUpdateMillis > CLOCK_UPDATE_DURATION_MILLIS) {
      updateClock();
      lastClockUpdateMillis = millis();
    }

    // remote LoRa devices time update
    if (millis() - lastRemoteTimeUpdateMillis > REMOTE_TIME_UPDATE_DURATION_MILLIS) {
      sendLoRaTimeUpdate();
      lastRemoteTimeUpdateMillis = millis();
    }

    if (isShowingAccessStatus) {
      if (millis() - lastShowAccessStatusMillis > SHOW_ACCESS_STATUS_DURATION_MILLIS) {
        isShowingAccessStatus = false;
        showIdlePage();
      }
    }
  }

  // at this point we received something
  ledFlash(1, 125);
  printSeconds();

  RXPacketL = LT.readRXPacketL();
  PacketRSSI = LT.readPacketRSSI();
  PacketSNR = LT.readPacketSNR();

  Serial.print(RXPacketL);
  Serial.print(F(" bytes > "));
  /*
  if (RXPacketL < 60) {
    LT.printSXBufferHEX(0, (RXPacketL - 1));
  }
  */
  Serial.println();

  if (RXPacketL == 0) {
    packetIsError();
  } else {
    packetIsOK();
  }

  ledFlash(2, 125);
  Serial.println();
}

/**
* If DIO0 (SX1276) or DIO1 (SX1262) are high a signal was received
**/
boolean readDioRx() {
  // this reads DIO0 (SX1276) or DIO1 (SX1262), depending on device
#if defined(HELTEC_VMT190)
  return digitalRead(SX_DIO1);
#endif
#ifdef HELTEC_V2
  return digitalRead(SX_DIO0);
#endif
#ifdef HELTEC_V3
  return digitalRead(SX_DIO1);
#endif
#ifdef HELTEC_WIRELESS_STICK_V3
  return digitalRead(SX_DIO1);
#endif
#ifdef HELTEC_WIRELESS_STICK_LITE_V3
  return digitalRead(SX_DIO1);
#endif
#ifdef LILYGO_T3S3_SX1262
  return digitalRead(SX_DIO1);
#endif
#ifdef ESP32_SX1276_OLED
  return digitalRead(SX_DIO0);
#endif
#ifdef ESP32_SX1276_TFT
  return digitalRead(SX_DIO0);
#endif
}

String uint8ToString(uint8_t* data, int len) {
  char temp[len + 1];       // Puffer erstellen (+1 für Null-Byte)
  memcpy(temp, data, len);  // Daten kopieren
  temp[len] = '\0';         // Null-Terminierung manuell setzen
  String s = String(temp);  // In String umwandeln
  s.trim();                 // Leerzeichen abschneiden
  return s;
}

void packetIsOK() {
  printReceptionDetails();
  Serial.println();

  // read the packet in the buffer
  // there seems to be a difference in the SX127x and SX126x classes
#ifdef HELTEC_V2
  LT.startReadSXBuffer(0);  // start buffer read at location 0
  for (int i = 0; i < RXPacketL; i++) {
    RXBuffer[i] = LT.readUint8();
  }
  //LoRa.readBuffer(*buff, LoRa.readRXPacketL());
  uint8_t RXPayloadL_Buffer = LT.endReadSXBuffer();  // this function returns the length of the array read
#else
  LT.readPacket(RXBuffer, RXPacketL);
#endif

  // Debug printing
  Serial.printf("Received a packet with %d bytes\n", RXPacketL);
  if (RXPacketL > 0) {
    Serial.printf("RCVD (%d):", RXPacketL);
    LT.printHEXPacket(RXBuffer, RXPacketL);  // print the received array as HEX
    Serial.println();
  }
  Serial.flush();

  // type: regular NFC tag reading, length 52
  if (RXPacketL == 52) {
    // check for magic byte 0x4E
    // get the data from RXBuffer
    uint8_t len;
    beginarrayRW(RXBuffer, 0);              // start reading from array at location 0
    uint8_t rxHeader = arrayReadUint8();    // 1 byte // the magic start byte
    rxTxPacketCounter = arrayReadUint32();  // 4 bytes
    // 20 bytes name
    for (uint8_t i = 0; i < loraNameLength; i++) {
      loraName[i] = arrayReadUint8();
    }
    // 2 bytes room
    for (uint8_t i = 0; i < loraRoomLength; i++) {
      loraRoom[i] = arrayReadUint8();
    }
    // 8 bytes validFrom
    for (uint8_t i = 0; i < loraValidLength; i++) {
      loraValidFrom[i] = arrayReadUint8();
    }
    // 8 bytes validTo
    for (uint8_t i = 0; i < loraValidLength; i++) {
      loraValidTo[i] = arrayReadUint8();
    }
    // 8 bytes deposit
    for (uint8_t i = 0; i < loraDepositLength; i++) {
      loraDeposit[i] = arrayReadUint8();
    }
    uint8_t rxFooter = arrayReadUint8();  // 1 byte the magic end byte
    // check the header and footer for our data
    if (rxHeader != 0x4E) {
      Serial.printf("The received header %0X does not match 0x4E, rejected", rxHeader);
      return;
    }
    if (rxFooter != 0x52) {
      Serial.printf("The received footer %0X does not match 0x52, rejected", rxFooter);
      return;
    }

    Serial.println("This packet is VALID for NFC Reader data");

    tft.fillScreen(TFT_BLACK);
    updateClock();
    Serial.printf("Data received on local timestamp %d\n", getLocalTimestampForLoRa());
    drawStaticUI();
    bool accessStatus = checkAccess();
    if (accessStatus) {
      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(2);
      tft.setCursor(40, 300);
      tft.println("GRANTED");
    } else {
      tft.setTextColor(TFT_RED);
      tft.setTextSize(2);
      tft.setCursor(40, 300);
      tft.println("DENIED");
    }
    sendLoraAccessStatus(rxTxPacketCounter, accessStatus);
    lastShowAccessStatusMillis = millis();
    isShowingAccessStatus = true;
  }  // if (RXPacketL == 52)

  return;
}

void append(char* s, char* buffer) {
  byte i;
  int bufferLen = strlen(buffer);
  int len = strlen(s);
  for (i = 0; i < bufferLen; i++) {
    s[len + i] = buffer[i];
  }
}

void packetIsError() {
  uint16_t IRQStatus;
  IRQStatus = LT.readIrqStatus();  //get the IRQ status

  if (IRQStatus & IRQ_RX_TIMEOUT) {
    Serial.print(F(" RXTimeout"));
  } else {
    Serial.print(F(" PacketError"));
    printReceptionDetails();
    Serial.print(F("  Length,"));
    Serial.print(LT.readRXPacketL());  //get the real packet length
    Serial.print(F(",IRQreg,"));
    Serial.print(IRQStatus, HEX);
  }
}

void printReceptionDetails() {
  Serial.print(F(" RSSI, "));
  Serial.print(PacketRSSI);
  Serial.print(F("dBm,SNR, "));
  Serial.print(PacketSNR);
  Serial.print(F(" dB  "));
#if defined(VERBOSE_PACKET_INFO)
  display1 = PROGRAM_VERSION;
  display2 = "RXPacketL: " + (String)RXPacketL;
  //display3 = "** ERROR **";
  display4 = "RSSI: " + (String)PacketRSSI + " dBm";
  display5 = "SNR : " + (String)PacketSNR + " dB";
  displayData();
#endif
}

void ledFlash(uint16_t flashes, uint16_t delaymS) {
  // run only if a LED is connected
  if (LED_PIN >= 0) {
    uint16_t index;
    for (index = 1; index <= flashes; index++) {
      digitalWrite(LED_PIN, HIGH);
      delay(delaymS);
      digitalWrite(LED_PIN, LOW);
      delay(delaymS);
    }
  }
}

void printSeconds() {
  float secs;
  secs = ((float)millis() / 1000);
  Serial.print(secs, 3);
}

void setup() {
#ifdef LILYGO_T3S3_SX1262
  setupLilyGoBoard();
#else
  Serial.begin(115200);
  delay(500);
#endif

  Serial.println(PROGRAM_VERSION);

  // if we have a power control for devices put it on
#ifdef IS_VEXT_CONTROL
  setVextControl(true);
#endif

  // if we have a power control for the display put it on
#ifdef IS_TFT_V_CONTROL
  setTftVControl(true);
#endif

  // set display brightness
#if defined(IS_TFT_ST7789_170_320)
  pinMode(TFT_BL, OUTPUT);
  //digitalWrite(TFT_BL, HIGH);  // full brightness
  analogWrite(TFT_BL, 100);
#endif

  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);  // setup pin as output for indicator LED
    ledFlash(1, 125);          // two quick LED flashes to indicate program start
  }

  delay(1000);

  // this is necessary as the LilyGo T3S3 board requires a different setup
#ifdef LILYGO_T3S3_SX1262
  SPI.begin(SX_SCK, SX_MISO, SX_MOSI);
#else
  SPI.begin();
#endif

  // setup display
#ifdef IS_OLED
  if (OLED_I2C_RST_PIN >= 0) {
    pinMode(OLED_I2C_RST_PIN, OUTPUT);
    digitalWrite(OLED_I2C_RST_PIN, LOW);  // set GPIO16 low to reset OLED
    delay(50);
    digitalWrite(OLED_I2C_RST_PIN, HIGH);
    delay(50);
  }
  clearDisplayData();
  display.init();
#ifdef DISPLAY_ORIENTATION_FLIPPED
  // do nothing
#else
  display.flipScreenVertically();  // Landscape 90 degrees right rotated
#endif
  display.setFont(ArialMT_Plain_10);
  delay(50);
  display1 = PROGRAM_VERSION;
  displayData();
  delay(500);
#endif

  // init TFT display
#ifdef IS_TFT
  tft.initR(INITR_BLACKTAB);     // den ST7735S Chip initialisieren, schwarz
  tft.fillScreen(ST77XX_BLACK);  // und den Schirm mit Schwarz füllen
  tft.setTextWrap(false);        // automatischen Zeilenumbruch ausschalten
#ifdef DISPLAY_ORIENTATION_FLIPPED
  tft.setRotation(1);  // Landscape 270 degrees right rotated
#else
  tft.setRotation(3);              // Landscape 90 degrees right rotated
#endif
  Serial.println(F("Display init done"));
  display1 = PROGRAM_VERSION;
  displayData();
  delay(500);
#endif

#if defined(IS_TFT_ST7789_170_320)
  Serial.println("Init TFT ST7789 170x320 px");
  tft.init();
  //spi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  //tft.init(170, 320);            // Init ST7789 320x240
  tft.fillScreen(TFT_BLACK);  // fill screen with black = empty
  tft.setTextWrap(false);     // no automated line wrapping

  tft.setRotation(TFT_ST7789_170_320_DISPLAY_ORIENTATION);
  /*
#ifdef DISPLAY_ORIENTATION_FLIPPED
  tft.setRotation(1);  // Landscape 0 degrees right rotated
#else
  tft.setRotation(3);              // Landscape 180 degrees right rotated
#endif
*/
  Serial.println(F("Display ST7789 170x320 px init done"));
  display1 = PROGRAM_VERSION;
  displayData();
  delay(500);
#endif

  display2 = "Display init done";
  displayData();
  delay(1000);

#ifdef HELTEC_V2
  if (LT.begin(SX_NSS, SX_NRESET, SX_DIO0, LORA_DEVICE)) {
    Serial.println(F("LoRa Device Heltec V2 found"));
    display3 = "LoRa Device HV2 found";
    displayData();
    ledFlash(2, 125);
  } else {
    Serial.println(F("Device error"));
    while (1) {
      Serial.println(F("No device responding"));
      display3 = "No LoRa Device found";
      displayData();
      ledFlash(50, 50);  // long fast speed flash indicates LoRa device error
    }
  }
#endif

#if defined(HELTEC_V3) || defined(HELTEC_WIRELESS_STICK_V3) || defined(HELTEC_WIRELESS_STICK_LITE_V3) || defined(HELTEC_VMT190)
  if (LT.begin(SX_NSS, SX_NRESET, SX_RFBUSY, SX_DIO1, LORA_DEVICE)) {
    Serial.println(F("LoRa Device Heltec V3 found"));
    display3 = "LoRa Device HV3 found";
    displayData();
    ledFlash(1, 125);
  } else {
    Serial.println(F("Device error"));
    while (1) {
      Serial.println(F("No device responding"));
      display3 = "No LoRa Device found";
      displayData();
      ledFlash(50, 50);  // long fast speed flash indicates LoRa device error
    }
  }
  // The Heltec V3 board uses an unusual crystal voltage. Somme errors came up
  // when using Reliable communication so I'm setting the value here.
  LT.setDIO3AsTCXOCtrl(TCXO_CTRL_1_8V);
#endif

#ifdef LILYGO_T3S3_SX1262
  if (LT.begin(SX_NSS, SX_NRESET, SX_RFBUSY, SX_DIO1, LORA_DEVICE)) {
    Serial.println(F("LoRa Device LilyGo T3S3 SX1262 found"));
    display3 = "LoRa Dev LilyGoT3S3 found";
    displayData();
    ledFlash(1, 125);
  } else {
    Serial.println(F("Device error"));
    Serial.println(F("LoRa Device LilyGo T3S3 SX1262"));
    while (1) {
      Serial.println(F("No device responding"));
      display3 = "No LoRa Device found";
      displayData();
      ledFlash(50, 50);  // long fast speed flash indicates LoRa device error
    }
  }
  // The LilyGo T3S3 board uses an unusual crystal voltage. Somme errors came up
  // when using Reliable communication so I'm setting the value here.
  LT.setDIO2AsRfSwitchCtrl();
  LT.setDIO3AsTCXOCtrl(TCXO_CTRL_1_8V);
#endif

#ifdef ESP32_SX1276_OLED
  if (LT.begin(SX_NSS, SX_NRESET, SX_DIO0, LORA_DEVICE)) {
    Serial.println(F("LoRa Device ESP32/SX1276 found"));
    display3 = "LoRa Dev.ESP32+SX1276";
    displayData();
    ledFlash(2, 125);
  } else {
    Serial.println(F("Device error"));
    while (1) {
      Serial.println(F("No device responding"));
      display3 = "No LoRa Device found";
      displayData();
      ledFlash(50, 50);  // long fast speed flash indicates LoRa device error
    }
  }
#endif

#ifdef ESP32_SX1276_TFT
  if (LT.begin(SX_NSS, SX_NRESET, SX_DIO0, LORA_DEVICE)) {
    Serial.println(F("LoRa Device ESP32/SX1276 found"));
    display3 = "LoRa Dev.ESP32+SX1276";
    displayData();
    ledFlash(2, 125);
  } else {
    Serial.println(F("Device error"));
    while (1) {
      Serial.println(F("No device responding"));
      display3 = "No LoRa Device found";
      displayData();
      ledFlash(50, 50);  // long fast speed flash indicates LoRa device error
    }
  }
#endif

  // just to be for sure - use the default syncword
  LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);  // this is the default value
  // set the high sensitive mode
  // Sets LoRa device for the highest sensitivity at expense of slightly higher LNA current.
  // The alternative is setLowPowerReceive() for lower sensitivity with slightly reduced current.
  LT.setHighSensitivity();
  // start the device with default parameters
  LT.setupLoRa(FREQUENCY, OFFSET, loRaSpreadingFactor, BANDWIDTH, CODE_RATE, OPTIMISATION);
  Serial.println(F("LoRa setup is complete"));

  // debug information
  Serial.println();
  LT.printModemSettings();
  Serial.println();
  LT.printOperatingSettings();
  Serial.println();

  display3 = "LoRa init done";
  display4 = "";
  display5 = "Please wait ...";
  displayData();
  delay(2000);

  display2 = "Frequency: " + (String)(FREQUENCY / 1000) + " Khz";
  display3 = "Spreading Factor: " + (String)SPREADING_FACTOR;
  // this is left in case you want to add something on Boot button pressing...
  //display4 = "";
  //display5 = "";
  displayData();
  delay(1000);

  // init the mode select button
  pinMode(BOOT_BUTTON_PIN, INPUT);
  attachInterrupt(BOOT_BUTTON_PIN, bootButtonPressed, RISING);

  // start WiFi
  WiFiInit();  // calls WiFiDisconnect();
  WiFiSetEvents();
  WiFiStationConnect();

  // enable NTP time synchronization
  configNtpTime();
  activateNtpTimeUpdateNotification();

  showIdlePage();
}

void displayData() {
#if defined(IS_TFT) || defined(IS_TFT_ST7789) || defined(IS_TFT_ST7789_170_320)
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  //tft.setFont(NULL);  // Pass NULL to revert to 'classic' fixed-space bitmap font.
  tft.setCursor(0, 0);
  tft.print(display1);
  tft.setCursor(0, 10);
  tft.print(display2);
  tft.setCursor(0, 20);
  tft.print(display3);
  tft.setCursor(0, 30);
  tft.print(display4);
  tft.setCursor(0, 40);
  tft.print(display5);
  tft.setCursor(0, 50);
  tft.print(display6);
  tft.setCursor(0, 60);
  tft.print(display7);
  tft.setCursor(0, 70);
  tft.print(display8);
  tft.setCursor(0, 80);
  tft.print(display9);
  tft.setCursor(0, 90);
  tft.print(display10);
  tft.setCursor(0, 100);
  tft.print(display11);
  tft.setCursor(0, 110);
  tft.print(display12);
  tft.setCursor(0, 120);
  tft.print(display13);
#endif

#if defined(IS_TFT_ST7789_170_320)
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  //tft.setFont(&FreeMono12pt7b);
  const uint8_t distance = 20;
  const uint8_t offset = 16;
  tft.setCursor(0, offset);
  tft.print(display1);
  tft.setCursor(0, offset + 1 * distance);
  tft.print(display2);
  tft.setCursor(0, offset + 2 * distance);
  tft.print(display3);
  tft.setCursor(0, offset + 3 * distance);
  tft.print(display4);
  tft.setCursor(0, offset + 4 * distance);
  tft.print(display5);
  tft.setCursor(0, offset + 5 * distance);
  tft.print(display6);
  tft.setCursor(0, offset + 6 * distance);
  tft.print(display7);
  tft.setCursor(0, offset + 7 * distance);
  tft.print(display8);
  tft.setCursor(0, offset + 8 * distance);
  tft.print(display9);
  tft.setCursor(0, offset + 9 * distance);
  tft.print(display10);
  tft.setCursor(0, offset + 10 * distance);
  tft.print(display11);
  tft.setCursor(0, offset + 11 * distance);
  tft.print(display12);
  tft.setCursor(0, offset + 12 * distance);
  tft.print(display13);
  tft.setCursor(0, offset + 13 * distance);
  tft.print(display14);
  tft.setCursor(0, offset + 14 * distance);
  tft.print(display15);
  tft.setCursor(0, offset + 15 * distance);
  tft.print(display16);
#endif

#ifdef IS_OLED
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Monospaced_plain_9);
  display.drawString(0, 0, display1);
  display.drawString(0, 12, display2);
  display.drawString(0, 24, display3);
  display.drawString(0, 36, display4);
  display.drawString(0, 48, display5);
  display.display();
#endif
}

void clearDisplayData() {
  display1 = "";
  display2 = "";
  display3 = "";
  display4 = "";
  display5 = "";
  display6 = "";
  display7 = "";
  display8 = "";
  display9 = "";
  display10 = "";
  display11 = "";
  display12 = "";
  display13 = "";
}

void setVextControl(boolean trueIsOn) {
#ifdef IS_VEXT_CONTROL
  if (trueIsOn) {
    pinMode(VEXT_POWER_CONTROL_PIN, OUTPUT);
    digitalWrite(VEXT_POWER_CONTROL_PIN, LOW);
    Serial.println("setVextControl Activate power by setting to LOW");
  } else {
    // pulled up, no need to drive it
    pinMode(VEXT_POWER_CONTROL_PIN, INPUT);
    Serial.println("setVextControl Deactivate power by setting to HIGH");
  }
#endif
}

void setTftVControl(boolean trueIsOn) {
#ifdef IS_TFT_V_CONTROL
  if (trueIsOn) {
    pinMode(TFT_V_CONTROL_PIN, OUTPUT);
    digitalWrite(TFT_V_CONTROL_PIN, LOW);
    Serial.println("setTftVControl Activate power by setting to LOW");
  } else {
    // pulled up, no need to drive it
    pinMode(TFT_V_CONTROL_PIN, INPUT);
    Serial.println("setTftVControl Deactivate power by setting to HIGH");
  }
#endif
}
