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

bool initHardware(void)
{
  Serial.begin(SERIAL_TERMINAL_BAUD);
  BB_PRINT("");
  BB_PRINT("SparkFun Blynk Board Hardware v" + String(BLYNKBOARD_HARDWARE_VERSION));
  BB_PRINT("SparkFun Blynk Board Firmware v" + String(BLYNKBOARD_FIRMWARE_VERSION));
  
  randomSeed(ESP.getChipId());
      
  // Initialize RGB LED and turn it off:
  rgb.begin();
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();
  // Attach a timer to the RGB LED:
  blinker.attach_ms(RGB_PERIOD_AP, blinkRGBTimer);

  // Initialize the button, and setup a hardware interrupt
  // Look for CHANGE - when the button is pressed or released.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);

  // Initialize the LED pin:
  pinMode(BLUE_LED_PIN, OUTPUT);   // Set pin as an output
  digitalWrite(BLUE_LED_PIN, LOW); // Turn the LED off

  if (!SPIFFS.begin())
  {
    BB_DEBUG("Failed to initialize SPIFFS");
    return false;
  }
  
  EEPROM.begin(EEPROM_SIZE);
  return true;
}

bool checkConfigFlag(void)
{  
  byte configFlag = EEPROM.read(EEPROM_CONFIG_FLAG_ADDRESS);
  if (configFlag == 1)
    return true;
    
  return false;
}

bool checkFailAPSetupFlag(void)
{
  if (EEPROM.read(EEPROM_AP_SETUP_FAIL_FLAG) != AP_SETUP_FAIL_FLAG_VALUE)
    return false;
    
  return true;
}

void writeAPSetupFlag(bool pass)
{
  uint8_t apSetupFlag;
  if (pass) apSetupFlag = AP_SETUP_FAIL_FLAG_VALUE;
  else apSetupFlag = 0;
  EEPROM.write(EEPROM_AP_SETUP_FAIL_FLAG, apSetupFlag);
  EEPROM.commit();
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
  WiFi.disconnect();
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

  // Initialize hardware to get it ready for Blynk mode:
  blynkSetup();
  
  return WIFI_BLYNK_SUCCESS;
}

long WiFiConnectWithTimeout(unsigned long timeout)
{
  runMode = MODE_CONNECTING_WIFI;
  
  long timeIn = timeout;
  // Relying on persistent ESP8266 WiFi credentials.
  BB_PRINT("Connecting to: " + WiFi.SSID());
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
  //detachInterrupt(BUTTON_PIN);
  
  long retVal = 1;
  unsigned long timeoutMs = millis() + timeout;
  
  Blynk.config(blynkAuth, blynkServer, blynkPort);
  
  while ((!Blynk.connected()) && (timeoutMs > millis()))
  {
    if (runMode != MODE_CONNECTING_BLYNK)
    {
      attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);
      return 0;
    }
      
    Blynk.run();
  }
  
  if (millis() >= timeoutMs) // If we' timed out
  {
    retVal = 0;
    BB_DEBUG("Timed out connecting to Blynk");
    runMode = MODE_WAIT_CONFIG;
  }

  attachInterrupt(BUTTON_PIN, buttonChange, CHANGE);
  return retVal;
}

bool checkSelfTestFlag(void)
{
  if (EEPROM.read(EEPROM_SELF_TEST_ADDRESS) == SELF_TEST_FLAG_VALUE)
    return true;
  return false;
}

bool setSelfTestFlag(void)
{
  EEPROM.write(EEPROM_SELF_TEST_ADDRESS, SELF_TEST_FLAG_VALUE);
  EEPROM.commit();
  return true;
}

void performSelfTest(void)
{
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(BLUE_LED_PIN, HIGH); // Turn blue LED on
  rgb.updateLength(2); // Connect to testbed LED
  rgb.setPixelColor(0, 0);
  rgb.setPixelColor(1, 0);
  rgb.show(); // Turn both WS2812's off
  
  //////////////////////////
  // WiFi connection test //
  //////////////////////////
  WiFi.enableSTA(true);
  WiFi.disconnect();
  WiFi.enableSTA(true);
  WiFi.begin("sparkfun-guest","sparkfun6333");
  unsigned timeout = 10000;
  while ((WiFi.status() != WL_CONNECTED) && (--timeout))
    delay(1);
  if (timeout > 0)
  {
    Serial.println("Connected to test network");
    selfTestResult |= (1<<0);
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
  }

  /////////////////
  // Si7021 Test //
  /////////////////
  thSense.begin();
  if (scanI2C(0x40)) // Si7021 is i2c address 0x40
  {
    Serial.println("Si7021 test succeeded");
    selfTestResult |= (1<<1);
  }
  else
  {
    Serial.println("Si7021 test failed");    
  }

  //////////////
  // ADC Test //
  //////////////
  float adcRaw = analogRead(A0); // Read in A0
  float voltage = ((float)adcRaw / 1024.0) * ADC_VOLTAGE_DIVIDER;
  if((voltage < 2.25) && (voltage > 1.75))
  {
    Serial.println("ADC test passed" + String(voltage));   
    selfTestResult |= (1<<2);
  }
  else
  {
    Serial.println("ADC test failed: " + String(voltage));       
  }

  /////////////
  // IO test //
  /////////////
  // Pins 16, 12, and 13 should be tied together.
  // Pin 16 will be written high/low, and pins 12/13 will be read.
  uint8_t ioTest = 0;
  timeout = 1000; // 1s timeout
  pinMode(16, OUTPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  
  // High test:
  digitalWrite(16, HIGH);
  delay(100);
  while ((--timeout))
  {
    if ((digitalRead(12) == HIGH) && (digitalRead(13) == HIGH))
      break;
  }
  if (timeout > 0)
    ioTest |= (1<<0);
  
  // Low test:
  digitalWrite(16, LOW);
  timeout = 1000;
  delay(100);
  while ((--timeout))
  {
    if ((digitalRead(12) == LOW) && (digitalRead(13) == LOW))
      break;
  }
  if (timeout > 0)
    ioTest |= (1<<1);

  if (ioTest == 3)
  {
    Serial.println("IO test passed");  
    selfTestResult |= (1<<3);
  }
  else
  {
    Serial.println("IO test failed");  
  }

  if (selfTestResult == 0xF) // If it passed
  {
    while (1)
    {
      rgb.setPixelColor(1, 0x008000);
      delay(1000);
      rgb.setPixelColor(1, 0);
      delay(1000);
    }
  }
  else // if it failed
  {
    while (1)
    {
      // WiFi failed - purple
      if (!(selfTestResult & (1<<0)))
      {
        rgb.setPixelColor(1, 0x800080);
        rgb.show();
        delay(1000);        
      }
      // Si7021 failed - yellow
      if (!(selfTestResult & (1<<1)))
      {
        rgb.setPixelColor(1, 0x802000);
        rgb.show();
        delay(1000);        
      }
      // ADC failed - red
      if (!(selfTestResult & (1<<2)))
      {
        rgb.setPixelColor(1, 0x800000);
        rgb.show();
        delay(1000);        
      }
      // IO test failed - orange
      if (!(selfTestResult & (1<<3)))
      {
        rgb.setPixelColor(1, 0x806000);
        rgb.show();
        delay(1000);        
      }

      // Blink off
      rgb.setPixelColor(1, 0);
      rgb.show();
      delay(1000);
    }
  }
}

void resetEEPROM(void)
{
  EEPROM.write(EEPROM_CONFIG_FLAG_ADDRESS, 0);
  EEPROM.commit();
  SPIFFS.remove(BLYNK_AUTH_SPIFF_FILE);
  SPIFFS.remove(BLYNK_HOST_SPIFF_FILE);
  SPIFFS.remove(BLYNK_PORT_SPIFF_FILE);
}
