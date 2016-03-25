# SparkFun Blynk Board - ESP8266 Core Firmware

The [SparkFun Blynk Board - ESP8266](https://www.sparkfun.com/products/13794) firmware allows you to configure a Blynk Board's WiFi network and Blynk auth token. Then, once the board is equipped with network access and a connection to the Blynk Cloud, the firmware is equipped with ten Blynk experiments, which all you to test out the Blynk Board and the Blynk app without ever bothering to re-program the board's ESP8266 WiFi/microcontroller.

### Firmware Contents

To better navigate the firmware's funcationality, the source code is divided into a number of separate files:

* **[BlynkBoard_Core_Firmware.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Core_Firmware.ino)** -- Main source file, where `setup()` and `loop()` are defined. Also controls the RGB LED status indicator.
* **[BlynkBoard_Setup.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Setup.ino)** -- Hardware setup and EEPROM/flash reading/writing.
* **[BlynkBoard_ConfigMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_ConfigMode.ino)** -- Configuration mode functions; AP and serial WiFi/Blynk configuration.
* **[BlynkBoard_BlynkMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_BlynkMode.ino)** -- Blynk mode functions, including definition of all Blynk Board introductory experiments.
* **[BlynkBoard_settings.h](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_settings.h)** -- Various constant settings, including RGB colors and connection timeouts.

### Using Arduino to Upload to the Blynk Board

For help adding Blynk Board support to your Arduino IDE, check out our [Blynk Board Arduino Development Guide](https://learn.sparkfun.com/tutorials/blynk-board-arduino-development-guide).

You can install the board definitions using the **Arduino Board Manager**. Just paste the link below into the "Additional Board URL's" textbox, in Arduino properties:

	https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json

After installing the Arduino ESP8266 board definitions, you'll find a option for "SparkFun Blynk Board" under the **Tools** > **Board** menu - select that, and upload some code!

### Custom Libraries Used

* [Blynk Arduino Library](https://github.com/blynkkk/blynk-library/releases/tag/v0.3.1)
* [SparkFun HTU21D](https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library/releases/tag/V_1.1.1)
* [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
* [SparkFun TSL2561 Library](https://github.com/sparkfun/SparkFun_TSL2561_Arduino_Library/releases/tag/V_1.1.0)
