/******************************************************************************
BlynBoard_settings.h
BlynkBoard Firmware: Board configuration and settings
Jim Lindblom @ SparkFun Electronics
February 24, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware

This file, part of the BlynkBoard Firmware, determines run-time characteristics
of the BlynkBoard. That includes RGB mode colors, blink rates, and connection 
timeouts.

Resources:
Adafruit_NeoPixel Library - https://github.com/adafruit/Adafruit_NeoPixel
ESP8266 Ticker Library (included with ESP8266 Arduino board definitions)

License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.

Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
******************************************************************************/

#include <Ticker.h>
#include <Adafruit_NeoPixel.h>

#define BLYNKBOARD_FIRMWARE_VERSION "0.5.0"
#define BLYNKBOARD_HARDWARE_VERSION "1.0.0"

#define SERIAL_TERMINAL_BAUD 9600

#ifdef DEBUG_ENABLED
#define BLYNK_PRINT Serial
#define BB_DEBUG(msg) {\
  Serial.print("[" + String(millis()) + "] "); \
  Serial.println(msg);}
#else
#define BB_DEBUG(msg)
#endif

#define BLYNK_AUTH_TOKEN_SIZE 32

///////////////
// Run Modes //
///////////////
enum runModes{
  MODE_WAIT_CONFIG,
  MODE_CONFIG,
  MODE_CONFIG_DEVICE_CONNECTED,
  MODE_CONNECTING_WIFI,
  MODE_CONNECTING_BLYNK,
  MODE_BLYNK_RUN,
  MODE_BLYNK_ERROR
};
runModes runMode, previousMode;

//////////////////////////////////
// EEPROM and NV-Memory Defines //
//////////////////////////////////
#define EEPROM_SIZE 1
#define EEPROM_CONFIG_FLAG_ADDRESS 0
const String BLYNK_AUTH_SPIFF_FILE = "/blynk.txt";

///////////////////////////////
// Config Server Definitions //
///////////////////////////////
#define BLYNK_WIFI_CONFIG_PORT 80

///////////////////
// WiFi Settings //
///////////////////
#define WIFI_STA_CONNECT_TIMEOUT 30000
#define BLYNK_CONNECT_TIMEOUT    15000

//////////////////////////
// Hardware Definitions //
//////////////////////////
#define WS2812_PIN 4 // Pin connected to WS2812 LED
#define NUMRGB 1 // Number of WS2812's in the string
Adafruit_NeoPixel rgb = Adafruit_NeoPixel(NUMRGB, WS2812_PIN, NEO_GRB + NEO_KHZ800);
#define BUTTON_PIN 0
#define BLUE_LED_PIN 5

///////////////////////
// RGB Status Colors //
///////////////////////
#define RGB_STATUS_MODE_WAIT_CONFIG   0x202020 // Light white - Start mode
#define RGB_STATUS_AP_MODE_DEFAULT    0x200000 // Light red - Default AP mode
#define RGB_STATUS_AP_MODE_DEVICE_ON  0x200020 // Light purple - Device connected to AP
#define RGB_STATUS_CONNECTING_WIFI    0x002000 // Light green - Connecting to WiFi
#define RGB_STATUS_CONNECTED_WIFI     0x008000 // Dark green - connected to WiFi
#define RGB_STATUS_CANT_CONNECT       0x200000 // Light red - failed to connect to WiFi
#define RGB_STATUS_CONNECTING_BLYNK   0x000020 // Light blue - connecting to Blynk cloud
#define RGB_STATUS_CONNECTED_BLYNK    0x000080 // Dark blue - Connected to Blynk cloud
#define RGB_STATUS_CANT_CONNECT_BLYNK 0x202000 // Light yellow - Failed to connect to Blynk

/////////////////////////////
// RGB Status Blink Period //
/////////////////////////////
#define RGB_PERIOD_START        1000
#define RGB_PERIOD_AP           1000
#define RGB_PERIOD_AP_STOP      2000
#define RGB_PERIOD_AP_DEFAULT   1000
#define RGB_PERIOD_AP_DEVICE_ON 500
#define RGB_PERIOD_CONNECTING   250
#define RGB_PERIOD_RUNNING      5000
#define RGB_PERIOD_BLYNK_CONNECTING 1000
#define RGB_PERIOD_BLINK_ERROR      1000

///////////////////
// BlynkRGB SSID //
///////////////////
Ticker blinker; // Timer to blink LED
uint8_t blinkCount = 0; // Timer iteration counter

#define WS2812_OFF 0x000000
#define WS2812_RED 0x200000
#define WS2812_GREEN 0x002000
#define WS2812_BLUE 0x000020
#define WS2812_YELLOW 0x202000
#define WS2812_PURPLE 0x200040
#define WS2812_NUM_COLORS 5 // Number of possible colors

const uint32_t SSID_COLORS[WS2812_NUM_COLORS] = {WS2812_RED,
                                                 WS2812_GREEN, WS2812_BLUE, WS2812_YELLOW, WS2812_PURPLE
                                                };
const char SSID_COLOR_CHAR[WS2812_NUM_COLORS] = {
  'R', 'G', 'B', 'Y', 'P'
}; // Map colors to a character

#define SSID_SUFFIX_LENGTH 4
const char SSID_PREFIX[] = "BlynkMe";
uint8_t ssidSuffixIndex[SSID_SUFFIX_LENGTH] = {0, 0, 0, 0};
char BoardSSID[33];
bool suffixGenerated = false;

////////////////////////
// Serial Config Mode //
////////////////////////
#define SERIAL_RX_BUFFER_SIZE 128
#define NUM_SERIAL_CONFIG_CHARS 4
const char CONFIG_CHAR_WIFI_NETWORK = 'w';
const char CONFIG_CHAR_WIFI_PASSWORD = 'p';
const char CONFIG_CHAR_BLYNK = 'b';
const char CONFIG_CHAR_HELP = 'h';
const char CONFIG_CHAR_SUBMIT = '\r';
const char CONFIG_CHARS[NUM_SERIAL_CONFIG_CHARS] = {
  CONFIG_CHAR_WIFI_NETWORK, CONFIG_CHAR_WIFI_PASSWORD, CONFIG_CHAR_BLYNK,
  CONFIG_CHAR_HELP
};
enum {
  SERIAL_CONFIG_WAITING,
  SERIAL_CONFIG_WIFI_NETWORK,
  SERIAL_CONFIG_WIFI_PASSWORD,
  SERIAL_CONFIG_BLYNK
} serialConfigMode = SERIAL_CONFIG_WAITING;
const char SERIAL_MESSAGE_WIFI_NETWORK[] = "Type your WiFi network SSID and hit enter.\r\n";
const char SERIAL_MESSAGE_WIFI_PASSWORD[] = "Type your WiFi network password and hit enter.\r\n" \
                                            "(If connecting to an open network, leave blank.)\r\n" ;
const char SERIAL_MESSAGE_BLYNK[]= "Enter your 32-character Blynk Auth token.\r\n";
const char SERIAL_MESSAGE_HELP[] = "Blynk Board - ESP8266 Serial Config\r\n" \
                                   "  w: Config WiFi network\r\n" \
                                   "  p: Config WiFi password\r\n" \
                                   "  b: Config Blynk Auth token\r\n" \
                                   "  h: (This) Help menu\r\n";

/////////////////////////
// Error/Success Codes //
/////////////////////////
enum {
  WIFI_BLYNK_SUCCESS = 1,
  ERROR_CONNECT_WIFI = -1,
  ERROR_CONNECT_BLYNK = -2
};

