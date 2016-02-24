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
******************************************************************************/

#define DEBUG_ENABLED

#include "BlynkBoard_settings.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include "FS.h"

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

// BlynkBoard_BlynkMode functions:
void blynkSetup(void);
void blynkLoop(void);

// BlynkBoard_setup functions:
void initHardware(void);
bool checkConfigFlag(void);
bool writeBlynkAuth(String authToken);
String getBlynkAuth(void);
bool setupBlynkStation(String network, String psk, String blynk);
void resetEEPROM(void);

// BlynkBoard_Core_Firmware functions:
void buttonPress(void);
void blinkRGBTimer(void);
void setRGB(uint32_t color);
uint32_t rgbModeConfig(void);
uint32_t evenBlinkRGB(uint32_t onColor, uint32_t period);
uint32_t rgbModeRun(void);

String authTokenStr;

void setup()
{
  runMode = MODE_CONFIG;
  previousMode = runMode;
  initHardware();
  //resetEEPROM();
  
  if (checkConfigFlag() == false)
  {
    generateSSID(false); // Generate a unique RGB-appended SSID
    setupServer();
  }
  else
  {
    authTokenStr = getBlynkAuth();
    
    if (authTokenStr == 0)
    {
      BB_DEBUG("No auth token.");
      //! TODO: If connect times out. Wait for user to hold
      //! TODO: combine this with the fail to connect outcome below
      // GPIO0 LOW for ~5s. If that happens reset EEPROM,
      // jump into MODE_CONFIG.
      runMode = MODE_CONFIG;
      generateSSID(false); // Generate a unique RGB-appended SSID
      setupServer();
    }
    else
    {
      BB_DEBUG("Auth token:" + authTokenStr + ".");
      
      runMode = MODE_CONNECTING_WIFI;
    
      long timeIn = WIFI_STA_CONNECT_TIMEOUT;
      // Relying on persistent ESP8266 WiFi credentials:
      //! TODO: Consider not doing that, using SPIFFS instead.
      while ((WiFi.status() != WL_CONNECTED) && (--timeIn > 0))
        delay(1);
      if (timeIn > 0)
      {
        Blynk.config(authTokenStr.c_str());
        blynkSetup();
      }
      else
      {
        //! TODO: If connect times out. Wait for user to hold
        // GPIO0 LOW for ~5s. If that happens reset EEPROM,
        // jump into MODE_CONFIG.
        runMode = MODE_CONFIG;
        generateSSID(true); // Generate a unique RGB-appended SSID
        setupServer();
      }
    }
  }
  
}

void loop()
{
  switch (runMode)
  {
  case MODE_CONFIG:
    checkForStations();
    break;
  case MODE_CONFIG_DEVICE_CONNECTED:
    handleConfigServer();
    break;
  case MODE_BLYNK_RUN:
    previousMode = MODE_BLYNK_RUN;
    if (Blynk.connected())
      blynkLoop();
    else
      runMode = MODE_BLYNK_ERROR;
    break;
  case MODE_BLYNK_ERROR:
    if (previousMode == MODE_BLYNK_RUN)
    {
      blinker.attach_ms(1, blinkRGBTimer);
    }
    previousMode = MODE_BLYNK_ERROR;
    Blynk.run(); // Try to do a Blynk run
    if (Blynk.connected()) // If it establishes a connection
      runMode = MODE_BLYNK_RUN; // Change to Blynk Run mode
    break;
  default:
    break;
  }
}

void buttonPress(void)
{
  generateSSID();
  blinkCount = 255;
}

void blinkRGBTimer(void)
{
  uint32_t returnTime = 0;
  switch (runMode)
  {
    case MODE_CONFIG:
      if (suffixGenerated)
        returnTime = rgbModeConfig();
      else
        returnTime = evenBlinkRGB(RGB_STATUS_AP_MODE_DEFAULT, RGB_PERIOD_AP_DEFAULT);
      break;
    case MODE_CONFIG_DEVICE_CONNECTED:
      returnTime = evenBlinkRGB(RGB_STATUS_AP_MODE_DEVICE_ON, RGB_PERIOD_AP_DEVICE_ON);
      break;
    case MODE_CONNECTING_WIFI:
      returnTime = evenBlinkRGB(RGB_STATUS_CONNECTING_WIFI, RGB_PERIOD_CONNECTING);
      break;
    case MODE_CONNECTING_BLYNK:
      returnTime = evenBlinkRGB(RGB_STATUS_CONNECTING_BLYNK, RGB_PERIOD_BLYNK_CONNECTING);
      break;
    case MODE_BLYNK_RUN:
      returnTime = rgbModeRun();
      break;
    case MODE_BLYNK_ERROR:
      returnTime = evenBlinkRGB(RGB_STATUS_CANT_CONNECT_BLYNK, RGB_PERIOD_BLINK_ERROR);
      break;
  }
  if (returnTime > 0)
    blinker.attach_ms(returnTime, blinkRGBTimer);
}

uint32_t rgbModeConfig(void)
{
  uint32_t retVal = 0;
  
  if (blinkCount >= SSID_SUFFIX_LENGTH * 2)
  {
    setRGB(WS2812_OFF);
    retVal = RGB_PERIOD_AP_STOP;
    blinkCount = 0;
  }
  else
  {
    if (blinkCount % 2 == 0)
      setRGB(SSID_COLORS[ssidSuffixIndex[blinkCount / 2]]);
    else
      setRGB(WS2812_OFF);
    retVal = RGB_PERIOD_AP / 2;
    blinkCount++;
  }

  return retVal;
}

uint32_t evenBlinkRGB(uint32_t onColor, uint32_t period)
{
  // Assume blinkCount is anywhere between 0-255
  if (blinkCount % 2 == 0)
  {
    setRGB(onColor);
    blinkCount = 1;
  }
  else
  {
    setRGB(WS2812_OFF);
    blinkCount = 0;
  }
  
  return period / 2;
}

uint32_t rgbModeRun(void)
{
  uint8_t blue;
  if (blinkCount < 128)
    blue = blinkCount;
  else
    blue = 255 - blinkCount;
  blue /= 2; // 255 hurts my eyes
  setRGB(rgb.Color(0, 0, blue));

  blinkCount++;
  return RGB_PERIOD_RUNNING / 256;
}

void setRGB(uint32_t color)
{
  rgb.setPixelColor(0, color);
  rgb.show();
}

