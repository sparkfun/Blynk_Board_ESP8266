#define BLYNK_PRINT Serial // Enables Serial Monitor MUST be before #includes...
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "LocalAuthToken";       // Put your Auth Token here
char remoteAuth[] = "RemoteAuthToken"; // Auth token of device connected via bridge

// 'Global':
#define BUTTON_PIN     0  // GPIO0
#define LED_PIN        5  // GPIO5
// Local:
#define LOCAL_LED_BUTTON  0 // V0
#define REMOTE_LED_BUTTON 1 // V1
#define LOCAL_TERMINAL    2 // V2
#define LOCAL_BRIDGE      3 // V3
#define LOCAL_RECEIVE     4 // V4
// Remote:
#define REMOTE_LOCAL_LED_BUTTON  5  // V5
#define REMOTE_REMOTE_LED_BUTTON 6  // V6
#define REMOTE_TERMINAL          7  // V7
#define REMOTE_BRIDGE            8  // V8
#define REMOTE_RECEIVE           9  // V9

bool wasButtonPressed = false;
bool remoteLedState = LOW;

// Attach virtual serial terminal to LOCAL_TERMINAL Virtual Pin
WidgetTerminal terminal(LOCAL_TERMINAL);

// Configure bridge on LOCAL_BRIDGE virtual pin
WidgetBridge bridge(LOCAL_BRIDGE);

void setup()
{
  Serial.begin(9600); // See the connection status in Serial Monitor
  Blynk.begin(auth, "SSID", "PASSWORD");  // Here your Arduino connects to the Blynk Cloud.

  // Wait until connected
  while (Blynk.connect() == false);
  
  bridge.setAuthToken(remoteAuth);

  // Setup the onboard button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Attach BUTTON_PIN interrupt to our handler
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, CHANGE);

  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println("-------------");
  terminal.println("Start chatting!");
  terminal.flush();
}

// Interrupt service routine to capture button press event
void onButtonPress()
{
  wasButtonPressed = !digitalRead(BUTTON_PIN); // Invert state, since button is "Active LOW"
}

// Virtual button used to change local LED
BLYNK_WRITE(LOCAL_LED_BUTTON)
{
  BLYNK_LOG("Local LED was pressed"); // This can be seen in the Serial Monitor
  digitalWrite(LED_PIN, param.asInt()); // Set state of virtual button to LED
}

// Virtual button used to change LED on bridge board
BLYNK_WRITE(REMOTE_LED_BUTTON)
{
  BLYNK_LOG("Remote LED was pressed");  // This can be seen in the Serial Monitor
  bridge.virtualWrite(REMOTE_LOCAL_LED_BUTTON, param.asInt()); // Set state of virtual button to remote LED
}

// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(LOCAL_TERMINAL)
{
  bridge.virtualWrite(REMOTE_RECEIVE, param.asStr());  // Send from this terminal to remote terminal
}

// Receive a string on LOCAL_RECEIVE and write that value to the connected phone's terminal
BLYNK_WRITE(LOCAL_RECEIVE)
{
  terminal.println(param.asStr());  // Write received string to local terminal
  terminal.flush();
}

void loop()
{
  Blynk.run(); // All the Blynk Magic happens here...
  
  if (wasButtonPressed) {
    BLYNK_LOG("Physical button was pressed.");              // This can be seen in the Serial Monitor
    remoteLedState ^= HIGH;                                 // Toggle state
    bridge.digitalWrite(LED_PIN, remoteLedState);           // Send new state to remote board
    Blynk.virtualWrite(REMOTE_LED_BUTTON, remoteLedState);  // Update state of button on mobile app
    wasButtonPressed = false;                               // Clear state variable set in ISR
  }
}

