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
  Serial.begin(115200);
  Serial.println();

  //! TODO: find something more unique to randomSeed
  //! Maybe the Si7021's guaranteed unique serial #
  randomSeed(millis() - analogRead(A0));

  // Initialize RGB LED and turn it off:
  rgb.begin();
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();
  // Attach a timer to the RGB LED, begin by calling it
  blinker.attach_ms(RGB_PERIOD_AP, blinkRGBTimer);

  // Initialize the button, and setup a hardware interrupt
  // Look for RISING - when the button is released.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonPress, RISING);

  SPIFFS.begin(); //! TODO: consider returning error if this fails.
  EEPROM.begin(EEPROM_SIZE);
}

bool checkConfigFlag(void)
{  
  byte configFlag = EEPROM.read(EEPROM_CONFIG_FLAG_ADDRESS);

  return configFlag;
}

bool writeBlynkAuth(String authToken)
{
  //! TODO: consider returning fail if file exists.
  //  Or if we even need to worry about overwriting.
  File authFile = SPIFFS.open(BLYNK_AUTH_SPIFF_FILE, "w");
  if (authFile)
  {
    Serial.println("Writing file.");
    authFile.print(authToken);
    authFile.close();
    Serial.println("Wrote file.");
    return true;
  }
  else
  {
    Serial.println("Failed to open file to write.");
    return false;
  }
}

String getBlynkAuth(void)
{
  String retAuth;
  
  if (SPIFFS.exists(BLYNK_AUTH_SPIFF_FILE))
  {
    Serial.println("Opening auth file.");
    File authFile = SPIFFS.open(BLYNK_AUTH_SPIFF_FILE, "r");
    if(authFile)
    {
      Serial.println("File opened.");
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
      Serial.println("Failed to open auth file.");
    }
  }
  else
  {
    Serial.println("File does not exist.");
  }

  return retAuth;
}

bool setupBlynkStation(String network, String psk, String blynk)
{
  long timeIn = WIFI_STA_CONNECT_TIMEOUT;
  
  runMode = MODE_CONNECTING_WIFI;
  blinker.attach_ms(0.1, blinkRGBTimer);

  WiFi.enableSTA(true);
  WiFi.disconnect();
  WiFi.begin(network.c_str(), psk.c_str());
  
  while ((WiFi.status() != WL_CONNECTED) && (--timeIn > 0))
    delay(1);

  if (timeIn <= 0)
  {
    Serial.println("Timed out connecting to WiFi.");
    return false;
  }

  runMode = MODE_CONNECTING_BLYNK;

  Blynk.config(blynk.c_str());
  timeIn = BLYNK_CONNECT_TIMEOUT;
  while ((!Blynk.connected()) && (--timeIn > 0))
  {
    Blynk.run();
    delay(1);
  }

  if (timeIn <= 0)
  {
    Serial.println("Timed out connecting to Blynk.");
    return false;  
  }
  return true;
}

void resetEEPROM(void)
{
  EEPROM.write(EEPROM_CONFIG_FLAG_ADDRESS, 0);
  EEPROM.commit();
  writeBlynkAuth("");
}
