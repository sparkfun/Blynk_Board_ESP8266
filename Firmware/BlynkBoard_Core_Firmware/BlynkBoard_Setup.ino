/******************************************************************************
BlynkBoard_Setup.h
BlynkBoard Firmware: Board initialization functions
Jim Lindblom @ SparkFun Electronics
February 24, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware

This file, part of the BlynkBoard Firmware, defines the hardware setup and
initialization functions. That includes EEPROM and flash (SPIFFS) reading
and writing.

Resources:
ESP8266 File System (FS.h) - (included with ESP8266 Arduino board definitions)

License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.

Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
******************************************************************************/

void initHardware(void)
{
  delay(1000);
  Serial.begin(SERIAL_TERMINAL_BAUD);
  BB_DEBUG("");
  BB_DEBUG("SparkFun Blynk Board Hardware v" + String(BLYNKBOARD_HARDWARE_VERSION));
  BB_DEBUG("SparkFun Blynk Board Firmware v" + String(BLYNKBOARD_FIRMWARE_VERSION));
  
  randomSeed(ESP.getChipId());
      
  // Initialize RGB LED and turn it off:
  rgb.begin();
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();
  // Attach a timer to the RGB LED, begin by calling it
  blinker.attach_ms(RGB_PERIOD_AP, blinkRGBTimer);

  // Initialize the button, and setup a hardware interrupt
  // Look for RISING - when the button is released.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);

  // Initialize the LED pin:
  pinMode(BLUE_LED_PIN, OUTPUT);   // Set pin as an output
  digitalWrite(BLUE_LED_PIN, LOW); // Turn the LED off

  if (!SPIFFS.begin())
    BB_DEBUG("Failed to initialize SPIFFS"); //! TODO: consider returning error if this fails.
  EEPROM.begin(EEPROM_SIZE);
}

bool checkConfigFlag(void)
{  
  byte configFlag = EEPROM.read(EEPROM_CONFIG_FLAG_ADDRESS);
  if (configFlag == 1)
    return true;
    
  return false;
}

bool writeBlynkConfig(String authToken, String host, uint16_t port)
{
  File authFile = SPIFFS.open(BLYNK_AUTH_SPIFF_FILE, "w");
  if (authFile)
  {
    BB_DEBUG("Opened " + String(BLYNK_AUTH_SPIFF_FILE));
    authFile.print(authToken);
    authFile.close();
    BB_DEBUG("Wrote file.");
  }
  else
  {
    BB_DEBUG("Failed to open file to write.");
    return false;
  }
  
  File hostFile = SPIFFS.open(BLYNK_HOST_SPIFF_FILE, "w");
  if (hostFile)
  {
    BB_DEBUG("Opened " + String(BLYNK_HOST_SPIFF_FILE));
    hostFile.print(host);
    hostFile.close();
    BB_DEBUG("Wrote " + String(BLYNK_HOST_SPIFF_FILE));
  }
  else
  {
    BB_DEBUG("Failed to open " + String(BLYNK_HOST_SPIFF_FILE));
    return false;
  }
  
  File portFile = SPIFFS.open(BLYNK_PORT_SPIFF_FILE, "w");
  if (portFile)
  {
    BB_DEBUG("Opened " + String(BLYNK_PORT_SPIFF_FILE));
    portFile.print(port);
    portFile.close();
    BB_DEBUG("Wrote " + String(BLYNK_PORT_SPIFF_FILE));
  }
  else
  {
    BB_DEBUG("Failed to open " + String(BLYNK_PORT_SPIFF_FILE));
    return false;
  }
  
  return true;
}

String getBlynkAuth(void)
{
  String retAuth;
  
  if (SPIFFS.exists(BLYNK_AUTH_SPIFF_FILE))
  {
    BB_DEBUG("Opening auth file.");
    File authFile = SPIFFS.open(BLYNK_AUTH_SPIFF_FILE, "r");
    if(authFile)
    {
      BB_DEBUG("File opened.");
      size_t authFileSize = authFile.size();
      // Only return auth token if it's the right size (32 bytes)
      if (authFileSize == BLYNK_AUTH_TOKEN_SIZE)
      {
        while (authFile.available())
        {
          retAuth += (char)authFile.read();
        }
      }
      authFile.close();
    }
    else
    {
      BB_DEBUG("Failed to open auth file.");
    }
  }
  else
  {
    BB_DEBUG("File does not exist.");
  }

  return retAuth;
}

String getBlynkHost(void)
{
  String retHost;
  
  if (SPIFFS.exists(BLYNK_HOST_SPIFF_FILE))
  {
    File hostFile = SPIFFS.open(BLYNK_HOST_SPIFF_FILE, "r");
    if(hostFile)
    {
      BB_DEBUG("Opened" + String(BLYNK_HOST_SPIFF_FILE));
      while (hostFile.available())
        retHost += (char)hostFile.read();
      hostFile.close();
    }
    else
    {
      BB_DEBUG("Failed to open " + String(BLYNK_HOST_SPIFF_FILE));
    }
  }
  else
  {
    BB_DEBUG(String(BLYNK_HOST_SPIFF_FILE) + " does not exist.");
  }

  return retHost;
}

int16_t getBlynkPort(void)
{
  String retPort;
  
  if (SPIFFS.exists(BLYNK_PORT_SPIFF_FILE))
  {
    File portFile = SPIFFS.open(BLYNK_PORT_SPIFF_FILE, "r");
    if(portFile)
    {
      BB_DEBUG("Opened" + String(BLYNK_PORT_SPIFF_FILE));
      while (portFile.available())
        retPort += (char)portFile.read();
      portFile.close();
    }
    else
    {
      BB_DEBUG("Failed to open " + String(BLYNK_PORT_SPIFF_FILE));
    }
  }
  else
  {
    BB_DEBUG(String(BLYNK_PORT_SPIFF_FILE) + " does not exist.");
  }

  return retPort.toInt();
}

int8_t setupBlynkStation(String network, String psk, String blynkAuth, 
                         String blynkHost, uint16_t blynkPort)
{
  WiFi.enableSTA(true);
  WiFi.disconnect();
  
  if (!WiFi.begin(network.c_str(), psk.c_str()))
    return ERROR_CONNECT_WIFI;

  if (!WiFiConnectWithTimeout(WIFI_STA_CONNECT_TIMEOUT))
  {
    BB_DEBUG("Timed out connecting to WiFi.");
    WiFi.enableSTA(false); // Disable station mode
    runMode = MODE_CONFIG; // Back to config LED blink
    return ERROR_CONNECT_WIFI;    
  }

  if (!BlynkConnectWithTimeout(blynkAuth.c_str(), blynkHost.c_str(), 
                               blynkPort, BLYNK_CONNECT_TIMEOUT))
  {
    BB_DEBUG("Timed out connecting to Blynk.");
    runMode = MODE_CONFIG; // Back to config LED blink
    return ERROR_CONNECT_BLYNK;
  }

  writeBlynkConfig(blynkAuth, blynkHost, blynkPort);
  EEPROM.write(EEPROM_CONFIG_FLAG_ADDRESS, 1);
  EEPROM.commit();

  //! TODO: Consider changing mode to an intermediary
  //  MODE_BLYNK_SETUP, where blynkSetup is called.
  blynkSetup();
  
  return WIFI_BLYNK_SUCCESS;
}

long WiFiConnectWithTimeout(unsigned long timeout)
{
  runMode = MODE_CONNECTING_WIFI;
  
  long timeIn = timeout;
  // Relying on persistent ESP8266 WiFi credentials.
  Serial.println("Connecting to: " + WiFi.SSID());
  while ((WiFi.status() != WL_CONNECTED) && (--timeIn > 0))
  {
    if (runMode != MODE_CONNECTING_WIFI)
      return 0;
    delay(1000);
    Serial.print('.');
  }
  Serial.println();
  
  if (timeIn <= 0)
  {
    runMode = MODE_WAIT_CONFIG;
  }

  return timeIn;
}

long BlynkConnectWithTimeout(const char * blynkAuth, const char * blynkServer, 
                             uint16_t blynkPort, unsigned long timeout)
{
  runMode = MODE_CONNECTING_BLYNK;
  // Button interrupt is interfering with something. If the ISR is entered
  // during this loop, an exception occurs. Disable interrupt:
  detachInterrupt(BUTTON_PIN);
  
  long timeIn = timeout;
  
  Blynk.config(blynkAuth, blynkServer, blynkPort);
  
  while ((!Blynk.connected()) && (--timeIn > 0))
  {
    if (runMode != MODE_CONNECTING_BLYNK)
    {
      attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);
      return 0;
    }
      
    Blynk.run();
    delay(1);
  }
  
  if (timeIn <= 0)
  {
    runMode = MODE_WAIT_CONFIG;
  }
  
  attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);
  return timeIn;
}

void resetEEPROM(void)
{
  EEPROM.write(EEPROM_CONFIG_FLAG_ADDRESS, 0);
  EEPROM.commit();
  SPIFFS.remove(BLYNK_AUTH_SPIFF_FILE);
  SPIFFS.remove(BLYNK_HOST_SPIFF_FILE);
  SPIFFS.remove(BLYNK_PORT_SPIFF_FILE);
}
