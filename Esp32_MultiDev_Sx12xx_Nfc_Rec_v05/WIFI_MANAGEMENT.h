/*
  This is responsible for all Wi-Fi related methods

  In setup(), run these commands
  WiFiInit(); // calls WiFiDisconnect();
  WiFiSetEvents();
  WiFiStationConnect();
*/

/*
* WiFi Events see https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/

0  ARDUINO_EVENT_WIFI_READY               < ESP32 WiFi ready
1  ARDUINO_EVENT_WIFI_SCAN_DONE                < ESP32 finish scanning AP
2  ARDUINO_EVENT_WIFI_STA_START                < ESP32 station start
3  ARDUINO_EVENT_WIFI_STA_STOP                 < ESP32 station stop
4  ARDUINO_EVENT_WIFI_STA_CONNECTED            < ESP32 station connected to AP
5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED         < ESP32 station disconnected from AP
6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
7  ARDUINO_EVENT_WIFI_STA_GOT_IP               < ESP32 station got IP from connected AP
8  ARDUINO_EVENT_WIFI_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
9  ARDUINO_EVENT_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
10 ARDUINO_EVENT_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
11 ARDUINO_EVENT_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
12 ARDUINO_EVENT_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
13 ARDUINO_EVENT_WIFI_AP_START                 < ESP32 soft-AP start
14 ARDUINO_EVENT_WIFI_AP_STOP                  < ESP32 soft-AP stop
15 ARDUINO_EVENT_WIFI_AP_STACONNECTED          < a station connected to ESP32 soft-AP
16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
19 ARDUINO_EVENT_WIFI_AP_GOT_IP6               < ESP32 ap interface v6IP addr is preferred
19 ARDUINO_EVENT_WIFI_STA_GOT_IP6              < ESP32 station interface v6IP addr is preferred
20 ARDUINO_EVENT_ETH_START                < ESP32 ethernet start
21 ARDUINO_EVENT_ETH_STOP                 < ESP32 ethernet stop
22 ARDUINO_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
23 ARDUINO_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
24 ARDUINO_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
19 ARDUINO_EVENT_ETH_GOT_IP6              < ESP32 ethernet interface v6IP addr is preferred
25 ARDUINO_EVENT_MAX
*/

#include <WiFi.h>

const char *ssid         = "Change me";
const char *WiFiPassword = "Change me";

bool isWiFiConnection = false;

char wiFiRssiChar[4]; // is used in DISPLAY_SENSOR_DATA.h
int wiFiRssi;

// fills wiFiRssi with current RSSI value, if not connected = 0
// wiFiRssiChar get the current RSSI value or 'NC' if no connection is available
void WiFiGetRssi() {
  if (isWiFiConnection) {
  wiFiRssi = WiFi.RSSI();
  sprintf(wiFiRssiChar, "%d", wiFiRssi);
  } else {
    wiFiRssi = 0;
    sprintf(wiFiRssiChar, "NC");
  }
}

void WiFiDisconnect() {
  // delete old config
  WiFi.disconnect(true);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
  isWiFiConnection = true;
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("WiFi connected to IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationConnect() {
  WiFi.begin(ssid, WiFiPassword);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  isWiFiConnection = false;
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFiStationConnect();
}

void WiFiSetEvents() {
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void WiFiInit() {
  //WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);
  WiFi.mode(WIFI_STA); //	station mode: the ESP32 connects to an access point
  //WiFi.mode(WIFI_AP_STA);
  //WiFi.mode(WIFI_AP)	access point mode: stations can connect to the ESP32
  WiFiDisconnect();
}