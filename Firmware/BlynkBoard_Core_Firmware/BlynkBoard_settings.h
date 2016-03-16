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
#include <ESP8266WiFi.h>

#define BLYNKBOARD_FIRMWARE_VERSION "0.8.5"
#define BLYNKBOARD_HARDWARE_VERSION "1.0.0"

#define SERIAL_TERMINAL_BAUD 9600

#ifdef DEBUG_ENABLED
#define BLYNK_PRINT Serial
#define BB_DEBUG(msg) {\
  Serial.print("[" + String(millis()) + "] "); \
  Serial.println(msg); }
#else
#define BB_DEBUG(msg)
#endif

#define BLYNK_AUTH_TOKEN_SIZE 32

///////////////
// Run Modes //
///////////////
enum runModes{
  MODE_SELF_TEST,
  MODE_WAIT_CONFIG,
  MODE_CONFIG,
  MODE_BUTTON_HOLD,
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
#define EEPROM_SIZE 7
#define EEPROM_CONFIG_FLAG_ADDRESS    0
#define EEPROM_SELF_TEST_ADDRESS      1
#define EEPROM_SSID_GENERATED_ADDRESS 2
#define EEPROM_SSID_SUFFX_0           3
#define EEPROM_SSID_SUFFX_1           4
#define EEPROM_SSID_SUFFX_2           5
#define EEPROM_SSID_SUFFX_3           6
const String BLYNK_AUTH_SPIFF_FILE = "/blynk.txt";
const String BLYNK_HOST_SPIFF_FILE = "/blynk_host.txt";
const String BLYNK_PORT_SPIFF_FILE = "/blynk_port.txt";

///////////////////////////////
// Config Server Definitions //
///////////////////////////////
#define BLYNK_WIFI_CONFIG_PORT 80
#define BLYNK_BOARD_URL "blynkme.cc"

//////////////////////////////
// Blynk Server Definitions //
//////////////////////////////
#define BB_BLYNK_HOST_DEFAULT BLYNK_DEFAULT_DOMAIN
#define BB_BLYNK_PORT_DEFAULT BLYNK_DEFAULT_PORT
// If blynk strings aren't global, reconnect's may see them as invalid
static String g_blynkAuthStr;
static String g_blynkHostStr;
uint16_t g_blynkPort;

///////////////////
// WiFi Settings //
///////////////////
#define WIFI_STA_CONNECT_TIMEOUT 30 // WiFi connection timeout (in seconds)
#define BLYNK_CONNECT_TIMEOUT    15000 // Blynk connection timeout (in ms)
IPAddress defaultAPIP(192, 168, 4, 1);
IPAddress defaultAPSub(255, 255, 255, 0);

//////////////////////////
// Hardware Definitions //
//////////////////////////
#define WS2812_PIN 4 // Pin connected to WS2812 LED
#define NUMRGB 1 // Number of WS2812's in the string
Adafruit_NeoPixel rgb = Adafruit_NeoPixel(NUMRGB, WS2812_PIN, NEO_GRB + NEO_KHZ800);
#define BUTTON_PIN 0
#define BLUE_LED_PIN 5
#define ADC_VOLTAGE_DIVIDER 3.2
// ms time that button should be held down to trigger re-config:
#define BUTTON_HOLD_TIME_MIN 3000 

///////////////////////
// RGB Status Colors //
///////////////////////
#define RGB_STATUS_MODE_WAIT_CONFIG   0x202020 // Light white - Start mode
#define RGB_STATUS_MODE_BUTTON_HOLD   0x202020 // Light white - breathing
#define RGB_STATUS_AP_MODE_DEFAULT    0x200000 // Light red - Default AP mode
#define RGB_STATUS_AP_MODE_DEVICE_ON  0x200020 // Light purple - Device connected to AP
#define RGB_STATUS_CONNECTING_WIFI    0x000020 // Light blue - Connecting to WiFi
#define RGB_STATUS_CONNECTED_WIFI     0x000080 // Dark blue - connected to WiFi
#define RGB_STATUS_CANT_CONNECT       0x200000 // Light red - failed to connect to WiFi
#define RGB_STATUS_CONNECTING_BLYNK   0x116926 // Blynk Green - connecting to Blynk cloud
#define RGB_STATUS_CONNECTED_BLYNK    0x116926 // Blynk Green - Connected to Blynk cloud
#define RGB_STATUS_CANT_CONNECT_BLYNK 0x202000 // Light yellow - Failed to connect to Blynk

/////////////////////////////
// RGB Status Blink Period //
/////////////////////////////
#define RGB_PERIOD_SELF_TEST    500
#define RGB_PERIOD_START        1000
#define RGB_PERIOD_BUTTON_HOLD  BUTTON_HOLD_TIME_MIN
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
bool rgbSetByProject = false;

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

////////////////////////
// Serial Config Mode //
////////////////////////
#define SERIAL_RX_BUFFER_SIZE 128
#define NUM_SERIAL_CONFIG_CHARS 4
const char CONFIG_CHAR_WIFI_NETWORK = 'w';
const char CONFIG_CHAR_WIFI_SCAN = 's';
const char CONFIG_CHAR_BLYNK = 'b';
const char CONFIG_CHAR_HELP = 'h';
const char CONFIG_CHAR_SUBMIT = '\r';
const char CONFIG_CHARS[NUM_SERIAL_CONFIG_CHARS] = {
  CONFIG_CHAR_WIFI_SCAN, CONFIG_CHAR_WIFI_NETWORK, 
  CONFIG_CHAR_BLYNK, CONFIG_CHAR_HELP
};
enum {
  SERIAL_CONFIG_WAITING,
  SERIAL_CONFIG_WIFI_SCAN,
  SERIAL_CONFIG_WIFI_NETWORK,
  SERIAL_CONFIG_WIFI_PASSWORD,
  SERIAL_CONFIG_BLYNK,
  SERIAL_CONFIG_BLYNK_HOST,
  SERIAL_CONFIG_BLYNK_PORT
} serialConfigMode = SERIAL_CONFIG_WAITING;
const char SERIAL_MESSAGE_WIFI_NETWORK[] = "\r\nType your WiFi network SSID and hit enter.\r\n> ";
const char SERIAL_MESSAGE_WIFI_PASSWORD[] = "\r\nType your WiFi network password and hit enter.\r\n" \
                                            "(If connecting to an open network, leave blank.)\r\n> " ;
const char SERIAL_MESSAGE_BLYNK_HOST[] = "\r\nType your Blynk Server and hit enter.\r\n" \
                                         "Leave blank to use default: cloud.blynk.cc\r\n> ";
const char SERIAL_MESSAGE_BLYNK_PORT[] = "\r\nType your Blynk Port and hit enter.\r\n" \
                                         "Leave blank to use default: 8442.\r\n> ";
const char SERIAL_MESSAGE_BLYNK[] = "\r\nEnter your 32-character Blynk Auth token.\r\n> ";
const char SERIAL_MESSAGE_HELP[] = "\r\nBlynk Board - ESP8266 Serial Config\r\n" \
                                   "  s: Scan for a WiFi network\r\n" \
                                   "  b: Configure Blynk token, host, and port\r\n" \
                                   "  w: Configure WiFi network\r\n" \
                                   "  h: (This) Help menu\r\n\r\n> ";

/////////////////////////
// Error/Success Codes //
/////////////////////////
enum {
  WIFI_BLYNK_SUCCESS = 1,
  ERROR_CONNECT_WIFI = -1,
  ERROR_CONNECT_BLYNK = -2
};

