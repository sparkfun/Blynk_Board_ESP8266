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
 0: RGB LED - ZeRGBa (V0)
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
#include <SparkFunTSL2561.h>
#include "Servo.h"
HTU21D thSense;

bool scanI2C(uint8_t address);

////////////////////////////////////
// Blynk Virtual Variable Mapping //
//////////////////////////////////// // Experiment #(s)
#define RGB_VIRTUAL               V0 // 0, 6
#define BUTTON_VIRTUAL            V1 // 1
#define RED_VIRTUAL               V2 // 3
#define GREEN_VIRTUAL             V3 // 3
#define BLUE_VIRTUAL              V4 // 3
#define TEMPERATURE_F_VIRTUAL     V5 // 4
#define TEMPERATURE_C_VIRTUAL     V6 // 4
#define HUMIDITY_VIRTUAL          V7 // 4
#define ADC_VOLTAGE_VIRTUAL       V8 // 5
#define RGB_RAINBOW_VIRTUAL       V9 // 6
#define LCD_VIRTUAL               V10 // 8
#define LCD_TEMPHUMID_VIRTUAL     V11 // 8
#define LCD_STATS_VIRTUAL         V12 // 8
#define LCD_RUNTIME_VIRTUAL       V13 // 8
#define SERVO_XY_VIRTUAL          V14
//! V15 available
#define SERVO_MAX_VIRTUAL         V16 // 9
#define SERVO_ANGLE_VIRUTAL       V17 // 9
#define LUX_VIRTUAL               V18 // 10
//! V19 available
#define ADC_BATT_VIRTUAL          V20 // 11
#define SERIAL_VIRTUAL            V21 // 12, 15
#define TWEET_ENABLE_VIRTUAL      V22 // 13
#define TWITTER_THRESHOLD_VIRTUAL V23 // 13
#define TWITTER_RATE_VIRTUAL      V24 // 13
#define DOOR_STATE_VIRTUAL        V25 // 14
#define PUSH_ENABLE_VIRTUAL       V26 // 14
#define EMAIL_ENABLED_VIRTUAL     V27 // 15
#define TEMP_OFFSET_VIRTUAL       V28 // 4
#define RGB_STRIP_NUM_VIRTUAL     V29 // 6
#define RUNTIME_VIRTUAL           V30 // Utility
#define RESET_VIRTUAL             V31 // Utility

WidgetLCD thLCD(LCD_VIRTUAL); // LCD widget, updated in blynkLoop

// BLYNK_CONNECTED is called the first time the Blynk Board
// connects to Blynk. Then any time it reconnects later on.
bool firstConnect = true;
BLYNK_CONNECTED() 
{
  if (firstConnect)
  {
    // Print a message to the LCD the first time connecting.
    thLCD.print(0, 0, " SparkFun Blynk ");
    thLCD.print(0, 1, "Board FW v" + String(BLYNKBOARD_FIRMWARE_VERSION));
    
    firstConnect = false;
  }
  Blynk.syncAll(); // Sync all virtual variables
}

/* 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 0 Experiment 0: zeRGBa                0
 0 Widget(s):                          0
 0  - zeRGBa: Merge, V0                0
 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 */
bool firstRGBWrite = true; // On startup
BLYNK_WRITE(RGB_VIRTUAL)
{
  // RGB widget may send invalid buffer data. If we try to read those in
  // the ESP8266 crashes. At a minimum, for valid data, the buffer 
  // length should be >=5. ("0,0,0" ?)
  if (param.getLength() < 5)
    return;
  
  int redParam = param[0].asInt();
  int greenParam = param[1].asInt();
  int blueParam = param[2].asInt();
  BB_DEBUG("Blynk Write RGB: " + String(redParam) + ", " + 
          String(greenParam) + ", " + String(blueParam));
  // Don't update the RGB if this is the first time. syncAll
  // will call this function initially. We want to breathe
  // the status LED instead.
  if (!firstRGBWrite)
  {
    if (!rgbSetByProject) // If blinker timer is attached
    {
      blinker.detach(); // Detach it
      rgbSetByProject = true;
    }
    // Set all attached pixels (usually it'll only be 1)
    for (int i=0; i<rgb.numPixels(); i++)
      rgb.setPixelColor(i, rgb.Color(redParam, greenParam, blueParam));
    rgb.show();
  }
  else
  {
    firstRGBWrite = false;
  }
}

/* 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
 1 Experiment 1: Button                1
 1 Widget(s):                          1
 1  - Button: GP5, Push or switch      1
 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 */
// No variables required - Use GP5 directly

/* 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
 2 Experiment 2: LED                   2
 2 Widget(s):                          2
 2  - LED: V1                          2
 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 */
// To update the buttonLED widget, buttonUpdate() is called through
// a pin-change interrupt.
WidgetLED buttonLED(BUTTON_VIRTUAL); // LED widget in Blynk App
unsigned long lastButtonUpdate = 0; // millis()-tracking button update rate
#define BUTTON_UPDATE_RATE 100 // Button update rate in ms
int lastButtonState = -1; // Keeps track of last button state push

void buttonUpdate(void) 
{
  int buttonState = digitalRead(BUTTON_PIN); // Read button state
  if (buttonState != lastButtonState) // If the state has changed
  {
    lastButtonState = buttonState; // Update last state
    if (buttonState) // If the button is released (HIGH)
      buttonLED.off(); // Turn the LED off
    else // If the button is pressed (LOW)
      buttonLED.on(); // Turn the LED on
  }
}

/* 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
 3 Experiment 3: Sliders               3
 3 Widget(s):                          3
 3  - Large Slider: GP5, D5, 0-255     3
 3  - Slider: Red, V2, 0-255           3
 3  - Slider: Green, V3, 0-255         3
 3  - Slider: Blue, V4, 0-255          3
 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 */
// GP5 variables not necessary - directly controlled
byte blynkRed = 0; // Keeps track of red value
byte blynkGreen = 0; // Keeps track of green value
byte blynkBlue = 0; // Keeps track of blue value

void updateBlynkRGB(void)
{
  if (!rgbSetByProject) // If the setByProject flag isn't set
  { // The LED should still be pulsing
    blinker.detach(); // Detach the rgbBlink timer
    rgbSetByProject = true; // Set the flag
  }
  // Show the new LED color:
  setRGB(rgb.Color(blynkRed, blynkGreen, blynkBlue));  
}

BLYNK_WRITE(RED_VIRTUAL)
{
  int redIn = param.asInt(); // Read the value in
  BB_DEBUG("Blynk Write red: " + String(redIn));
  redIn = constrain(redIn, 0, 255); // Keep it between 0-255
  blynkRed = redIn; // Update the global variable
  updateBlynkRGB(); // Show the color on the LED
}

BLYNK_WRITE(GREEN_VIRTUAL)
{
  int greenIn = param.asInt(); // Read the value in
  BB_DEBUG("Blynk Write green: " + String(greenIn));
  greenIn = constrain(greenIn, 0, 255); // Keep it between 0-255
  blynkGreen = greenIn; // Update the global variable
  updateBlynkRGB(); // Show the color on the LED
}

BLYNK_WRITE(BLUE_VIRTUAL)
{
  int blueIn = param.asInt(); // Read the value in
  BB_DEBUG("Blynk Write blue: " + String(blueIn));
  blueIn = constrain(blueIn, 0, 255); // Keep it between 0-255
  blynkBlue = blueIn; // Update the global variable
  updateBlynkRGB(); // Show the color on the LED
}

/* 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4
 4 Experiment 4: Values                4
 4 Widget(s):                          4
 4  - Value: TempF, V5, 0-1023, 1 s    4
 4  - Value: TempC, V6, 0-1023, 1 s    4
 4  - Value: Humidity, V7, 0-1023, 1 s 4
 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 */
// Value ranges are ignored once values are written
// Board runs hot, subtract an offset to try to compensate:
float tempCOffset = 0; //-8.33;
BLYNK_READ(TEMPERATURE_F_VIRTUAL)
{
  float tempC = thSense.readTemperature(); // Read from the temperature sensor
  tempC += tempCOffset; // Add any offset
  float tempF = tempC * 9.0 / 5.0 + 32.0; // Convert to farenheit
  // Create a formatted string with 1 decimal point:
  Blynk.virtualWrite(TEMPERATURE_F_VIRTUAL, tempF); // Update Blynk virtual value
  BB_DEBUG("Blynk Read TempF: " + String(tempF));
}

BLYNK_READ(TEMPERATURE_C_VIRTUAL)
{
  float tempC = thSense.readTemperature(); // Read from the temperature sensor
  tempC += tempCOffset; // Add any offset
  Blynk.virtualWrite(TEMPERATURE_C_VIRTUAL, tempC); // Update Blynk virtual value
  BB_DEBUG("Blynk Read TempC: " + String(tempC));
}

BLYNK_READ(HUMIDITY_VIRTUAL)
{
  float humidity = thSense.readHumidity(); // Read from humidity sensor
  Blynk.virtualWrite(HUMIDITY_VIRTUAL, humidity); // Update Blynk virtual value
  BB_DEBUG("Blynk Read Humidity: " + String(humidity));
}

BLYNK_WRITE(TEMP_OFFSET_VIRTUAL) // Very optional virtual to set the tempC offset
{
  tempCOffset = param.asInt();
  BB_DEBUG("Blynk TempC Offset: " + String(tempCOffset));
}

/* 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5
 5 Experiment 5: Gauge           5
 5 Widget(s):                    5
 5  - Gauge: ADC0, 0-1023, 1 sec 5
 5  - Gauge: V8, 0-4, 1 sec      5
 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 */
// ADC is read directly to a gauge
// Voltage is read to V8
BLYNK_READ(ADC_VOLTAGE_VIRTUAL)
{
  float adcRaw = analogRead(A0); // Read in A0
  // Divide by 1024, then multiply by the hard-wired voltage divider max (3.2)
  float voltage = ((float)adcRaw / 1024.0) * ADC_VOLTAGE_DIVIDER;
  Blynk.virtualWrite(ADC_VOLTAGE_VIRTUAL, voltage); // Output value to Blynk
  BB_DEBUG("Blynk DC Voltage: " + String(voltage));
}

/* 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6
 6 Experiment 6: zeRGBa                6
 6 Widget(s):                          6
 6  - zeRGBa: Merge, V0                6
 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 */
Ticker rgbRainbowTicker; // Timer to set RGB rainbow mode

void rgbRainbow(void)
{
  static byte rainbowCounter = 0; // cycles from 0-255
  // From Wheel function in Adafruit_Neopixel strand example:
  uint32_t rainbowColor;
  for (int i=0; i<rgb.numPixels(); i++) // numPixels may just be 1
  {
    byte colorPos = i + rainbowCounter; 
    if (colorPos < 85)
    {
      rainbowColor = rgb.Color(255 - colorPos * 3, 0, colorPos * 3);
    }
    else if (colorPos < 170)
    {
      colorPos -= 85;
      rainbowColor = rgb.Color(0, colorPos * 3, 255 - colorPos * 3);
    }
    else
    {
      colorPos -= 170;
      rainbowColor = rgb.Color(colorPos * 3, 255 - colorPos * 3, 0);
    }
    
    rgb.setPixelColor(i, rainbowColor); // Set the pixel color
  }
  rgb.show(); // Actually set the LED
  rainbowCounter++; // IncremenBUTTONt counter, may roll over to 0
}

BLYNK_WRITE(RGB_RAINBOW_VIRTUAL)
{
  int rainbowState = param.asInt();
  BB_DEBUG("Rainbow Write: " + String(rainbowState));
  if (rainbowState) // If parameter is 1
  {
    blinker.detach(); // Detactch the status LED 
    rgbRainbowTicker.attach_ms(20, rgbRainbow);
    rgbSetByProject = true;
  }
  else
  {
    rgbRainbowTicker.detach();
    for (int i=0; i<rgb.numPixels(); i++)
      rgb.setPixelColor(i, 0);
    rgb.show();
    blinker.attach_ms(1, blinkRGBTimer);
    rgbSetByProject = false;
  }
}

BLYNK_WRITE(RGB_STRIP_NUM_VIRTUAL)
{
  int ledCount = param.asInt();
  if (ledCount <= 0) ledCount = 1;
  BB_DEBUG("RGB Strip length: " + String(ledCount));
  rgb.updateLength(ledCount);
}

/* 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7
 7 Experiment 7: Timer                 7
 7 Widget(s):                          7
 7  - Timer: Relay, GP12, Start, Stop  7
 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 */
// No functions or variables required

/* 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8
 8 Experiment 8: LCD                   8
 8 Widget(s):                          8
 8  - LCD: Advanced, V10               8
 8  - Button: TempHumid, V11           8
 8  - Button: Stats, V12               8
 8  - Button: Runtime, V13             8
 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 */
BLYNK_WRITE(LCD_TEMPHUMID_VIRTUAL)
{
  if (param.asInt() > 0)
  {
    float humd = thSense.readHumidity(); // Read humidity
    float tempC = thSense.readTemperature(); // Read temperature
    float tempF = tempC * 9.0 / 5.0 + 32.0; // Calculate farenheit
    String tempLine = String(tempF, 2) + "F / "+ String(tempC, 2) + "C";
    String humidityLine = "Humidity: " + String(humd, 1) + "%";
    thLCD.clear(); // Clear the LCD
    thLCD.print(0, 0, tempLine.c_str());
    thLCD.print(0, 1, humidityLine.c_str());
  }  
}
BLYNK_WRITE(LCD_STATS_VIRTUAL)
{
  if (param.asInt() > 0)
  {
    String firstLine = "GP0: ";
    if (digitalRead(BUTTON_PIN))
      firstLine += "HIGH";
    else
      firstLine += "LOW";
    String secondLine = "ADC0: " + String(analogRead(A0));
    thLCD.clear(); // Clear the LCD
    thLCD.print(0, 0, firstLine.c_str());
    thLCD.print(0, 1, secondLine.c_str());
  }
}

BLYNK_WRITE(LCD_RUNTIME_VIRTUAL)
{
  if (param.asInt() > 0)
  {
    String topLine = "RunTime-HH:MM:SS";
    String botLine = "    ";
    float seconds, minutes, hours;
    // Calculate seconds, minutes, hours elapsed, based on millis
    seconds = (float)millis() / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    seconds = (int)seconds % 60;
    minutes = (int)minutes % 60;
    // Construct a string indicating run time
    if (hours < 10) botLine += "0"; // Add the leading 0
    botLine += String((int)hours) + ":";
    if (minutes < 10) botLine += "0"; // Add the leading 0
    botLine += String((int)minutes) + ":";
    if (seconds < 10) botLine += "0"; // Add the leading 0
    botLine += String((int)seconds);
    
    thLCD.clear(); // Clear the LCD
    thLCD.print(0, 0, topLine.c_str()); // Print top line
    thLCD.print(0, 1, botLine.c_str()); // Print bottom line
  }
}

/* 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9
 9 Experiment 9: Joystick              9
 9 Widget(s):                          9
 9  - Joystick: ServoPos, V14, 0-255,  9
 9     V15, 0-255, Off, Off            9
 9  - Slider: ServoMax, V16, 0-360     9
 9  - Gauge: ServoAngle, V17, 0-360    9
 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 */

#define SERVO_PIN 15 // Servo attached to pin 15
#define SERVO_MINIMUM 5 // Minimum servo angle
unsigned int servoMax = 180; // Default maximum servo angle
int servoX = 0; // Servo angle x component
int servoY = 0; // Servo angle y component
Servo myServo; // Servo object
bool firstServoRun = true;

BLYNK_WRITE(SERVO_XY_VIRTUAL)
{
  // If it's the first time the servo is set, turn it on
  if (firstServoRun)
  {
    myServo.attach(SERVO_PIN);
    myServo.write(15);
    firstServoRun = false;
  }

  // Read in the servo value:
  int servoXIn = param[0].asInt();
  int servoYIn = param[1].asInt();
  BB_DEBUG("Servo X: " + String(servoXIn));
  BB_DEBUG("Servo Y: " + String(servoYIn));
  servoX = servoXIn - 128; // Center xIn around 0 (+/-128)
  servoY = servoYIn - 128; // Center xIn around 0 (+/-128)

  // Calculate the angle, given x and y components:
  float pos = atan2(servoY, servoX) * 180.0 / PI; // Convert to degrees
  // atan2 will give us an angle +/-180
  if (pos < 0) // Convert it to 0-360:
    pos = 360.0 + pos;

  int servoPos = map(pos, 0, 360, SERVO_MINIMUM, servoMax);
  
  // Write the newly calculated angle to a virtual variable:
  Blynk.virtualWrite(SERVO_ANGLE_VIRUTAL, servoPos);

  // Constrain the angle between min/max:
  myServo.write(servoPos); // And set the servo position
}

BLYNK_WRITE(SERVO_MAX_VIRTUAL)
{
  int sMax = param.asInt();
  BB_DEBUG("Servo Max: " + String(sMax));
  servoMax = constrain(sMax, SERVO_MINIMUM + 1, 360);
  Serial.println("ServoMax: " + String(servoMax));
}

/* 10 10 10 10 10 10 10 10 10 10 10 10 10
 10 Experiment 10: Graph               10
 10 Widget(s):                         10
 10  - Graph: Lux, V18, 0-1023,        10
 10    5s, Line                        10
 10  - Slider: UpdateRate, V19, 0-???  10
 10 10 10 10 10 10 10 10 10 10 10 10 10 */

// TSL2561 Definitions:
#define LUX_ADDRESS 0x39 // I2C address of Lux sensor
bool luxInitialized = false; // Lux sensor initialized flag
unsigned int ms = 1000;  // Integration time (ms)
boolean gain = 0; // Lux sensor gain setting
SFE_TSL2561 light; // Lux sensor object

#define LUX_UPDATE_RATE 250 // Lux update rate (ms)
unsigned long lastLuxUpdate = 0; // Last lux update (ms)

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
  if (luxInitialized)
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
  else
  {
    int lightADC = analogRead(A0);
    Blynk.virtualWrite(LUX_VIRTUAL, lightADC);
  }
}

/* 11 11 11 11 11 11 11 11 11 11 11 11 11
 11 Experiment 11: History Graph       11
 11 Widget(s):                         11
 11  - History Graph: V20, Voltage     11
 11  - Value: Voltage, V20, 1sec       11
 11 11 11 11 11 11 11 11 11 11 11 11 11 */
BLYNK_READ(ADC_BATT_VIRTUAL)
{
  int rawADC = analogRead(A0);
  float voltage = ((float) rawADC / 1024.0) * ADC_VOLTAGE_DIVIDER;
  voltage *= 2.0; // Assume dividing VIN by two with another divider

  Blynk.virtualWrite(ADC_BATT_VIRTUAL, voltage);
}

/* 12 12 12 12 12 12 12 12 12 12 12 12 12
 12 Experiment 12: Terminal            12
 12 Widget(s):                         12
 12  - Terminal: V21, On, On           12
 12 12 12 12 12 12 12 12 12 12 12 12 12 */
String emailAddress = "";
String boardName = "BlynkMe";

WidgetTerminal terminal(SERIAL_VIRTUAL);

BLYNK_WRITE(SERIAL_VIRTUAL)
{
  String incoming = param.asStr();
  Serial.println(incoming);
  
  if (incoming.charAt(0) == '!')
  {
    String emailAdd = incoming.substring(1, incoming.length());
    for (int i=0; i<emailAdd.length(); i++)
    {
      if (emailAdd.charAt(i) == ' ')
        emailAdd.remove(i, 1);
    }
    terminal.println("Your email is:" + emailAdd + ".");
    emailAddress = emailAdd;
    terminal.flush();
  }
  if (incoming.charAt(0) == '$')
  {
    String newName = incoming.substring(1, incoming.length());

    boardName = newName;
    terminal.println("Board name set to: " + boardName + ".");
    terminal.flush();
  }
}

/* 13 13 13 13 13 13 13 13 13 13 13 13 13
 13 Experiment 13: Twitter             13
 13 Widget(s):                         13
 13  - Twitter: Connect account        13
 13  - Button: TweeEn, V22, Switch     13
 13  - Slider: Threshold, V23, 1023    13
 13  - Slider: Rate, V24, 0-60         13
 13 13 13 13 13 13 13 13 13 13 13 13 13 */
// Tweets are sent in blynkLoop. These functions set enable
// flags and other twitter-controlling parameters.
bool tweetEnabled = false; // Twitter enabled flag
unsigned long tweetUpdateRate = 60000; // Default tweet rate (ms)
unsigned long lastTweetUpdate = 0; // Last tweet flag
unsigned int moistureThreshold = 512; // Low-moisture setting

// A button attached to V22 enables or disables tweeting
BLYNK_WRITE(TWEET_ENABLE_VIRTUAL) 
{
  uint8_t state = param.asInt();
  BB_DEBUG("Tweet enable: " + String(state));
  if (state) // If the param is >=1
  {
    tweetEnabled = true; // Enable tweeting
    BB_DEBUG("Tweet enabled.");
  }
  else
  {
    tweetEnabled = false; // Disable tweeting
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

void twitterUpdate(void)
{
  unsigned int moisture = analogRead(A0);
  String msg = boardName + "\r\nSoil Moisture reading: " + String(moisture) + "\r\n";
  if (moisture < moistureThreshold)
  {
    msg += "FEED ME!\r\n";
  }
  msg += "[" + String(millis()) + "]";
  BB_DEBUG("Tweeting: " + msg);
  Blynk.tweet(msg);  
}

/* 14 14 14 14 14 14 14 14 14 14 14 14 14
 14 Experiment 14: Push                14
 14 Widget(s):                         14
 14  - Push: Off, On (iPhone)          14
 14  - Value: DoorState, V25, 1sec     14
 14  - Button: PushEnable, V26, Switch 14
 14 14 14 14 14 14 14 14 14 14 14 14 14 */
#define DOOR_SWITCH_PIN 16
#define NOTIFICATION_LIMIT 60000
unsigned long lastDoorSwitchNotification = 0;
bool pushEnabled = false;
uint8_t lastSwitchState = 255;

BLYNK_READ(DOOR_STATE_VIRTUAL)
{
  uint8_t switchState = digitalRead(DOOR_SWITCH_PIN); // Read the door switch pin
  // Pin 16 is pulled low internally. If the switch (and door) is open,
  // pin 16 is LOW. If the switch is closed (door too), pin 16 is HIGH.
  // LOW = open
  // HIGH = closed
  if (switchState)
    Blynk.virtualWrite(DOOR_STATE_VIRTUAL, "Close"); // Update virtual variable
  else
    Blynk.virtualWrite(DOOR_STATE_VIRTUAL, "Open");
    
  if (lastSwitchState != switchState) // If the state has changed
  {
    if (switchState) // If the switch is closed (door shut)
    {
      if (pushEnabled)
      {
        BB_DEBUG("Notified closed.");
        
        if (lastDoorSwitchNotification && (lastDoorSwitchNotification + NOTIFICATION_LIMIT > millis()))
        {
          int timeLeft = (lastDoorSwitchNotification + NOTIFICATION_LIMIT - millis()) / 1000;
          BB_DEBUG("Can't notify for " + String(timeLeft) + "s");
          terminal.println("Door closed. Can't notify for " + String(timeLeft) + "s");
          terminal.flush();
        }
        else
        {
          Blynk.notify("Door closed\r\nFrom: " + boardName + "\r\n[" + String(millis()) + "]");
          lastDoorSwitchNotification = millis();
        }
      }
    }
    else
    { 
      if (pushEnabled)
      {
        BB_DEBUG("Notified opened.");
        // Send the notification
        if (lastDoorSwitchNotification && (lastDoorSwitchNotification + NOTIFICATION_LIMIT > millis()))
        {
          int timeLeft = (lastDoorSwitchNotification + NOTIFICATION_LIMIT - millis()) / 1000;
          BB_DEBUG("Can't notify for " + String(timeLeft) + "s");
          terminal.println("Door open. Can't notify for " + String(timeLeft) + "s");
          terminal.flush();
        }
        else
        {
          Blynk.notify("Door open\r\nFrom: " + boardName + "\r\n[" + String(millis()) + "]");
          lastDoorSwitchNotification = millis();
        }
      }
    }
    lastSwitchState = switchState;
  }  
}

BLYNK_WRITE(PUSH_ENABLE_VIRTUAL)
{
  uint8_t state = param.asInt(); // Read in handler parameter
  BB_DEBUG("Push enable: " + String(state));
  if (state) // If it's >= 1
  {
    pushEnabled = true; // enable push
    BB_DEBUG("Push enabled.");
  }
  else
  {
    pushEnabled = false; // disable push
    BB_DEBUG("Push disabled.");    
  }
}

/* 15 15 15 15 15 15 15 15 15 15 15 15 15
 15 Experiment 15: Email               15
 15 Widget(s):                         15
 15  - Email: Off, On (iPhone)         15
 15  - Terminal: V21, On, On           15
 15  - Button: Send, V27, Push         15
 15 15 15 15 15 15 15 15 15 15 15 15 15 */

#define EMAIL_UPDATE_RATE 60000 // Minimum time between emails
unsigned long lastEmailUpdate = 0; // Keeps track of last email send time

void emailUpdate(void)
{
  String emailSubject = "My BlynkBoard Statistics"; // Set the subject
  String emailMessage = ""; // Create a message string
  emailMessage += "D0: " + String(digitalRead(0)) + "\r\n"; // Add D0 status
  emailMessage += "D16: " + String(digitalRead(16)) + "\r\n"; // Add D16 status 
  emailMessage += "\r\n"; // Empty line
  emailMessage += "A0: " + String(analogRead(A0)) + "\r\n"; // Add A0 reading
  emailMessage += "\r\n"; // Empty line
  emailMessage += "Temp: " + String(thSense.readTemperature()) + "C\r\n"; // Add temp sensor
  emailMessage += "Humidity: " + String(thSense.readHumidity()) + "%\r\n"; // Add humidity sensor
  emailMessage += "\r\n"; // Empty line
  emailMessage += "Runtime: " + String(millis() / 1000) + "s\r\n";
  // May have been running into an issue with a too-long emailMessage.
  // Be careful adding anything else to this String.
  BB_DEBUG("email: " + emailAddress);
  BB_DEBUG("subject: " + emailSubject);
  BB_DEBUG("message: " + emailMessage);
  // Send the email:
  Blynk.email(emailAddress.c_str(), emailSubject.c_str(), emailMessage.c_str());
  // Print a help message
  terminal.println("Sent an email to " + emailAddress);
  terminal.flush();
}

BLYNK_WRITE(EMAIL_ENABLED_VIRTUAL)
{
  int emailEnableIn = param.asInt(); // Read parameter in
  BB_DEBUG("Email enabled: " + String(emailEnableIn))
  if (emailEnableIn) // If parameter is >= 1
  {
    // And if an email address has been set through terminal
    if (emailAddress != "") 
    {
      // And if it's been 60 or more seconds since the last email.
      if ((lastEmailUpdate == 0) || (lastEmailUpdate + EMAIL_UPDATE_RATE < millis()))
      { 
        emailUpdate(); // Send an email
        lastEmailUpdate = millis(); // Update the email time
      }
      else // If we haven't waited long enough
      {
        // Print how many seconds before the next print
        int waitTime = (lastEmailUpdate + EMAIL_UPDATE_RATE) - millis();
        waitTime /= 1000;
        terminal.println("Please wait " + String(waitTime) + " seconds");
        terminal.flush();
      }
    }
    else // If an email address has not been set
    { // Print a help message:
      terminal.println("Type !email@address.com to set the email address");
      terminal.flush();
    }
  }
}

// Runtime tracker utility function. Displays the run time to a 
// maximum of four digits. It'll show up to 999 seconds, then
// up to 999 minutes, then up to 999 hours, then up to 999 days
BLYNK_READ(RUNTIME_VIRTUAL)
{
  float runTime = (float) millis() / 1000.0; // Convert millis to seconds
  // Assume we can only show 4 digits
  if (runTime >= 1000) // 1000 seconds = 16.67 minutes
  {
    runTime /= 60.0; // Convert to minutes
    if (runTime >= 1000) // 1000 minutes = 16.67 hours
    {
      runTime /= 60.0; // Convert to hours
      if (runTime >= 1000) // 1000 hours = 41.67 days
        runTime /= 24.0;
    }
  }
  Blynk.virtualWrite(RUNTIME_VIRTUAL, runTime);
}

// Reset WiFi and Blynk auth token 
// Utility function. 
//! Not to be included in release firmware!
#ifdef DEBUG_ENABLED
BLYNK_WRITE(RESET_VIRTUAL)
{
  int resetIn = param.asInt();
  BB_DEBUG("Blynk Reset: " + String(resetIn));
  if (resetIn)
  {
    BB_DEBUG("Factory resetting");
    resetEEPROM();
    ESP.reset();
  }
}
#endif

void blynkSetup(void)
{
  BB_DEBUG("Initializing Blynk Demo");
  runMode = MODE_BLYNK_RUN; // Set to Blynk Run mode
  
  WiFi.enableAP(false); // Disable access point mode

  // Setup the Pin 0 button:
  detachInterrupt(BUTTON_PIN); // detatch the buttonChange interrupt [BlynkBoard_Core_Firmware]

  // Set up the temperature-humidity sensor
  thSense.begin();

  // Set up the pin 16 door switch input:
  pinMode(DOOR_SWITCH_PIN, INPUT_PULLDOWN_16);
  lastSwitchState = digitalRead(DOOR_SWITCH_PIN);

  if (scanI2C(LUX_ADDRESS)) // Look for a TSL light sensor
  {
    BB_DEBUG("Luminosity sensor connected.");
    luxInit(); // If it's there, initialize it
  }

  BB_DEBUG("Done initializing Blynk demo");
}

void blynkLoop(void)
{
  // Polling Button status -- interrupts lead to bounce errors,
  // especially if noisy buttons are connected.
  if (lastButtonUpdate + BUTTON_UPDATE_RATE < millis())
  {
    buttonUpdate();
    lastButtonUpdate = millis();
  }

  // Polling Light status -- results in the best UX using history graph
  if (lastLuxUpdate + LUX_UPDATE_RATE < millis())
  {
    luxUpdate();
    lastLuxUpdate = millis();
  }

  // If twitter is enabled, and it's been long enough since the last tweet
  if (tweetEnabled && ((lastTweetUpdate == 0) || (lastTweetUpdate + tweetUpdateRate < millis())))
  {
    twitterUpdate(); // Send a tweet
    lastTweetUpdate = millis(); // Update the last tweet time
  }

  if (Serial.available()) // If serial is available
  {
    String toSend;
    while (Serial.available())
      toSend += (char)Serial.read(); // Read all available chars into a string
    terminal.print(toSend); // Send them to the terminal
    terminal.flush(); // Make sure they're sent
  }
}

// Check for a response from an I2C device
bool scanI2C(uint8_t address)
{
  Wire.beginTransmission(address);
  Wire.write( (byte)0x00 );
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}
