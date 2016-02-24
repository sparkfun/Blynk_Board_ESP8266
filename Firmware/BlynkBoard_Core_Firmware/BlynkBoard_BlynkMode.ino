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
HTU21D thSense;

/****************************************** 
 *********************************************/
#define RED_VIRTUAL V0
#define GREEN_VIRTUAL V1
#define BLUE_VIRTUAL V2
#define BUTTON_VIRTUAL V3
#define TEMPERATURE_VIRTUAL V4
#define HUMIDITY_VIRTUAL V5
#define SERIAL_VIRTUAL V6
#define ADC_BATT_VIRTUAL V7

#define RESET_VIRTUAL V31

WidgetLED buttonLED(BUTTON_VIRTUAL);
WidgetTerminal terminal(SERIAL_VIRTUAL);

byte red = 0;
byte green = 0;
byte blue = 0;

#define BUTTON_UPDATE_RATE 250
unsigned long lastButtonUpdate = 0;
void buttonUpdate(void);

#define TH_UPDATE_RATE 1000
unsigned long lastTHUpdate = 0;
void thUpdate(void);

#define ADC_UPDATE_RATE 60000
unsigned long lastADCupdate = 0;
void adcUpdate(void);

void blynkSetup(void)
{
  runMode = MODE_BLYNK_RUN;
  detachInterrupt(BUTTON_PIN);
  thSense.begin();
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
    String email = incoming.substring(1, incoming.length());
    for (int i=0; i<email.length(); i++)
    {
      if (email.charAt(i) == ' ') //! TODO check for ANY invalid character
        email.remove(i, 1);
    }
    terminal.println("Your email is:" + email + ".");
    terminal.flush();
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

  if (Serial.available())
  {
    String toSend;
    while (Serial.available())
      toSend += (char)Serial.read();
    terminal.print(toSend);
    terminal.flush();
  }
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
  float temp = thSense.readTemperature();
  temp = temp * 9.0 / 5.0 + 32.0;
  Blynk.virtualWrite(TEMPERATURE_VIRTUAL, String(temp, 1) + "F");
  Blynk.virtualWrite(HUMIDITY_VIRTUAL, String(humd, 1) + "%");
}
