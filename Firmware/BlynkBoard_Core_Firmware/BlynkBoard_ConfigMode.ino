/******************************************************************************
BlynkBoard_ConfigMode.ino
Blynk Board Firmware: Config Mode Source
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
const String SSIDWebFormHdr = R"raw_string(
<!DOCTYPE HTML>
<html><head>
  <meta content="text/html;charset=utf-8" http-equiv="Content-Type">
  <meta content="utf-8" http-equiv="encoding">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>SparkFun Blynk Board Config</title>
  <style>
* { box-sizing: border-box }
h1, h2, h3, h4, h5, h6, .h1, .h2, .h3, .h4, .h5, .h6 {
    font-family: Montserrat,"Helvetica Neue",Helvetica,Arial,sans-serif;
    font-weight: 400;
}
h1, .h1, h2, .h2, h3, .h3 {
    margin-top: 20px;
    margin-bottom: 10px;
}
h1, h2, h3, h4, h5, h6, .h1, .h2, .h3, .h4, .h5, .h6 {
    font-family: inherit;
    font-weight: 500;
    line-height: 1.1;
    color: #555;
}
a {
    color: #e0311d;
}
body {
    background-color:white; 
    font-family: "Helvetica Neue",Helvetica,Arial,sans-serif;
    font-size: 14px;
    line-height: 1.42857143;
    color: #333;
}
input, select {
  border: none;
  padding:7px;
  margin: 7px;
  width: calc(100% - 2*7px);
  color: white;
  background-color: #e0311d;
  font-weight: 700;
  -webkit-border-radius:3px;
  -moz-border-radius:3px;
  -webkit-appearance:none;
  border-radius:3px;
}
form {
  margin: 0 auto;
  width: 400px;
}

@media all and (max-width: 768px){
  form { width:100% }
}
input[name=host] { width: calc(70% - 2*7px); }
input[name=port] { width: calc(30% - 2*7px); float:right; }
input[type=submit] { width: 100px; border: 1px solid black }
select { padding:6px }
::-webkit-input-placeholder{color:rgba(255,255,255,0.8)}
:-moz-placeholder{color:rgba(255,255,255,0.8)}
::-moz-placeholder{color:rgba(255,255,255,0.8)}
:-ms-input-placeholder{color:rgba(255,255,255,0.8)}
:placeholder-shown{color:rgba(255,255,255,0.8)}
  </style>
</head>
<body>
)raw_string";

const String SSIDWebFormStart = R"raw_string(
  <form method="get" action="config" id="webconfig">
  <h1>SparkFun Blynk Board Config</h1>
)raw_string";

const String SSIDWebFormBtm = R"raw_string(
  <p>Enter the <b>password</b> for your network:<br>(leave blank if the network is open)<br>
  <input type="password" name="pass" placeholder="WiFi password"></p>
  <p>Enter the <b>Blynk auth token</b> for your project:<br>
  <input type="text" name="blynk" placeholder="a0b1c2d..." pattern="[a-zA-Z0-9]{32}" maxlength="32" required="required"></p>
  <p>Enter the Blynk <b>host and port</b> for your Blynk project:<br>
  (leave as-is for defaults)<br>
  <input type="text" name="host" value="blynk-cloud.com"><input type="number" name="port" value="8442" min="1" max="65535"></p>
  <input type="submit" value="Apply"></form>

  <script type="text/javascript">
function onNetSelect(){
  var net; if(net = document.getElementById("net")){
    document.getElementById("manual").style.display = (net.options[net.selectedIndex].value==="")?"":"none";
  }
}
window.onload = function(){
  onNetSelect();
};
  </script>
</body></html>
)raw_string";

const String SSIDWebRspFtr = R"raw_string(
  <h2>RGB LED Status</h2>
  <p><b><span style="color:blue">&#x25cf; Blue</span></b>: Connecting to WiFi<br>
  <b><span style="color:#1FD695;">&#x25cf; Blynk green (blink)</span></b>: Connecting to blink<br>
  <b><span style="color:#1FD695;">&#x25cf; Blynk green (fade in/out)</span></b>: Connected to Blynk!<br>
  <b><span style="color:purple;">&#x25cf; Purple (blink)</span></b>: Failed to connect to WiFi or Blynk. Phone re-connected.<br>
  <b><span style="color:red;">&#x25cf; R/G/B/Y/P color code</span></b>: Failed to connect to WiFi or Blynk<br>
  <b><span style="color:#C0C000;">&#x25cf; Yellow (blink)</span></b>: Error connecting to Blynk server.</p>
  <hr>
  <p>If the LED is <b>"breathing" Blynk-green</b>, it's connected! Switch to the Blynk app to start blinking!</p>
  <p>If the LED has reverted back to <b>purple or the R/G/B/Y/P color code</b>, go back to <a href="blynkme.cc">blynkme.cc</a> and try again.</p>
  <p>If the LED is <b>blinking yellow</b>, the Blynk Board encountered an error. Unplug it, wait a couple seconds, then plug it back in. It should attempt to connect again.</p>  
  </body></html>
)raw_string";


DNSServer dnsServer;
const byte DNS_PORT = 53;
  
bool setupAP(char * ssidName)
{
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(defaultAPIP, defaultAPIP, defaultAPSub);
  WiFi.softAP(ssidName);

  IPAddress myIP = WiFi.softAPIP();
  BB_DEBUG("AP IP address: " + String(myIP[0]) + "." + 
       String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]));
       
  //! ESP8266 bug: IP may be 0 -- makes AP un-connectable
  if (myIP == (uint32_t)0)
  {
    writeAPSetupFlag(false);
    return false;
  }
  else
  {
    writeAPSetupFlag(true);
    return true;
  }
}

void handleRoot(void) // On root request to server, send form
{
  String webPage = SSIDWebFormHdr;
  webPage += SSIDWebFormStart;
  BB_DEBUG("Initiating WiFi network scan.");
  WiFi.enableSTA(true);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n != 0)
  {
    BB_DEBUG("Scan found " + String(n) + " networks");
    webPage += "<p>Select your <b>WiFi network</b>:<br>";
    webPage += "<select name='ssid' id='net' onChange='onNetSelect()' onkeyup='onNetSelect()' >"; // Begin scrolling selection

    // Sort networks
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    
    for (int i = 0; i < n; ++i)
    {
      int id = indices[i];
      webPage += "<option value=\"" + WiFi.SSID(id) + "\">";
      webPage += WiFi.SSID(id) + " (" + String(WiFi.RSSI(id)) + ")";
      webPage += WiFi.encryptionType(id) == ENC_TYPE_NONE ? " " : "*";
      webPage += "</option>";
    }
    webPage += "<option value=\"\">[ Enter manually ... ]</option>"; // Add manual option
    webPage += "</select><br>";
    webPage += "<input type=\"text\" name=\"ssidManual\" id='manual' placeholder=\"Network name\"></p>";
  }
  else
  {
    BB_DEBUG("Scan didn't find any networks.");
    webPage += "<p>Type in your <b>network</b> name:<br>";
    webPage += "<input type=\"text\" name=\"ssid\" placeholder=\"Network name\"></p>";  
  }
  webPage += SSIDWebFormBtm;
  WiFi.enableSTA(false);
  
  server.send(200, "text/html", webPage);
  BB_DEBUG("Root served.");
}

void handleReset(void)
{
  resetEEPROM();
  server.send(200, "text/html", "<!DOCTYPE HTML><html>EEPROM reset</html>");
}

void handleBoardInfo(void)
{
  char buff[256];
  const char* fmt =
R"json({
  "board": "Blynk Board",
  "vendor": "SparkFun",
  "fw_ver": "%s",
  "hw_ver": "%s",
  "blynk_ver": "%s"
})json";

  snprintf(buff, sizeof(buff), fmt,
    BLYNKBOARD_FIRMWARE_VERSION,
    BLYNKBOARD_HARDWARE_VERSION,
    BLYNK_VERSION
  );
  server.send(200, "application/json", buff);
}

void handleConfig(void) // handler for "/config" server request
{
  // Gather the arguments into String variables
  String ssidManual = server.arg("ssidManual"); // Network name - entered manually
  String ssidScan = server.arg("ssid"); // Network name - entered from select list
  String pass = server.arg("pass"); // Network password
  g_blynkAuthStr = server.arg("blynk"); // Blynk auth code
  g_blynkHostStr = server.arg("host");
  g_blynkPort = server.arg("port").toInt();
  
  String ssid; // Select between the manually or scan entered
  if (ssidScan != "") // Prefer scan entered
    ssid = ssidScan;
  else if (ssidManual != "") // Otherwise, if manually entered is valid
    ssid = ssidManual; // Use manually entered
  else
    return;

  BB_DEBUG("SSID: " + ssid);
  BB_DEBUG("Pass: " + pass);
  BB_DEBUG("Auth: " + g_blynkAuthStr);
  BB_DEBUG("Host: " + g_blynkHostStr);
  BB_DEBUG("Port: " + String(g_blynkPort));

  // Send a response back to the requester
  String rsp = SSIDWebFormHdr;
  rsp += "<h2>WiFi/Blynk Connecting...</h2><p>Your Blynk Board is attempting to connect to <b>";
  rsp += ssid + "</b></p>";
  rsp += "<p>Then it'll use the Blynk auth token <b>" + g_blynkAuthStr + "</b>";
  rsp += " to connect to " + g_blynkHostStr + ":" + String(g_blynkPort); + ".</p>";
  rsp += SSIDWebRspFtr;

  server.send(200, "text/html", rsp);

  if ((g_blynkHostStr != "") && (g_blynkPort != 0)) // If host and port are not null/0
  {
    BB_DEBUG("Connecting to " + g_blynkHostStr);
    setupBlynkStation(ssid, pass, g_blynkAuthStr, g_blynkHostStr, g_blynkPort); // Connect using those
  }
  else
  {
    BB_DEBUG("Connecting to default server");
    setupBlynkStation(ssid, pass, g_blynkAuthStr); // Otherwise connect using defaults
  }
}

void setupServer(void)
{
  // Set up DNS Server
  dnsServer.setTTL(300); // Time-to-live 300s
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure); // Return code for non-accessible domains
#ifdef CAPTIVE_PORTAL_ENABLE
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Point all to our IP
  server.onNotFound(handleRoot);
#else
  dnsServer.start(DNS_PORT, BLYNK_BOARD_URL, WiFi.softAPIP()); // Point blynkme.cc to our IP
#endif

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/reset", handleReset);
  server.on("/board_info.json", handleBoardInfo);
  server.begin();

  BB_DEBUG("HTTP server started");
}

void generateSSIDSuffix(bool newSuffix)
{
  // If asking for a new suffix, or suffix hasn't been generated
  if ((newSuffix) || (EEPROM.read(EEPROM_SSID_GENERATED_ADDRESS) != 42))
  {
    for (int i=0; i<SSID_SUFFIX_LENGTH; i++)
    {
      // Create new random suffix indices
      ssidSuffixIndex[i] = random(0, WS2812_NUM_COLORS);
      // Write them to successive EEPROM addresses
      EEPROM.write(EEPROM_SSID_SUFFX_0 + i, ssidSuffixIndex[i]);
    }
    // Set the flag, so we don't have to do it again.
    EEPROM.write(EEPROM_SSID_GENERATED_ADDRESS, 42);
    EEPROM.commit(); // Commit all EEPROM write's
  }
  else // Otherwise, if not new and EEPROM flag set
  {
    for (int i=0; i<SSID_SUFFIX_LENGTH; i++)
      ssidSuffixIndex[i] = EEPROM.read(EEPROM_SSID_SUFFX_0 + i);
  }
  
  strcpy(BoardSSID, SSID_PREFIX); // Copy the prefix into boardSSID
  char dash = '-';
  strncat(BoardSSID, &dash, 1); // Add a dash between BlynkMe and color code
  for (int i=0; i < SSID_SUFFIX_LENGTH; i++)
  {
    ssidSuffixIndex[i] %= WS2812_NUM_COLORS; // Just in case it was invalid
    // Add color-code character to the board SSID
    strncat(BoardSSID, &SSID_COLOR_CHAR[ssidSuffixIndex[i]], 1);
  }
  BB_DEBUG("New SSID: " + String(BoardSSID));

  if (!setupAP(BoardSSID))
    ESP.reset();
  blinkCount = 0; // Reset LED blinker count
}

void handleConfigServer(void)
{
  dnsServer.processNextRequest();
  server.handleClient();
}

void checkForStations(void)
{
  if (WiFi.softAPgetStationNum() > 0)
    runMode = MODE_CONFIG_DEVICE_CONNECTED;
  else
    runMode = MODE_CONFIG;
}

bool SerialWiFiScan(void)
{
  int n = WiFi.scanNetworks();
  if (n != 0)
  {
    char index;
    Serial.println("Scan found " + String(n) + " networks:");
    Serial.println("0: Not listed (hidden network)");
    for (int i=0; i<n; ++i)
    {
      if (i <= 8)
        index = '1' + i;
      else
        index = 'a' + (i - 9);
      Serial.print(String(index) + ": ");
      Serial.println(WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + ")");
    }
    Serial.print("\r\nPress 1-" + String(index) + " to select from the list above.\r\n");
    Serial.print("Press 0 if your network is hidden or not listed.\r\n> ");
    
    return true;
  }
  else
  {
    Serial.println("Didn't find any WiFi networks. Try typing 'w' to enter manually.");
    return false;
  }  
}

String serialConfigBuffer = "";
String serialConfigWiFiSSID = "";
String serialConfigWiFiPSK = "";
String serialConfigBlynkAuth = "";
String serialConfigBlynkHost = "";
uint16_t serialConfigBlynkPort = 0;

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
          case CONFIG_CHAR_WIFI_SCAN:
            Serial.println("Scanning for WiFi Networks");
            if (SerialWiFiScan())
              serialConfigMode = SERIAL_CONFIG_WIFI_SCAN;
            break;            
          case CONFIG_CHAR_WIFI_NETWORK:
            serialConfigMode = SERIAL_CONFIG_WIFI_NETWORK;
            Serial.print(SERIAL_MESSAGE_WIFI_NETWORK);
            break;
          case CONFIG_CHAR_BLYNK:
            serialConfigMode = SERIAL_CONFIG_BLYNK;
            Serial.print(SERIAL_MESSAGE_BLYNK);
            break;
          case CONFIG_CHAR_HELP:
            Serial.print(SERIAL_MESSAGE_HELP);
            break;
          }
        }
        else // If it's not a config char, and we're in waiting mode
        {
          // Ignore it (?)
        }
      }
      else if (serialConfigMode == SERIAL_CONFIG_WIFI_SCAN)
      {
        int i;
        if ((c >= '1') && (c <= '9'))
          i = (int)c - ((int)'1' - 1);
        else if ((c >= 'a') && (c <= 'z'))
          i = (int)c - (int)'a' + 10;
        else
          i = 0;
        if (i > 0)
        {
          Serial.println("Network " + String(c) + ": " + WiFi.SSID(i-1));
          serialConfigWiFiSSID = WiFi.SSID(i-1);
          serialConfigMode = SERIAL_CONFIG_WIFI_PASSWORD;
          Serial.print(SERIAL_MESSAGE_WIFI_PASSWORD);
        }
        else
        {
          serialConfigMode = SERIAL_CONFIG_WIFI_NETWORK;
          Serial.print(SERIAL_MESSAGE_WIFI_NETWORK);
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
    Serial.print(SERIAL_MESSAGE_WIFI_PASSWORD);
    break;
  case SERIAL_CONFIG_WIFI_PASSWORD:
    serialConfigWiFiPSK = serialConfigBuffer;
    serialConfigMode = SERIAL_CONFIG_WAITING;
    passwordEntered = true;
    if (serialConfigBlynkAuth == "") // If blynk auth token is blank
      Serial.print("\r\nWiFi configured! Hit 'b' to configure Blynk.\r\n\r\n> ");
    break;
  case SERIAL_CONFIG_BLYNK:
    serialConfigBlynkAuth = serialConfigBuffer;
    serialConfigMode = SERIAL_CONFIG_BLYNK_HOST;
    Serial.print(SERIAL_MESSAGE_BLYNK_HOST);
    break;
  case SERIAL_CONFIG_BLYNK_HOST:
    if (serialConfigBuffer == "")
    {
      Serial.println("Using default host: " + String(BB_BLYNK_HOST_DEFAULT));
      serialConfigBlynkHost = BB_BLYNK_HOST_DEFAULT;
    }
    else
    {
      serialConfigBlynkHost = serialConfigBuffer;
    }
    serialConfigMode = SERIAL_CONFIG_BLYNK_PORT;
    Serial.print(SERIAL_MESSAGE_BLYNK_PORT);
    break;
  case SERIAL_CONFIG_BLYNK_PORT:
    if (serialConfigBuffer == "")
    {
      Serial.println("Using default port: " + String(BB_BLYNK_PORT_DEFAULT));
      serialConfigBlynkPort = BB_BLYNK_PORT_DEFAULT;
    }
    else
    {
      serialConfigBlynkPort = serialConfigBuffer.toInt();
    }
    Serial.println();
    if (serialConfigWiFiSSID == "")
      Serial.print("Blynk configured. Hit 'w' or 's' to configure WiFi.\r\n\r\n> ");
    serialConfigMode = SERIAL_CONFIG_WAITING;
    break;
  }
  if ((serialConfigWiFiSSID != "") && (passwordEntered) && (serialConfigBlynkAuth != "") &&
      (serialConfigBlynkHost != "") && (serialConfigBlynkPort != 0))
  {
    BB_DEBUG("Connecting to WiFi.");
    int8_t ret = setupBlynkStation(serialConfigWiFiSSID, serialConfigWiFiPSK, serialConfigBlynkAuth,
                                   serialConfigBlynkHost, serialConfigBlynkPort);
    if (ret < 0)
    {
      if (ret == ERROR_CONNECT_BLYNK)
      {
        BB_DEBUG("Serial error connecting to Blynk.");
        serialConfigBlynkAuth = "";
        serialConfigBlynkHost = "";
        serialConfigBlynkPort = 0;
      }
      else if (ret == ERROR_CONNECT_WIFI)
      {
        BB_DEBUG("Serial error connecting to WiFi.");
        serialConfigWiFiSSID = "";
        serialConfigWiFiPSK = "";
      }
    }
    else
    {
      g_blynkAuthStr = serialConfigBlynkAuth;
      g_blynkHostStr = serialConfigBlynkHost;
      g_blynkPort = serialConfigBlynkPort;
    }
  }
  serialConfigBuffer = "";
}
