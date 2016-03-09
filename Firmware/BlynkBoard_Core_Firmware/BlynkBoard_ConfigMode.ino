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
#include "string.h"
ESP8266WebServer server(BLYNK_WIFI_CONFIG_PORT);
const String SSIDWebFormTop = "<!DOCTYPE HTML> "\
                           "<html><body>" \
                           "<h1>SparkFun Blynk Board Config</h1>" \
                           "<p><form method='get' action='config' id='webconfig'>";
const String SSIDWebFormBtm = "<p>Enter the <b>password</b> for your network. Leave it blank if the network is open:<br>" \
                              "<input type=\"text\" name=\"pass\" placeholder=\"NetworkPassword\"></p>" \
                              "<p>Enter the <b>Blynk auth token</b> for your Blynk project:<br>" \
                              "<input type=\"text\" name=\"blynk\" length=\"32\" placeholder=\"a0b1c2d3e4f5ghijklmnopqrstuvwxyz\"></p>" \
                              "<input type=\"submit\" value=\"submit\"></form>" \
                              "</body></html>";

String serialConfigBuffer = "";
String serialConfigWiFiSSID = "";
String serialConfigWiFiPSK = "";
String serialConfigBlynkAuth = "";

#ifdef CAPTIVE_PORTAL
  #include <DNSServer.h>
  const byte DNS_PORT = 53;
  DNSServer dnsServer;
#endif

void setupAP(char * ssidName)
{
  WiFi.softAP(ssidName);

  IPAddress myIP = WiFi.softAPIP();
  //! Check if IP is 0, return error, or re-try
  BB_DEBUG("AP IP address: " + String(myIP[0]) + "." + 
       String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]));
}

void handleRoot(void) // On root request to server, send form
{
  String webPage = SSIDWebFormTop;
  
  BB_DEBUG("Initiating WiFi network scan.");
  WiFi.enableSTA(true);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n != 0)
  {
    BB_DEBUG("Scan found " + String(n) + " networks");
    webPage += "<p>Select your <b>network</b>, or type it in if it's not in the list:<br>";
    webPage += "<select name=\"ssid\">"; // Begin scrolling selection
    webPage += "<option value=\"\">&mdash; (Not Listed)</option>"; // Add blank option
    for (int i = 0; i < n; ++i)
    {
      webPage += "<option value=\"" + WiFi.SSID(i) + "\">";
      webPage += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + ")";
      webPage += WiFi.encryptionType(i) == ENC_TYPE_NONE ? " " : "*";
      webPage += "</option>";
    }
    webPage += "</select>";
    webPage += "<input type=\"text\" name=\"ssidManual\" placeholder=\"HiddenNetwork\"></p>";
  }
  else
  {
    BB_DEBUG("Scan didn't find any networks.");
    webPage += "<p>Type in your <b>network</b> name:<br>";
    webPage += "<input type=\"text\" name=\"ssid\" placeholder=\"NetworkName\"></p>";  
  }
  webPage += SSIDWebFormBtm;
  Serial.println("");
  WiFi.enableSTA(false);
  
  server.send(200, "text/html", webPage);
}

void handleReset(void)
{
  resetEEPROM();
  server.send(200, "text/html", "<!DOCTYPE HTML><html>EEPROM reset</html>");
}

void handleConfig(void) // handler for "/config" server request
{
  // Gather the arguments into String variables
  String ssidManual = server.arg("ssidManual"); // Network name - entered manually
  String ssidScan = server.arg("ssid"); // Network name - entered from select list
  String pass = server.arg("pass"); // Network password
  String auth = server.arg("blynk"); // Blynk auth code

  String ssid; // Select between the manually or scan entered
  if (ssidScan != "") // Prefer scan entered
    ssid = ssidScan;
  else if (ssidManual != "") // Otherwise, if manually entered is valid
    ssid = ssidManual; // Use manually entered
  else
    return;

  BB_DEBUG("SSID: " + ssid + ".");
  BB_DEBUG("Pass: " + pass + ".");
  BB_DEBUG("Auth: " + auth + ".");

  // Send a response back to the requester
  String rsp = "<!DOCTYPE HTML> <html>";
  rsp += "Connecting to: " + ssid + "<br>";
  rsp += "Then using Blynk auth token: " + auth;
  rsp += "</html>";
  server.send(200, "text/html", rsp);

  //startBlynk(ssid, pass, auth);
  setupBlynkStation(ssid, pass, auth);
}

void setupServer(void)
{
#ifdef CAPTIVE_PORTAL
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.onNotFound(handleRoot);
#endif
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
#ifdef CAPTIVE_PORTAL
  dnsServer.processNextRequest();
#endif
}

void checkForStations(void)
{
  if (WiFi.softAPgetStationNum() > 0)
    runMode = MODE_CONFIG_DEVICE_CONNECTED;
  else
    runMode = MODE_CONFIG;
}

void checkSerialConfig(void)
{
  if (Serial.available())
  {
    while (Serial.available())
    {
      char c = Serial.read();
      if (serialConfigMode == SERIAL_CONFIG_WAITING)
      {
        if (strchr(CONFIG_CHARS, c)) // If it's one of the config chars
        {
          switch (c)
          {
          case CONFIG_CHAR_WIFI_NETWORK:
            serialConfigMode = SERIAL_CONFIG_WIFI_NETWORK;
            Serial.println(SERIAL_MESSAGE_WIFI_NETWORK);
            break;
          case CONFIG_CHAR_WIFI_PASSWORD:
            serialConfigMode = SERIAL_CONFIG_WIFI_PASSWORD;
            Serial.println(SERIAL_MESSAGE_WIFI_PASSWORD);
            break;
          case CONFIG_CHAR_BLYNK:
            serialConfigMode = SERIAL_CONFIG_BLYNK;
            Serial.println(SERIAL_MESSAGE_BLYNK);
            break;
          case CONFIG_CHAR_HELP:
            Serial.println(SERIAL_MESSAGE_HELP);
            break;
          }
        }
        else // If it's not a config char, and we're in waitin mode
        {
          // Ignore it (?)
        }
      }
      else // If it's not a config char, and we're _not_ in waiting mode
      {
        if (c == CONFIG_CHAR_SUBMIT)
        {
          Serial.println();
          executeSerialCommand();
        }
        else if (c == '\b')
        {
          serialConfigBuffer.remove(serialConfigBuffer.length() - 1);
        }
        else //! TODO: Should be checking to make sure the character is valid
        {
          serialConfigBuffer += c;
          Serial.write(c);
        }
      }
    }
  }
}

void executeSerialCommand(void)
{
  bool checkCommand = false;
  static bool passwordEntered = false;
  switch (serialConfigMode)
  {
  case SERIAL_CONFIG_WIFI_NETWORK:
    serialConfigWiFiSSID = serialConfigBuffer;
    serialConfigMode = SERIAL_CONFIG_WIFI_PASSWORD;
    Serial.println(SERIAL_MESSAGE_WIFI_PASSWORD);
    break;
  case SERIAL_CONFIG_WIFI_PASSWORD:
    serialConfigWiFiPSK = serialConfigBuffer;
    serialConfigMode = SERIAL_CONFIG_WAITING;
    passwordEntered = true;
    break;
  case SERIAL_CONFIG_BLYNK:
    serialConfigBlynkAuth = serialConfigBuffer;
    serialConfigMode = SERIAL_CONFIG_WAITING;
    break;
  }
  if ((serialConfigWiFiSSID != "") && (passwordEntered) && (serialConfigBlynkAuth != ""))
  {
    BB_DEBUG("Connecting to WiFi.");
    int8_t ret = setupBlynkStation(serialConfigWiFiSSID, serialConfigWiFiPSK, serialConfigBlynkAuth);
    if (ret < 0)
    {
      if (ret == ERROR_CONNECT_BLYNK)
      {
        BB_DEBUG("Serial error connecting to Blynk.");
        serialConfigBlynkAuth = "";
      }
      else if (ret == ERROR_CONNECT_WIFI)
      {
        BB_DEBUG("Serial error connecting to WiFi.");
        serialConfigWiFiSSID = "";
        serialConfigWiFiPSK = "";
      }
    }
  }
  serialConfigBuffer = "";
}
