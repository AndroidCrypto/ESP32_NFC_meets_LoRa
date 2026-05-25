/*
  This sketch is part of my 'Proof Of Concept' article about
  'NFC meets LoRa on an ESP32'. There are two devices to run the
  demonstration:
  a) The 'card reader unit': it is an ESP32 with attached PN532 NFC tag reader,
     an 170x320 pixel TFT display and a LoRa Transmitter and Receiver module (this unit)
  b) The 'controller unit': this is an ESP32-S3 with LoRa chip and an
     170x320 pixel TFT display (Heltec Vision Master T190) (other unit)




*/

/*
  Dieser Sketch läuft auf einem ESP32 mit angebautem 1,9 inch TFT Display
  mit ST7789 driver chip und 170 x 320 Pixel Auflösung im Hochformat.

  Es liest den Inhalt einer Mifare Classic 1K Karte.

  Die Karten können beschrieben werden durch Eingabe des Befehls 'WRITE'
  im Serial Monitor ("No Line Ending, 115200 baud"). Es wird der Name
  (max. 20 Zeichen), das Zimmer (max 2 Zeichen), die Gültigkeit ab und bis
  im Format TT.MM.JJ (max. 8 Zeichen) und ein Guthaben (max. 8 Zeichen,
  Trenner ist der Dezimalpunkt) eingegeben. Bei 'WRITE1' bzw. 'WRITE2'
  wird ein Demodatensatz geschrieben.

  Arduino 2.3.8, esp32 3.3.8, Adafruit PN532 1.3.4
*/

/*
  Version Management
24.05.2026 V08 Better timing for some pages, code cleaning
23.05.2026 V07 Displays an 'Wait for Time Synchronization' page  
23.05.2026 V06 Receives and Displays the access response  
23.05.2026 V05 Displaying the local time on the idle screen  
23.05.2026 V04 Receiving a timestamp by LoRa and setting system time  
23.05.2026 V03 Changed variable names to English
22.05.2026 V02 Changed reading of the card from String to uint8_t arrays for LoRa transmission
22.05.2026 V01 Initial programming based on 'Esp32_Tft_ST7789_Adafruit_PN532_MifareClassic1K_v07'
*/

char *PROGRAM_VERSION = "ESP32 ST7789 1,9 inch TFT Display with Adafruit PN532 and UART LoRa V08";

//#define ENABLE_TRANSMISSION true

#include <SPI.h>
#include <Adafruit_PN532.h> // https://github.com/adafruit/Adafruit-PN532
#include <TFT_eSPI.h> // https://github.com/Bodmer/TFT_eSPI

// --- PIN Definitions for PN532 NFC Reader ---
#define PN532_SCK (33)
#define PN532_MOSI (32)
#define PN532_SS (25)
#define PN532_MISO (34)

const uint8_t LED_BACKLIGHT_PIN = 21;
int ledBrightness = 50;

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
TFT_eSPI tft = TFT_eSPI();

bool isIdle = false;
uint8_t keya[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // Standard-Key Classic 1K

const long CLOCK_UPDATE_DURATION_MILLIS = 1000;
long lastClockUpdateMillis = 0;

// Global Buffer for LoRa transmission
uint8_t loraName[20];
uint8_t loraRoom[2];
uint8_t loraValidFrom[8];
uint8_t loraValidTo[8];
uint8_t loraDeposit[8];

#include "LORA_UART.h"
const long TRANSMISSION_INTERVAL_MILLIS = 14000;  // 14 seconds
long lastTransmissionMillis = 0;

bool isTimeSync = false;

// --- Brightness Control ---
void setDisplayBrightnessTft_eSPI(int brightness) {
  analogWrite(LED_BACKLIGHT_PIN, brightness);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(PROGRAM_VERSION);

  pinMode(LED_BACKLIGHT_PIN, OUTPUT);
  setDisplayBrightnessTft_eSPI(ledBrightness);

  // Connect to LoRa device
  setupLoraUart();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("PN532 not found!");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("PN532", 85, 80);
    tft.drawString("not", 85, 105);
    tft.drawString("found", 85, 130);
    while (1)
      ;
  }
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0xFF);

  Serial.println("System ready. Commands: WRITE, WRITE1, WRITE2");
  Serial.println("Waiting for Time Synchronization");
  showTimeSyncPage();
}

// --- HELPER ---

String padString(String str, int len) {
  while (str.length() < len) str += " ";
  return str.substring(0, len);
}

String promptInput(String prompt, int maxLen) {
  Serial.println(prompt);
  while (!Serial.available()) delay(10);
  String input = Serial.readStringUntil('\n');
  input.trim();
  return (input.length() > maxLen) ? input.substring(0, maxLen) : input;
}

// --- DISPLAY LOGIC ---

void showTimeSyncPage() {
  tft.fillScreen(TFT_BLACK);

  int centerX = 170 / 2;
  int centerY = 100;

    int r = 50;
  tft.drawCircle(centerX, centerY, r, TFT_WHITE);
  tft.drawCircle(centerX, centerY, r - 2, TFT_LIGHTGREY);

  tft.fillRect(centerX - 2, centerY - r + 5, 4, 10, TFT_WHITE);   // 12 o'clock
  tft.fillRect(centerX - 2, centerY + r - 15, 4, 10, TFT_WHITE);  // 6 o'clock
  tft.fillRect(centerX + r - 15, centerY - 2, 10, 4, TFT_WHITE);  // 3 o'clock
  tft.fillRect(centerX - r + 5, centerY - 2, 10, 4, TFT_WHITE);   // 9 o'clock

  tft.drawLine(centerX, centerY, centerX + 20, centerY - 20, TFT_CYAN);
  tft.drawLine(centerX, centerY, centerX - 15, centerY - 10, TFT_CYAN);
  tft.fillCircle(centerX, centerY, 3, TFT_WHITE); 

  tft.setTextDatum(MC_DATUM);

  int y = 200;

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Wait for", centerX, y, 4);

  y += 45;
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.drawString("TIME", centerX, y, 2);

  y += 45;
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Synchro", centerX, y, 4);
}


void displayNFCData(String name, String room, String validFrom, String validTo, String deposit) {
  setDisplayBrightnessTft_eSPI(250);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("GUEST-INFO");
  tft.drawFastHLine(0, 35, 170, TFT_WHITE);

  tft.setCursor(10, 50);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(1);
  tft.println("NAME:");
  tft.setTextColor(TFT_WHITE);

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
  tft.println("VALIDITY (From-To):");
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

void drawIdleScreen() {
  //setDisplayBrightnessTft_eSPI(40); // reduce the brightness during idle phase
  tft.fillScreen(TFT_BLACK);
  int cx = 170 / 2;
  int cy = 140;
  tft.drawRoundRect(cx - 40, cy - 60, 80, 120, 10, TFT_CYAN);
  tft.drawCircle(cx, cy, 15, TFT_WHITE);
  tft.drawCircle(cx, cy, 25, TFT_CYAN);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("TAP", cx, cy + 90);
  tft.drawString("Mifare", cx, cy + 115);
  tft.drawString("Classic", cx, cy + 140);
  tft.drawString("TAG", cx, cy + 165);

}

void drawLoRaSendScreen() {
  tft.fillScreen(TFT_BLACK);
  setDisplayBrightnessTft_eSPI(250);

  int cx = 170 / 2;
  int cy = 140;

  // --- Draw LoRa Antenna-Icon ---
  tft.drawLine(cx, cy + 40, cx, cy - 20, TFT_WHITE);
  tft.fillTriangle(cx, cy - 25, cx - 5, cy - 15, cx + 5, cy - 15, TFT_CYAN);

  // Airwaves (left/right)
  tft.drawCircle(cx, cy - 20, 15, TFT_CYAN);
  tft.drawCircle(cx, cy - 20, 30, TFT_DARKGREY);

  tft.fillRect(0, cy - 10, 170, 80, TFT_BLACK);
  tft.drawLine(cx, cy + 40, cx, cy - 20, TFT_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("DATA SEND", cx, cy + 80);
  tft.drawString("BY LoRa", cx, cy + 105);

  tft.fillRect(20, 280, 130, 4, TFT_DARKGREY);
  tft.fillRect(20, 280, 130, 4, TFT_GREEN);
}

void drawAccessGrantedScreen() {
  tft.fillScreen(TFT_BLACK);

  int centerX = 170 / 2;
  int centerY = 320 / 3;
  int radius = 40;

  tft.fillCircle(centerX, centerY, radius, TFT_GREEN);

  tft.drawLine(centerX - 20, centerY, centerX - 5, centerY + 15, TFT_WHITE);
  tft.drawLine(centerX - 5, centerY + 15, centerX + 25, centerY - 15, TFT_WHITE);
  tft.drawLine(centerX - 20, centerY + 1, centerX - 5, centerY + 16, TFT_WHITE);
  tft.drawLine(centerX - 5, centerY + 16, centerX + 25, centerY - 14, TFT_WHITE);

  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);

  tft.setTextSize(2);
  tft.drawString("Access", centerX, centerY + 80, 2);
  // "Granted" in Green
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Granted", centerX, centerY + 115, 2);
}


void drawAccessDeniedScreen() {
  tft.fillScreen(TFT_BLACK);  // Bildschirm leeren

  int centerX = 170 / 2;
  int centerY = 320 / 3;
  int radius = 40;

  tft.fillCircle(centerX, centerY, radius, TFT_RED);

  int offset = 18;
  tft.drawLine(centerX - offset, centerY - offset, centerX + offset, centerY + offset, TFT_WHITE);
  tft.drawLine(centerX + offset, centerY - offset, centerX - offset, centerY + offset, TFT_WHITE);
  tft.drawLine(centerX - offset + 1, centerY - offset, centerX + offset + 1, centerY + offset, TFT_WHITE);
  tft.drawLine(centerX + offset + 1, centerY - offset, centerX - offset + 1, centerY + offset, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);

  // "Access" in White
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Access", centerX, centerY + 80, 2);

  // "Denied" in Red
  tft.setTextColor(TFT_RED);
  tft.drawString("Denied", centerX, centerY + 120, 2);
}

// --- MIFARE CLASSIC 1K SPECIFIC ---
void writeClassic1K(String name, String zimmer, String von, String bis, String guthaben) {
  uint8_t uid[4];
  uint8_t uidLen;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) return;

  uint8_t data[16];  // a Mifare Classic sector has 16 bytes

  // Sector 1
  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 4, 0, keya)) {
    // Name part 1
    memset(data, ' ', 16);
    for (int i = 0; i < 16 && i < name.length(); i++) data[i] = name[i];
    nfc.mifareclassic_WriteDataBlock(4, data);

    // Name part 2
    memset(data, ' ', 16);
    if (name.length() > 16) {
      for (int i = 0; i < 4 && (i + 16) < name.length(); i++) data[i] = name[i + 16];
    }
    nfc.mifareclassic_WriteDataBlock(5, data);

    // Room
    memset(data, ' ', 16);
    for (int i = 0; i < 2 && i < zimmer.length(); i++) data[i] = zimmer[i];
    nfc.mifareclassic_WriteDataBlock(6, data);
  }

  // Sector 2
  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 8, 0, keya)) {
    // Valid From
    memset(data, ' ', 16);
    for (int i = 0; i < 8 && i < von.length(); i++) data[i] = von[i];
    nfc.mifareclassic_WriteDataBlock(8, data);

    // Valid To
    memset(data, ' ', 16);
    for (int i = 0; i < 8 && i < bis.length(); i++) data[i] = bis[i];
    nfc.mifareclassic_WriteDataBlock(9, data);

    // Deposit
    memset(data, ' ', 16);
    for (int i = 0; i < 8 && i < guthaben.length(); i++) data[i] = guthaben[i];
    nfc.mifareclassic_WriteDataBlock(10, data);
  }
}

void readClassic1K() {
  uint8_t uid[4];
  uint8_t uidLen;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) return;

  uint8_t data[16];

  // Sector 1: Name and Room
  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 4, 0, keya)) {
    // Block 4 & 5 -> loraName
    nfc.mifareclassic_ReadDataBlock(4, data);
    memcpy(loraName, data, 16);
    nfc.mifareclassic_ReadDataBlock(5, data);
    memcpy(loraName + 16, data, 4);

    // Block 6 -> loraRoom
    nfc.mifareclassic_ReadDataBlock(6, data);
    memcpy(loraRoom, data, 2);
  }

  // Sektor 2: Valid Data and Deposit
  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 8, 0, keya)) {
    // Block 8 -> lora valid from
    nfc.mifareclassic_ReadDataBlock(8, data);
    memcpy(loraValidFrom, data, 8);

    // Block 9 -> lora valid To
    nfc.mifareclassic_ReadDataBlock(9, data);
    memcpy(loraValidTo, data, 8);

    // Block 10 -> lora Deposit
    nfc.mifareclassic_ReadDataBlock(10, data);
    memcpy(loraDeposit, data, 8);
  }

  // convert data to string for displaying the data
  String n = "";
  for (int i = 0; i < 20; i++)
    if (loraName[i] >= 32) n += (char)loraName[i];
  String z = "";
  for (int i = 0; i < 2; i++)
    if (loraRoom[i] >= 32) z += (char)loraRoom[i];
  String v = "";
  for (int i = 0; i < 8; i++)
    if (loraValidFrom[i] >= 32) v += (char)loraValidFrom[i];
  String b = "";
  for (int i = 0; i < 8; i++)
    if (loraValidTo[i] >= 32) b += (char)loraValidTo[i];
  String g = "";
  for (int i = 0; i < 8; i++)
    if (loraDeposit[i] >= 32) g += (char)loraDeposit[i];

  displayNFCData(n, z, v, b, g);
  delay(2000);
  sendNfcData();
  drawLoRaSendScreen();
}

void readClassic1K_v1() {
  uint8_t uid[4];
  uint8_t uidLen;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) return;

  uint8_t data[16];
  char strBuf[17];  // 16 characters + 1 zero byte
  strBuf[16] = '\0';

  String n1 = "", n2 = "", zim = "", v = "", b = "", gut = "";

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 4, 0, keya)) {
    nfc.mifareclassic_ReadDataBlock(4, data);
    memcpy(strBuf, data, 16);
    n1 = String(strBuf).substring(0, 16);
    nfc.mifareclassic_ReadDataBlock(5, data);
    memcpy(strBuf, data, 16);
    n2 = String(strBuf).substring(0, 4);
    nfc.mifareclassic_ReadDataBlock(6, data);
    memcpy(strBuf, data, 16);
    zim = String(strBuf).substring(0, 2);
  }

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 8, 0, keya)) {
    nfc.mifareclassic_ReadDataBlock(8, data);
    memcpy(strBuf, data, 16);
    v = String(strBuf).substring(0, 8);
    nfc.mifareclassic_ReadDataBlock(9, data);
    memcpy(strBuf, data, 16);
    b = String(strBuf).substring(0, 8);
    nfc.mifareclassic_ReadDataBlock(10, data);
    memcpy(strBuf, data, 16);
    gut = String(strBuf).substring(0, 8);
  }

  String fullName = n1 + n2;
  fullName.trim();
  zim.trim();
  v.trim();
  b.trim();
  gut.trim();
  displayNFCData(fullName, zim, v, b, gut);
}

void sendNfcData() {
  // prepare the array, we are using the arrayRW method from the LoRa library
  Serial.println("Received data from NFC card, transmitting them by LoRa");
  txCounter++;
  uint8_t txLength = 0;
  beginarrayRW(txBuffer, 0);    // start writing to control array
  arrayWriteUint8(0x4E);        // 1 byte static for identifying 'N'
  arrayWriteUint32(txCounter);  // 4 bytes
  for (uint8_t i = 0; i < 20; i++) {
    arrayWriteUint8(loraName[i]);
  }
  for (uint8_t i = 0; i < 2; i++) {
    arrayWriteUint8(loraRoom[i]);
  }
  for (uint8_t i = 0; i < 8; i++) {
    arrayWriteUint8(loraValidFrom[i]);
  }
  for (uint8_t i = 0; i < 8; i++) {
    arrayWriteUint8(loraValidTo[i]);
  }
  for (uint8_t i = 0; i < 8; i++) {
    arrayWriteUint8(loraDeposit[i]);
  }
  arrayWriteUint8(0x52);        // 1 byte static for identifying 'R'
  txLength = endarrayRW() + 1;  // this returns the number of array bytes written, is 52 bytes

  Serial.printf("TX (%d): ", txLength);
  Serial.println();

  if (transmitData(txLength)) {
    printTxBuffer(txLength);
    Serial.flush();
  }
}

void syncSystemTime(uint32_t receivedTimestamp) {
  struct timeval tv;
  tv.tv_sec = receivedTimestamp;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

void printCurrentTime() {
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  Serial.printf("Local Time and Date is %02d:%02d:%02d %02d.%02d.%04d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}

void updateClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char timeStr[9];  // Buffer for "hh:mm:ss\0" (8 chars + 0-terminator)
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);              // Zentriert auf X-Achse
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Text weiß, Hintergrund schwarz (verhindert Flackern)

  tft.drawString(timeStr, 85, 10, 4);
}

// --- Main Loop ---

void loop() {

  loraUartLoop();

  if (isReceived) {
    isReceived = false;
    // here is the place to work with the data, I'm just printing the received packet
    // HEX and UTF-8 encodings and displaying HEX encoding
    printRxBuffer(rxBufferLength);
    printRxBufferChar(rxBufferLength);

    // time update
    if (rxBufferLength == 6) {
      beginarrayRW(rxBuffer, 0);                 // start reading from array at location 0
      uint8_t rxHeader = arrayReadUint8();       // 1 byte
      uint32_t rxTimestamp = arrayReadUint32();  // 4 bytes
      uint8_t rxFooter = arrayReadUint8();       // 1 byte
      uint8_t packetLength = endarrayRW() + 1;   // this returns the number of array bytes read (6)
      // short check on magic bytes
      if (rxHeader != 0x59) {
        Serial.printf("The received header %0X does not match 0x59, rejected", rxHeader);
        return;
      }
      if (rxFooter != 0x52) {
        Serial.printf("The received footer %0X does not match 0x52, rejected", rxFooter);
        return;
      }
      Serial.printf("Received timestamp by LoRa: %d\n", rxTimestamp);
      syncSystemTime(rxTimestamp);
      printCurrentTime();
      isTimeSync = true;
    }

    // access update
    if (rxBufferLength == 7) {
      beginarrayRW(rxBuffer, 0);                   // start reading from array at location 0
      uint8_t rxHeader = arrayReadUint8();         // 1 byte
      uint32_t rxCounter = arrayReadUint32();      // 4 bytes
      uint8_t rxAccessGranted = arrayReadUint8();  // 1 byte
      uint8_t rxFooter = arrayReadUint8();         // 1 byte
      uint8_t packetLength = endarrayRW() + 1;     // this returns the number of array bytes read (7)
      // short check on magic bytes
      if (rxHeader != 0x43) {
        Serial.printf("The received header %0X does not match 0x43, rejected", rxHeader);
        return;
      }
      if (rxFooter != 0x52) {
        Serial.printf("The received footer %0X does not match 0x52, rejected", rxFooter);
        return;
      }
      bool accessGranted = false;
      Serial.printf("Received response to access request by LoRa for counter: %d: \n", rxCounter);
      if (rxAccessGranted == 1) {
        accessGranted = true;
        Serial.println("GRANTED");
      } else {
        Serial.println("DENIED");
      }
      if (accessGranted) {
        drawAccessGrantedScreen();
        delay(2000);
      } else {
        drawAccessDeniedScreen();
        delay(2000);
      }
    }
  }

#ifdef ENABLE_TRANSMISSION
  // this is the transmission part
  if (millis() - lastTransmissionMillis > TRANSMISSION_INTERVAL_MILLIS) {
    txCounter++;

    // prepare the array, we are using the arrayRW method from the LoRa library
    uint8_t txLength = 0;
    beginarrayRW(txBuffer, 0);    // start writing to control array
    arrayWriteUint8(0x48);        // 1 byte static for identifying 'N'
    arrayWriteUint32(txCounter);  // 4 bytes
    for (uint8_t i = 0; i < 20; i++) {
      arrayWriteUint8(loraName[i]);
    }
    for (uint8_t i = 0; i < 2; i++) {
      arrayWriteUint8(loraRoom[i]);
    }
    for (uint8_t i = 0; i < 8; i++) {
      arrayWriteUint8(loraValidFrom[i]);
    }
    for (uint8_t i = 0; i < 8; i++) {
      arrayWriteUint8(loraValidTo[i]);
    }
    for (uint8_t i = 0; i < 8; i++) {
      arrayWriteUint8(loraDeposit[i]);
    }
    arrayWriteUint8(0x52);        // 1 byte static for identifying 'R'
    txLength = endarrayRW() + 1;  // this returns the number of array bytes written, is 52 bytes

    Serial.printf("TX (%d): ", txLength);
    Serial.println();

    if (transmitData(txLength)) {
      printTxBuffer(txLength);
      Serial.flush();
    }
    lastTransmissionMillis = millis();
  }
#endif


  // NFC part
  uint8_t uid[4];
  uint8_t uidLen;
  bool cardPresent = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);

  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "WRITE" || cmd == "WRITE1" || cmd == "WRITE2") {
      setDisplayBrightnessTft_eSPI(250);
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("WRITEMODE...", 170 / 2, 160, 4);

      if (!cardPresent) {
        Serial.println("Please TAP Classic 1K...");
        while (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500)) delay(10);
      }

      if (cmd == "WRITE") {
        writeClassic1K(promptInput("Name:", 20), promptInput("Room:", 2), promptInput("Valid From:", 8), promptInput("Valid To:", 8), promptInput("Deposit (max 8):", 8));
      } else if (cmd == "WRITE1") writeClassic1K("Sophia Williams", "12", "01.01.26", "31.12.26", "150.50");
      else if (cmd == "WRITE2") writeClassic1K("Prof Dr John Dullen", "89", "01.07.26", "31.07.26", "600.00");

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.drawString("READY!", 170 / 2, 160, 4);
      delay(2000);
      isIdle = false;
      return;
    }
  }

  if (cardPresent) {
    isIdle = false;
    readClassic1K();
    delay(1500);
  } else {
    if (!isIdle) {
      if (isTimeSync) {
        // draw the idle screen after time sync only
        drawIdleScreen();
        isIdle = true;
      }
    }

    // clock update
    if (millis() - lastClockUpdateMillis > CLOCK_UPDATE_DURATION_MILLIS) {
      updateClock();
      lastClockUpdateMillis = millis();
    }
  }
}
