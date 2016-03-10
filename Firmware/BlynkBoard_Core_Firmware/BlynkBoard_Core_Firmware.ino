/******************************************************************************
BlynkBoard_Core_Firmware.ino
BlynkBoard Firmware: Main Source
Jim Lindblom @ SparkFun Electronics
February 24, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware

This file, part of the BlynkBoard Firmware, is the top-level source code.
It includes the main loop() and setup() function. It also maintains the
WS2812 status LED

Resources:
ESP8266WiFi Library (included with ESP8266 Arduino board definitions)
ESP8266WiFiClient Library (included with ESP8266 Arduino board definitions)
ESP8266WebServer Library (included with ESP8266 Arduino board definitions)
Adafruit_NeoPixel Library - https://github.com/adafruit/Adafruit_NeoPixel

License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.

Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
ESP8266 Arduino Core - version 2.1.0-rc2 <- Critical, must be up-to-date
******************************************************************************/

#define DEBUG_ENABLED
//#define CAPTIVE_PORTAL

#include "BlynkBoard_settings.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include "FS.h"
#include <ESP.h>

/////////////////////////
// Function Prototypes //
/////////////////////////
// BlynkBoard_ConfigMode functions:
void handleRoot(void);
void handleReset(void);
void handleConfig(void);
void setupServer(void);
void generateSSID(bool rgbCode = true);
void handleConfigServer(void);
void checkForStations(void);
void setupAP(char * ssidName);
void checkSerialConfig(void);
void executeSerialCommand(void);

// BlynkBoard_BlynkMode functions:
void buttonUpdate(void);
void blynkSetup(void);
void blynkLoop(void);

// BlynkBoard_setup functions:
void initHardware(void);
bool checkConfigFlag(void);
bool writeBlynkConfig(String authToken, String host, uint16_t port);
String getBlynkAuth(void);
String getBlynkHost(void);
int16_t getBlynkPort(void);
int8_t setupBlynkStation(String network, String psk, 
                         String blynkAuth, 
                         String blynkHost = BB_BLYNK_HOST_DEFAULT,
                         uint16_t blynkPort = BB_BLYNK_PORT_DEFAULT);
void resetEEPROM(void);
long WiFiConnectWithTimeout(unsigned long timeout);
long BlynkConnectWithTimeout(const char * blynkAuth, const char * blynkDomain = BB_BLYNK_HOST_DEFAULT,
                             uint16_t blynkPort = BB_BLYNK_PORT_DEFAULT,
                             unsigned long timeout = BLYNK_CONNECT_TIMEOUT);

// BlynkBoard_Core_Firmware functions:
void buttonPress(void);
void blinkRGBTimer(void);
void setRGB(uint32_t color);
uint32_t rgbModeConfig(void);
uint32_t blinkRGB(uint32_t onColor, uint32_t period);
uint32_t breatheRGB(uint32_t colorMax, unsigned int breathePeriod);

// If blynk strings aren't global, reconnect's may see them as invalid
String authTokenStr; 
String blynkHost;
uint16_t blynkPort;

bool rgbSetByProject = false;

void setup()
{
  // runMode keeps track of the Blynk Board's current operating mode.
  // It may be either MODE_WAIT_CONFIG, MODE_CONFIG, MODE_CONFIG_DEVICE_CONNECTED,
  // MODE_CONNECTING_WIFI, MODE_CONNECTING_BLYNK, MODE_BLYNK_RUN,
  // or MODE_BLYNK_ERROR.
  runMode = MODE_WAIT_CONFIG;
  previousMode = runMode; // Previous mode keeps track of the previous runMode
  
  // initHardware() initializes: Serial terminal, randomSeed, WS2812 RGB LED,
  // GP0 button, GP5 LED, SPIFFS (flash storage), and EEPROM.
  initHardware();
  
  // checkConfigFlag() [BlynkBoard_Setup] checks a byte in EEPROM
  // to determine if the Blynk Board's Blynk auth token has been set.
  if (checkConfigFlag())
  { // If the flag has been set, 
    authTokenStr = getBlynkAuth(); // read the stored auth token
    BB_DEBUG("Auth token:" + authTokenStr + ".");
    blynkHost = getBlynkHost(); // Read the stored host
    BB_DEBUG("Blynk Host: " + blynkHost + ".");
    blynkPort = getBlynkPort(); // Read the stored port
    BB_DEBUG("Blynk Port: " + String(blynkPort) + ".");

    // As long as the auth token, host, and port aren't null
    if ((authTokenStr != 0) && (blynkHost != 0) && (blynkPort != 0))
    {
      // Connect to WiFi network stored in ESP8266's persistent flash
      if (WiFiConnectWithTimeout(WIFI_STA_CONNECT_TIMEOUT))
      { // If we successfully connect to WiFi, connect to Blynk
        if (BlynkConnectWithTimeout(authTokenStr.c_str(), blynkHost.c_str(), 
                                    blynkPort, BLYNK_CONNECT_TIMEOUT));
        { // If we successfully connect to Blynk
          blynkSetup(); // Run the Blynk setup function [BlynkBoard_BlynkMode]
        }
      }
    }
  }
  else
  {
    runMode = MODE_CONFIG;
  }
}

void loop()
{
  switch (runMode)
  {
  case MODE_WAIT_CONFIG: // Do nothing, wait for GP0 button
    break;
  case MODE_CONFIG: // Config mode - no connected device
    checkForStations(); // Check for any new connected device
    checkSerialConfig(); // Check for serial config messages
    if (previousMode != MODE_CONFIG)
    {
      generateSSID(true); // Start the AP with the BlynkMe-CCCC SSID
      setupServer(); // Start the config server up:      
      previousMode = MODE_CONFIG;
      Serial.print(SERIAL_MESSAGE_HELP);
    }
    break;
  case MODE_CONFIG_DEVICE_CONNECTED: // Config mode - connected device
    checkForStations(); // Check if any stations disconnect
    handleConfigServer(); // Serve up the config webpage
    checkSerialConfig(); // Check for serial config messages
    break;
  case MODE_BLYNK_RUN: // Main Blynk Demo run mode
    if (previousMode != MODE_BLYNK_RUN) // If this is the first execution of BLYNK_RUN since a state change
    {
      previousMode = MODE_BLYNK_RUN; // previousMode is used by MODE_BLYNK_ERROR
    }
    Blynk.run();
    if (Blynk.connected()) // If Blynk is connected
    {
      blynkLoop(); // Do the blynkLoop [BlynkBoard_BlynkMode]
    }
    else
    {
      runMode = MODE_BLYNK_ERROR; // Otherwise switch to MODE_BLYNK_ERROR mode
      BB_DEBUG("Blynk Disconnected");
    }
    break;
  case MODE_BLYNK_ERROR: // Error connecting to Blynk
    if (previousMode != MODE_BLYNK_ERROR) // If we just got here
    {
      blinker.attach_ms(1, blinkRGBTimer); // Turn on the RGB blink timer
      previousMode = MODE_BLYNK_ERROR; // Update previousMode so blink timer isn't re-called
    }
    Blynk.run(); // Try to do a Blynk run
    if (Blynk.connected()) // If it establishes a connection
    {
      runMode = MODE_BLYNK_RUN; // Change to Blynk Run mode
      BB_DEBUG("Blynk Connected");
    }
    break;
  default: // Modes not defined: MODE_CONNECTING_WIFI, MODE_CONNECTING_BLYNK
    break;
  }
}

// buttonPress is actually set to be called when the GP0 button is released
// [BlynkBoard_Setup] configures the interrupt with:
//    attachInterrupt(BUTTON_PIN, buttonPress, RISING)
void buttonPress(void)
{
  // A button press has different effects in different modes:
  switch (runMode)
  {
  case MODE_WAIT_CONFIG: // If we're in "wait for config mode"
    BB_DEBUG("Switching to config mode");
    runMode = MODE_CONFIG; // A button release will switch to config mode
    break;
  case MODE_CONFIG: // If we're in config mode:
    //BB_DEBUG("Generating a new RGBYP SSID");
    //generateSSID(true); // Create a new SSID, with RGB suffix
    blinkCount = 255; // Restart the blink counter
    break;
  case MODE_CONNECTING_WIFI:
  case MODE_CONNECTING_BLYNK:
    BB_DEBUG("Switching to config mode");
    runMode = MODE_CONFIG;
    break;
  }  
}

// This function is tied to the blinker Ticker. It will either blink or
// breathe the RGB LED, then re-set the Ticker to be called at either
// the same, or a new return time.
void blinkRGBTimer(void)
{
  uint32_t returnTime = 0;
  rgbSetByProject = false;
  switch (runMode)
  {
    case MODE_WAIT_CONFIG: // Waiting for config mode, blink white:
      returnTime = blinkRGB(RGB_STATUS_MODE_WAIT_CONFIG, RGB_PERIOD_START);
      break;
    case MODE_CONFIG:
      if (suffixGenerated) // If a suffix is generated
        returnTime = rgbModeConfig(); // Blink the unique RGBYP code
      else // Otherwise blink red:
        returnTime = blinkRGB(RGB_STATUS_AP_MODE_DEFAULT, RGB_PERIOD_AP_DEFAULT);
      break;
    case MODE_CONFIG_DEVICE_CONNECTED: // Device connected in config mode, blink purple:
      returnTime = blinkRGB(RGB_STATUS_AP_MODE_DEVICE_ON, RGB_PERIOD_AP_DEVICE_ON);
      break;
    case MODE_CONNECTING_WIFI: // Connecting to a WiFi AP, blink green
      returnTime = blinkRGB(RGB_STATUS_CONNECTING_WIFI, RGB_PERIOD_CONNECTING);
      break;
    case MODE_CONNECTING_BLYNK: // Connecting to Blynk cloud, blink blue
      returnTime = blinkRGB(RGB_STATUS_CONNECTING_BLYNK, RGB_PERIOD_BLYNK_CONNECTING);
      break;
    case MODE_BLYNK_RUN: // In Blynk run mode, breathe blue
      returnTime = breatheRGB(RGB_STATUS_CONNECTED_BLYNK, RGB_PERIOD_RUNNING);
      break;
    case MODE_BLYNK_ERROR: // Error connecting to Blynk, blink yellow
      returnTime = blinkRGB(RGB_STATUS_CANT_CONNECT_BLYNK, RGB_PERIOD_BLINK_ERROR);
      break;
  }
  if (returnTime > 0) // Call this function again in returnTime ms
    blinker.attach_ms(returnTime, blinkRGBTimer);
}
// Blink the LED with a unique-ish combination of R/G/B/Y/P
// Returns a time, in ms, when this function should be called again
uint32_t rgbModeConfig(void)
{
  uint32_t retVal = 0;

  // Assume blinkCount is anywhere between 0-255
  // If it's greater than our suffix length (4) times two
  if (blinkCount >= SSID_SUFFIX_LENGTH * 2)
  {
    setRGB(WS2812_OFF); // Turn the LED off
    retVal = RGB_PERIOD_AP_STOP; // Come back in a longer time
    blinkCount = 0; // Reset blinkCount
  }
  else // If we're blinking a color, or mid-color
  {
    if (blinkCount % 2 == 0) // If even, blink the color
      setRGB(SSID_COLORS[ssidSuffixIndex[blinkCount / 2]]);
    else // If odd, turn the LED off
      setRGB(WS2812_OFF);
    retVal = RGB_PERIOD_AP / 2; // Come back in a shorter time
    blinkCount++; // Increment blinkCount
  }

  return retVal; // Return the blink time
}

// Blink the LED a specific color with a set period.
// 50% duty cycle ~ even on/off time.
// Returns a time, in ms, when this function should be called again
uint32_t blinkRGB(uint32_t onColor, uint32_t period)
{
  // Assume blinkCount is anywhere between 0-255
  if (blinkCount % 2 == 0) // If blinkCount is even (0)
  {
    setRGB(onColor); // Turn the LED on
    blinkCount = 1; // Turn the LED off next time
  }
  else // If blinkCount is odd (1)
  {
    setRGB(WS2812_OFF); // Turn LED off
    blinkCount = 0; // Turn the LED on next time
  }
  
  return period / 2; // Come back in half the period
}

// Breathe the LED between off and a maximum 32-bit color value
// Returns a time, in ms, when this function should be called again
uint32_t breatheRGB(uint32_t colorMax, unsigned int breathePeriod)
{
  // Find the three r/g/b colors from colorMax:
  uint8_t redMax = (colorMax & 0xFF0000) >> 16;
  uint8_t greenMax = (colorMax & 0x00FF00) >> 8;
  uint8_t blueMax = (colorMax & 0x0000FF);

  // Determine the color brightness, based on blinkCount.
  // brightness will steadily rise from 0 to 128, then steadily fall back to 0
  uint8_t brightness;
  if (blinkCount < 128) // If it's 0-128
    brightness = blinkCount; // Leave it be
  else // If it's 129->255
    brightness = 255 - blinkCount; // Adjust it to 127-0

  // Multiply our three colors by the brightness:
  redMax *= ((float)brightness / 128.0);
  greenMax *= ((float)brightness / 128.0);
  blueMax *= ((float)brightness / 128.0);
  // And turn the LED to that color:
  setRGB(rgb.Color(redMax, greenMax, blueMax));

  blinkCount++; // Increment blinkCount
  // This function relies on the 8-bit, unsigned blinkCount rolling over. 
  // If it's 255, it'll go back to 0.
  return breathePeriod / 256;
}

// All-in-one RGB-setting function
void setRGB(uint32_t color)
{
  rgb.setPixelColor(0, color);
  rgb.show();
}

