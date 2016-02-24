# SparkFun Blynk Board - ESP8266 Core Firmware

The [SparkFun Blynk Board - ESP8266]() firmware allows you to configure a Blynk Board's WiFi network and Blynk auth token. Then, once the board is equipped with network access and a connection to the Blynk Cloud, the firmware is equipped with ten Blynk experiments, which all you to test out the Blynk Board and the Blynk app without ever bothering to re-program the board's ESP8266 WiFi/microcontroller.

### Firmware Contents

To better navigate the firmware's funcationality, the source code is divided into a number of separate files:

* **[BlynkBoard_Core_Firmware.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Core_Firmware.ino)** -- Main source file, where `setup()` and `loop()` are defined. Also controls the RGB LED status indicator.
* **[BlynkBoard_Setup.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Setup.ino)** -- Hardware setup and EEPROM/flash reading/writing.
* **[BlynkBoard_ConfigMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_ConfigMode.ino)** -- Configuration mode functions; AP and serial WiFi/Blynk configuration.
* **[BlynkBoard_BlynkMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_BlynkMode.ino)** -- Blynk mode functions, including definition of all Blynk Board introductory experiments.
* **[BlynkBoard_settings.h](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_settings.h)** -- Various constant settings, including RGB colors and connection timeouts.