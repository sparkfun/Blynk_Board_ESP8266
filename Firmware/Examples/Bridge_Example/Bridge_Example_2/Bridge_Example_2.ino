/******************************************************************************
Bridge_Example_2.ino
BlynkBoard Firmware: Blynk Demo Source
Brent Wilkins @ SparkFun Electronics
March 11, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware
This file, part of the BlynkBoard Firmware, implements all "Blynk Mode"
functions of the BlynkBoard Core Firmware. That includes managing the Blynk
connection and ~10 example experiments, which can be conducted without
reprogramming the Blynk Board.
Resources:
Blynk Arduino Library: https://github.com/blynkkk/blynk-library/releases/tag/v0.3.3
License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.
Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
******************************************************************************/

// Comment next line out to disable serial prints and save space
#define BLYNK_PRINT Serial // Enables Serial Monitor MUST be before #includes...
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "LocalAuthToken";        // Put your Auth Token here
char remoteAuth[] = "RemoteAuthToken"; // Auth token of bridged device

// Physical Pins:
#define BUTTON_PIN 0  // GPIO0
#define LED_PIN    5  // GPIO5

// Virtual Pins:
#define TERMINAL                 0  // V0
#define LOCAL_LED_BUTTON         1  // V1
#define REMOTE_LED_BUTTON        2  // V2
#define TERMINAL_RECEIVE         3  // V3
#define BRIDGE                   4  // V4
#define LOCAL_LED_RECIEVE        5  // V5
#define REMOTE_LED_STATUS_UPDATE 6  // V6

volatile bool wasButtonPressed = false;
bool ledState = LOW;
bool remoteLedState = LOW;

// Attach virtual serial terminal to TERMINAL Virtual Pin
WidgetTerminal terminal(TERMINAL);

// Configure bridge on LOCAL_BRIDGE virtual pin
WidgetBridge bridge(BRIDGE);

void setup()
{
  Serial.begin(9600); // See the connection status in Serial Monitor

  // Here your Arduino connects to the Blynk Cloud.
  Blynk.begin(auth, "SSID", "PASSWORD");

  // TODO: Set defaults after weird state changes
  // Wait until connected
  while (Blynk.connect() == false);
  
  bridge.setAuthToken(remoteAuth);

  // Setup the onboard button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Attach BUTTON_PIN interrupt to our handler
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  // Setup the onboard blue LED
  pinMode(LED_PIN, OUTPUT);

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
  // Invert state, since button is "Active LOW"
  wasButtonPressed = !digitalRead(BUTTON_PIN);
}

// Virtual button on connected app was used to change local LED.
// Update LED, and tell remote devices about it.
BLYNK_WRITE(LOCAL_LED_BUTTON)
{
  ledState = param.asInt();           // Update local state to match app state
  BLYNK_LOG("LED state is now %s", ledState ? "HIGH" : "LOW");
  digitalWrite(LED_PIN, ledState);    // Set state of virtual button to LED
  // Send updated status to remote board.
  bridge.virtualWrite(REMOTE_LED_STATUS_UPDATE, ledState);
}

// Virtual button on connected app was used to change remote LED.
// Tell remote devices about the change.
BLYNK_WRITE(REMOTE_LED_BUTTON)
{
  remoteLedState = param.asInt(); // Update state with info from remote
  BLYNK_LOG("New remote LED state: %s", param.asInt() ? "HIGH" : "LOW");
  // Send updated status to remote board.
  bridge.virtualWrite(LOCAL_LED_RECIEVE, remoteLedState);
}

// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(TERMINAL)
{
  // Send from this terminal to remote terminal
  bridge.virtualWrite(TERMINAL_RECEIVE, param.asStr());
}

// Receive a string on LOCAL_RECEIVE and write that value to the connected
// phone's terminal
BLYNK_WRITE(TERMINAL_RECEIVE)
{
  terminal.println(param.asStr());  // Write received string to local terminal
  terminal.flush();
}

// Remote device triggered an LED change. Update the LED and app status.
BLYNK_WRITE(LOCAL_LED_RECIEVE)
{
  // This can be seen in the Serial Monitor
  BLYNK_LOG("Remote device triggered LED change");

  ledState = param.asInt();
  // Turn on LED
  digitalWrite(LED_PIN, ledState);
  // Set state of virtual button to match remote LED
  Blynk.virtualWrite(LOCAL_LED_BUTTON, ledState);
}

// Remote LED status changed. Show this in the app.
BLYNK_WRITE(REMOTE_LED_STATUS_UPDATE)
{
  remoteLedState = param.asInt();
  Blynk.virtualWrite(REMOTE_LED_BUTTON, remoteLedState);
}

void loop()
{
  Blynk.run(); // All the Blynk Magic happens here...

  if (wasButtonPressed) {
    // This can be seen in the Serial Monitor
    BLYNK_LOG("Physical button was pressed.");
    // Toggle state
    remoteLedState ^= HIGH;
    // Send new state to remote board LED
    bridge.virtualWrite(LOCAL_LED_RECIEVE, remoteLedState);
    // Update state of button on local mobile app
    Blynk.virtualWrite(REMOTE_LED_BUTTON, remoteLedState);
    // Clear state variable set in ISR
    wasButtonPressed = false;
  }
}
