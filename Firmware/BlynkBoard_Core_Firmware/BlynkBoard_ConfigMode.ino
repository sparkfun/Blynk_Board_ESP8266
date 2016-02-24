/******************************************************************************
BlynkBoard_ConfigMode.ino
BlynkBoard Firmware: Config Mode Source
Jim Lindblom @ SparkFun Electronics
February 24, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware

This file, part of the BlynkBoard Firmware, implements all "Config Mode"
functions of the BlynkBoard Core Firmware. That includes setting up an access
point (AP), serving the config server, and listening for serial configuration 
commands.

Resources:
ESP8266WiFi Library (included with ESP8266 Arduino board definitions)
ESP8266WiFiClient Library (included with ESP8266 Arduino board definitions)
ESP8266WebServer Library (included with ESP8266 Arduino board definitions)

License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.

Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
******************************************************************************/

///////////////////////////
// BlynkMe Config Server //
///////////////////////////
ESP8266WebServer server(BLYNK_WIFI_CONFIG_PORT);
const String SSIDWebForm = "<!DOCTYPE HTML> "\
                           "<html>" \
                           "<h1>Blynk Board Config</h1>" \
                           "<p>Enter your network name (SSID), password, and Blynk Auth token:</p>" \
                           "<p><form method='get' action='config'>" \
                           "<label>SSID: </label><input name='ssid' length=32><br>" \
                           "<label>PASS: </label><input name='pass' type = 'password' length=64><br>" \
                           "<label>Blynk: </label><input name='blynk' length=32><br><br>" \
                           "<input type='submit' value='submit'></form></p></html>";

void setupAP(char * ssidName)
{
  WiFi.softAP(ssidName);

  IPAddress myIP = WiFi.softAPIP();
  BB_DEBUG("AP IP address: " + String(myIP));
}

void handleRoot(void) // On root request to server, send form
{
  server.send(200, "text/html", SSIDWebForm);
}

void handleReset(void)
{
  resetEEPROM();
  server.send(200, "text/html", "<!DOCTYPE HTML><html>EEPROM reset</html>");
}

void handleConfig(void) // handler for "/config" server request
{
  // Gather the arguments into String variables
  String ssid = server.arg("ssid"); // Network name
  String pass = server.arg("pass"); // Network password
  String auth = server.arg("blynk"); // Blynk auth code

  BB_DEBUG("SSID: " + ssid + ".");
  BB_DEBUG("Pass: " + pass + ".");
  BB_DEBUG("Auth: " + auth + ".");

  // Send a response back to the requester
  String rsp = "<!DOCTYPE HTML> <html>";
  rsp += "Connecting to: " + ssid + "<br>";
  rsp += "Then using Blynk auth token: " + auth;
  rsp += "</html>";
  server.send(200, "text/html", rsp);

  // Setup the BlynkBoard as a WiFi station
  if (setupBlynkStation(ssid, pass, auth))
  {
    writeBlynkAuth(auth); //! TODO: check return value of this
    WiFi.enableAP(false);
    
    // Detach button input so it can be used in Blynk:
    detachInterrupt(BUTTON_PIN);
    
    EEPROM.write(EEPROM_CONFIG_FLAG_ADDRESS, 1);
    EEPROM.commit();

    blynkSetup();
  }
  else
  {
    WiFi.enableSTA(false); // Disable station mode
    runMode = MODE_CONFIG; // Back to config LED blink
  }
}

void setupServer(void)
{
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/reset", handleReset);
  server.begin();

  BB_DEBUG("HTTP server started");
}

void generateSSID(bool rgbCode)
{
  strcpy(BoardSSID, SSID_PREFIX);
  if (rgbCode)
  {
    for (int i = 0; i < SSID_SUFFIX_LENGTH; i++)
    {
      ssidSuffixIndex[i] = random(0, WS2812_NUM_COLORS);
      strncat(BoardSSID, &SSID_COLOR_CHAR[ssidSuffixIndex[i]], 1);
    }
    BB_DEBUG("New SSID: " + String(BoardSSID));
  }
  suffixGenerated = rgbCode;

  setupAP(BoardSSID);
  blinkCount = 0;
}

void handleConfigServer(void)
{
  server.handleClient();
  if (WiFi.softAPgetStationNum() == 0)
  {
    if (runMode == MODE_CONFIG_DEVICE_CONNECTED)
      runMode = MODE_CONFIG;
  }
}

void checkForStations(void)
{
  if (WiFi.softAPgetStationNum() > 0)
    runMode = MODE_CONFIG_DEVICE_CONNECTED;
  else
    runMode = MODE_CONFIG;
}
