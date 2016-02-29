/******************************************************************************
BlynkBoard_BlynkMode.ino
BlynkBoard Firmware: Blynk Demo Source
Jim Lindblom @ SparkFun Electronics
February 24, 2016
https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware

This file, part of the BlynkBoard Firmware, implements all "Blynk Mode"
functions of the BlynkBoard Core Firmware. That includes managing the Blynk
connection and ~10 example experiments, which can be conducted without
reprogramming the Blynk Board.

 Supported Experiment list:
 1: RGB LED - ZeRGBa (V0, V1, V2)
 2: 5 LED - Button (D5)
 3: 0 Button - Virtual LED (V3)
      2~: 0 Button - tweet, would need to modify button interrupt handler
 4: Humidity and temperature to <strike>gauge</strike> value displays
 5: RGB control from <strike>Zebra widet</strike> Sliders
 6. Incoming serial on hardware port sends to terminal in Blynk
 7. ADC hooked to VIN pin on graph
 8. TODO: Alligator clips connected to GPIO 12(?) send an email
 9. TODO: Timer triggers relay
 10. TODO: IMU graphs to phone 

Resources:
Blynk Arduino Library: https://github.com/blynkkk/blynk-library/releases/tag/v0.3.1
SparkFun HTU21D Library: https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library/releases/tag/V_1.1.1

License:
This is released under the MIT license (http://opensource.org/licenses/MIT).
Please see the included LICENSE.md for more information.

Development environment specifics:
Arduino IDE 1.6.7
SparkFun BlynkBoard - ESP8266
******************************************************************************/

#include <Wire.h>
#include "SparkFunHTU21D.h"
#include <SparkFun_MMA8452Q.h>
#include <SparkFunTSL2561.h>
#include "Servo.h"
HTU21D thSense;

#define BLYNK_BOARD_SDA 2
#define BLYNK_BOARD_SCL 14
void initializeVirtualVariables(void);
void accelUpdate(void);
bool scanI2C(uint8_t address);
void luxInit(void);
void luxUpdate(void);
void doorSwitchUpdate(void);
void twitterUpdate(void);
void emailUpdate(void);

/****************************************** 
 *********************************************/
#define RED_VIRTUAL V0
#define GREEN_VIRTUAL V1
#define BLUE_VIRTUAL V2
#define BUTTON_VIRTUAL V3
#define TEMPERATURE_F_VIRTUAL V4
#define HUMIDITY_VIRTUAL V5
#define TEMPERATURE_C_VIRTUAL V6
#define ADC_BATT_VIRTUAL V7
#define PUSH_ENABLE_VIRTUAL V8
#define SI7021_TERMINAL_LCD V9
#define TWEET_ENABLE_VIRTUAL V10
#define SERVO_X_VIRTUAL V11
#define SERVO_Y_VIRTUAL V12
#define SERVO_MAX_VIRTUAL V13
#define SERVO_ANGLE_VIRUTAL V14
#define SERIAL_VIRTUAL V15
#define LUX_VIRTUAL V16
#define LUX_RATE_VIRTUAL V17
#define DOOR_STATE_VIRTUAL V18
#define TWITTER_THRESHOLD_VIRTUAL V19
#define TWITTER_RATE_VIRTUAL V20
#define EMAIL_ENABLED_VIRTUAL V21

#define RESET_VIRTUAL V31

WidgetLED buttonLED(BUTTON_VIRTUAL);
WidgetTerminal terminal(SERIAL_VIRTUAL);
WidgetLCD si7021LCD(SI7021_TERMINAL_LCD);

bool blynkVirtualsInitialized = false;

byte red = 0;
byte green = 0;
byte blue = 0;

#define BUTTON_UPDATE_RATE 500
unsigned long lastButtonUpdate = 0;
void buttonUpdate(void);

#define TH_UPDATE_RATE 2000
unsigned long lastTHUpdate = 0;
void thUpdate(void);

#define ADC_UPDATE_RATE 60000
unsigned long lastADCupdate = 0;
void adcUpdate(void);

#define SERVO_PIN 15
#define SERVO_MINIMUM 5
unsigned int servoMax = 180;
int servoX = 0;
int servoY = 0;
Servo myServo;

MMA8452Q mma;
#define MMA8452Q_ADDRESS 0x1D
bool accelPresent = false;
#define ACCEL_UPDATE_RATE 1000
unsigned long lastAccelupdate = 0;

#define LUX_ADDRESS 0x39
bool luxPresent = false;
bool luxInitialized = false;
unsigned int luxUpdateRate = 1000;
unsigned int ms = 1000;  // Integration ("shutter") time in milliseconds
unsigned long lastLuxUpdate = 0;
SFE_TSL2561 light;
boolean gain = 0;

#define DOOR_SWITCH_PIN 16
#define DOOR_SWITCH_UPDATE_RATE 1000
unsigned int lastDoorSwitchUpdate = 0;
bool pushEnabled = false;
uint8_t lastSwitchState;

bool tweetEnabled = false;
unsigned long tweetUpdateRate = 60000;
unsigned int lastTweetUpdate = 0;
unsigned int moistureThreshold = 0;

String emailAddress = "";
#define EMAIL_UPDATE_RATE 60000
unsigned long lastEmailUpdate = 0;

void blynkSetup(void)
{
  BB_DEBUG("Initializing Blynk Demo");
  WiFi.enableAP(false);
  runMode = MODE_BLYNK_RUN;
  detachInterrupt(BUTTON_PIN);
  thSense.begin();

  myServo.attach(SERVO_PIN);
  myServo.write(15);

  pinMode(DOOR_SWITCH_PIN, INPUT_PULLDOWN_16);
  lastSwitchState = digitalRead(DOOR_SWITCH_PIN);

  if (scanI2C(LUX_ADDRESS))
  {
    BB_DEBUG("Luminosity sensor connected.");
    luxInit();
  }
  else
  {
    BB_DEBUG("No lux sensor.");
  }
  
  if (mma.init())
  {
    BB_DEBUG("MMA8452Q connected.");
    accelPresent = true;
  }
  else
  {
    BB_DEBUG("MMA8452Q not found.");
    accelPresent = false;
  }
}

BLYNK_WRITE(RED_VIRTUAL)
{
  blinker.detach();
  red = param.asInt();
  rgb.setPixelColor(0, rgb.Color(red, green, blue));
  rgb.show();
}

BLYNK_WRITE(GREEN_VIRTUAL)
{
  blinker.detach();
  green = param.asInt();
  rgb.setPixelColor(0, rgb.Color(red, green, blue));
  rgb.show();
}

BLYNK_WRITE(BLUE_VIRTUAL)
{
  blinker.detach();
  blue = param.asInt();
  rgb.setPixelColor(0, rgb.Color(red, green, blue));
  rgb.show();
}

BLYNK_WRITE(SERIAL_VIRTUAL)
{
  String incoming = param.asStr();
  BB_DEBUG("Serial: " + incoming);
  if (incoming.charAt(0) == '!')
  {
    String emailAdd = incoming.substring(1, incoming.length());
    for (int i=0; i<emailAdd.length(); i++)
    {
      if (emailAdd.charAt(i) == ' ') //! TODO check for ANY invalid character
        emailAdd.remove(i, 1);
    }
    //! TODO: Check if valid email - look for @, etc.
    terminal.println("Your email is:" + emailAdd + ".");
    emailAddress = emailAdd;
    terminal.flush();
  }
}

BLYNK_WRITE(SERVO_X_VIRTUAL)
{
  servoX = param.asInt() - 128;
  float pos = atan2(servoY, servoX) * 180.0 / PI;
  if (pos < 0)
    pos = 360.0 + pos;
  Blynk.virtualWrite(SERVO_ANGLE_VIRUTAL, pos);
  int servoPos = map(pos, 0, 360, SERVO_MINIMUM, servoMax);
  myServo.write(servoPos);
  Serial.println("Pos: " + String(pos));
  Serial.println("ServoPos: " + String(servoPos));
}

BLYNK_WRITE(SERVO_Y_VIRTUAL)
{
  servoY = param.asInt() - 128;
  float pos = atan2(servoY, servoX) * 180.0 / PI;
  if (pos < 0)
    pos = 360.0 + pos;
  Blynk.virtualWrite(SERVO_ANGLE_VIRUTAL, pos);
  int servoPos = map(pos, 0, 360, SERVO_MINIMUM, servoMax);
  myServo.write(servoPos);
  Serial.println("Pos: " + String(pos));
  Serial.println("ServoPos: " + String(servoPos));
}

BLYNK_WRITE(SERVO_MAX_VIRTUAL)
{
  int sMax = param.asInt();
  servoMax = constrain(sMax, SERVO_MINIMUM + 1, 360);
  Serial.println("ServoMax: " + String(servoMax));
}

BLYNK_WRITE(LUX_RATE_VIRTUAL)
{
  luxUpdateRate = param.asInt() * 1000;
  if (luxUpdateRate < 1000) luxUpdateRate = 1000;
}

BLYNK_WRITE(PUSH_ENABLE_VIRTUAL)
{
  uint8_t state = param.asInt();
  if (state)
  {
    pushEnabled = true;
    BB_DEBUG("Push enabled.");
  }
  else
  {
    pushEnabled = false;
    BB_DEBUG("Push disabled.");    
  }
}

BLYNK_WRITE(TWEET_ENABLE_VIRTUAL)
{
  uint8_t state = param.asInt();
  if (state)
  {
    tweetEnabled = true;
    BB_DEBUG("Tweet enabled.");
  }
  else
  {
    tweetEnabled = false;
    BB_DEBUG("Tweet disabled.");    
  }
}

BLYNK_WRITE(TWITTER_THRESHOLD_VIRTUAL)
{
  moistureThreshold = param.asInt();
  BB_DEBUG("Tweet threshold set to: " + String(moistureThreshold));
}

BLYNK_WRITE(TWITTER_RATE_VIRTUAL)
{
  int tweetRate = param.asInt();
  if (tweetRate <= 0) tweetRate = 1;
  BB_DEBUG("Setting tweet rate to " + String(tweetRate) + " minutes");
  tweetUpdateRate = tweetRate * 60 * 1000;
}

BLYNK_WRITE(EMAIL_ENABLED_VIRTUAL)
{
  if (param.asInt())
  {
    if (emailAddress != "")
    {
      if ((lastEmailUpdate == 0) || (lastEmailUpdate + EMAIL_UPDATE_RATE < millis()))
      {
        emailUpdate();
        lastEmailUpdate = millis();
      }
      else
      {
        int waitTime = (lastEmailUpdate + EMAIL_UPDATE_RATE) - millis();
        waitTime /= 1000;
        terminal.println("Please wait " + String(waitTime) + " seconds");
        terminal.flush();
      }
    }
    else
    {
      terminal.println("Type !email@address.com to set the email address");
      terminal.flush();
    }
  }
}

BLYNK_WRITE(RESET_VIRTUAL)
{
  BB_DEBUG("Factory resetting");
  resetEEPROM();
  ESP.reset();
}

void blynkLoop(void)
{
  Blynk.run();
  if (Blynk.connected() && !blynkVirtualsInitialized)
  {
    initializeVirtualVariables();
    blynkVirtualsInitialized = true;
  }
  
  if (lastButtonUpdate + BUTTON_UPDATE_RATE < millis())
  {
    buttonUpdate();
    lastButtonUpdate = millis();
  }
  
  if (lastTHUpdate + TH_UPDATE_RATE < millis())
  {
    thUpdate();
    lastTHUpdate = millis();
  }

  if (lastADCupdate + ADC_UPDATE_RATE < millis())
  {
    adcUpdate();
    lastADCupdate = millis(); 
  }

  if (lastLuxUpdate + luxUpdateRate < millis())
  {
    if (luxInitialized)
      luxUpdate();
    else
      Blynk.virtualWrite(LUX_VIRTUAL, analogRead(A0));
    lastLuxUpdate = millis();
  }

  if (lastDoorSwitchUpdate + DOOR_SWITCH_UPDATE_RATE < millis())
  {
    doorSwitchUpdate();
    lastDoorSwitchUpdate = millis();
  }

  if (tweetEnabled && (lastTweetUpdate + tweetUpdateRate < millis()))
  {
    twitterUpdate();
    lastTweetUpdate = millis();
  }

  if (Serial.available())
  {
    String toSend;
    while (Serial.available())
      toSend += (char)Serial.read();
    terminal.print(toSend);
    terminal.flush();
  }
}

void initializeVirtualVariables(void)
{
  BB_DEBUG("Initializing Blynk variables.");
  //! TODO: Initialize all necessary variables here
  
  // Initialize Twitter variables:
  Blynk.virtualWrite(TWEET_ENABLE_VIRTUAL, tweetEnabled);
  Blynk.virtualWrite(TWITTER_RATE_VIRTUAL, (tweetUpdateRate / 60 / 1000));
  Blynk.virtualWrite(TWITTER_THRESHOLD_VIRTUAL, moistureThreshold);
}

void adcUpdate(void)
{
  int rawADC = analogRead(A0);
  float voltage = ((float) rawADC / 1024.0) * 32.0 / 10.0 * 2.0;

  Blynk.virtualWrite(ADC_BATT_VIRTUAL, voltage);
}

void buttonUpdate(void)
{
  if (digitalRead(BUTTON_PIN))
    buttonLED.off();
  else
    buttonLED.on();
}

void thUpdate(void)
{
  float humd = thSense.readHumidity();
  float tempC = thSense.readTemperature();
  float tempF = tempC * 9.0 / 5.0 + 32.0;
  Blynk.virtualWrite(TEMPERATURE_C_VIRTUAL, String(tempC, 1));// + "F");
  Blynk.virtualWrite(TEMPERATURE_F_VIRTUAL, String(tempF, 1));// + "F");
  Blynk.virtualWrite(HUMIDITY_VIRTUAL, String(humd, 1));// + "%");
  si7021LCD.clear();
  si7021LCD.print(0, 0, String(tempF, 2) + "F / "+ String(tempC, 2) + "C");
  si7021LCD.print(0, 1, "Humidity: " + String(humd, 1) + "%");
}

void accelUpdate(void)
{
  if (mma.available())
  {
    mma.read();
    Serial.print(mma.cx, 3);
    Serial.print("\t");
    Serial.print(mma.cy, 3);
    Serial.print("\t");
    Serial.println(mma.cz, 3);
  }  
}

void luxInit(void)
{
  // Initialize the SFE_TSL2561 library
  // You can pass nothing to light.begin() for the default I2C address (0x39)
  light.begin();
  
  // If gain = false (0), device is set to low gain (1X)
  gain = 0;
  
  unsigned char time = 0;
  // setTiming() will set the third parameter (ms) to the 13.7ms
  light.setTiming(gain,time,ms);

  // To start taking measurements, power up the sensor:
  light.setPowerUp();
  
  luxInitialized = true;
}

void luxUpdate(void)
{
  // This sketch uses the TSL2561's built-in integration timer.
  delay(ms);
  
  // Once integration is complete, we'll retrieve the data.
  unsigned int data0, data1;
  
  if (light.getData(data0,data1))
  {
    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated
    good = light.getLux(gain,ms,data0,data1,lux);
    BB_DEBUG("Lux: " + String(lux));
    Blynk.virtualWrite(LUX_VIRTUAL, lux);
  }
}

void doorSwitchUpdate(void)
{
  uint8_t switchState = digitalRead(DOOR_SWITCH_PIN);
  if (lastSwitchState != switchState)
  {
    if (switchState)
    {
      BB_DEBUG("Door switch closed.");  
      Blynk.virtualWrite(DOOR_STATE_VIRTUAL, "Close");
      if (pushEnabled)
      {
        BB_DEBUG("Notified closed.");  
        Blynk.notify("[" + String(millis()) + "]: Door closed");
      }
    }
    else
    {
      BB_DEBUG("Door switch opened.");    
      Blynk.virtualWrite(DOOR_STATE_VIRTUAL, "Open");
      if (pushEnabled)
      {
        BB_DEBUG("Notified opened.");  
        Blynk.notify("[" + String(millis()) + "]: Door opened");          
      }
    }
    lastSwitchState = switchState;
  }
  
}

void twitterUpdate(void)
{
  unsigned int moisture = analogRead(A0);
  String msg = "~~MyBoard~~\r\nSoil Moisture reading: " + String(moisture) + "\r\n";
  if (moisture < moistureThreshold)
  {
    msg += "FEED ME!\r\n";
  }
  msg += "[" + String(millis()) + "]";
  BB_DEBUG("Tweeting: " + msg);
  Blynk.tweet(msg);  
}

void emailUpdate(void)
{
  String emailSubject = "My BlynkBoard Statistics";
  String emailMessage = "";
  emailMessage += "D0: " + String(digitalRead(0)) + "\r\n";
  emailMessage += "D16: " + String(digitalRead(16)) + "\r\n";
  emailMessage += "\r\n";
  emailMessage += "A0: " + String(analogRead(A0)) + "\r\n";
  emailMessage += "\r\n";
  emailMessage += "Temp: " + String(thSense.readTemperature()) + "C\r\n";
  emailMessage += "Humidity: " + String(thSense.readHumidity()) + "%\r\n";
  emailMessage += "\r\n";
  emailMessage += "Runtime: " + String(millis() / 1000) + "s\r\n";

  BB_DEBUG("email: " + emailAddress);
  BB_DEBUG("subject: " + emailSubject);
  BB_DEBUG("message: " + emailMessage);
  Blynk.email(emailAddress.c_str(), emailSubject.c_str(), emailMessage.c_str());
  terminal.println("Sent an email to " + emailAddress);
  terminal.flush();
}

bool scanI2C(uint8_t address)
{
  Wire.beginTransmission(address);
  Wire.write( (byte)0x00 );
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}
