# SparkFun Blynk Board - ESP8266 Core Firmware

The [SparkFun Blynk Board - ESP8266]() firmware allows you to configure a Blynk Board's WiFi network and Blynk auth token. Then, once the board is equipped with network access and a connection to the Blynk Cloud, the firmware is equipped with ten Blynk experiments, which all you to test out the Blynk Board and the Blynk app without ever bothering to re-program the board's ESP8266 WiFi/microcontroller.

### Firmware Contents

To better navigate the firmware's funcationality, the source code is divided into a number of separate files:

* **[BlynkBoard_Core_Firmware.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Core_Firmware.ino)** -- Main source file, where `setup()` and `loop()` are defined. Also controls the RGB LED status indicator.
* **[BlynkBoard_Setup.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_Setup.ino)** -- Hardware setup and EEPROM/flash reading/writing.
* **[BlynkBoard_ConfigMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_ConfigMode.ino)** -- Configuration mode functions; AP and serial WiFi/Blynk configuration.
* **[BlynkBoard_BlynkMode.ino](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_BlynkMode.ino)** -- Blynk mode functions, including definition of all Blynk Board introductory experiments.
* **[BlynkBoard_settings.h](https://github.com/sparkfun/Blynk_Board_ESP8266/Firmware/BlynkBoard_settings.h)** -- Various constant settings, including RGB colors and connection timeouts.

### Using Arduino to Upload to the Blynk Board

The Blynk Board firmware is written in Arduino, and uses the [ESP8266/Arduino](https://github.com/esp8266/arduino) board support files. The ESP8266/Arduino board package must be installed - see [here](https://github.com/esp8266/arduino#installing-with-boards-manager) for more information. **Version 2.1.0-rc2 or later is required**.

After installing the Arduino ESP8266 board definitions, an additional board-entry item must be added to **boards.txt** to support the SparkFun Blynk Board. Find boards.txt within your latest esp8266 package. On Windows it should be somewhere like "C:\Users\user.name\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.1.0-rc2". On Mac, the package may be located in "/Users/NAME/Library/Arduino15/packages/esp8266/hardware/esp8266/2.1.0-rc1".

Open boards.txt, and add the textblock below to the bottom of the file:

	##############################################################
	sparkfunBlynk.name=SparkFun Blynk Board

	sparkfunBlynk.upload.tool=esptool
	sparkfunBlynk.upload.speed=115200
	sparkfunBlynk.upload.resetmethod=nodemcu
	sparkfunBlynk.upload.maximum_size=1044464
	sparkfunBlynk.upload.maximum_data_size=81920
	sparkfunBlynk.upload.wait_for_upload_port=true
	sparkfunBlynk.serial.disableDTR=true
	sparkfunBlynk.serial.disableRTS=true

	sparkfunBlynk.build.mcu=esp8266
	sparkfunBlynk.build.f_cpu=80000000L
	sparkfunBlynk.build.board=ESP8266_THING
	sparkfunBlynk.build.core=esp8266
	sparkfunBlynk.build.variant=thing
	sparkfunBlynk.build.flash_mode=qio
	sparkfunBlynk.build.flash_size=4M
	sparkfunBlynk.build.flash_freq=40
	sparkfunBlynk.build.debug_port=
	sparkfunBlynk.build.debug_level=

	sparkfunBlynk.menu.CpuFrequency.80=80 MHz
	sparkfunBlynk.menu.CpuFrequency.80.build.f_cpu=80000000L
	sparkfunBlynk.menu.CpuFrequency.160=160 MHz
	sparkfunBlynk.menu.CpuFrequency.160.build.f_cpu=160000000L

	sparkfunBlynk.menu.UploadTool.esptool=Serial
	sparkfunBlynk.menu.UploadTool.esptool.upload.tool=esptool
	sparkfunBlynk.menu.UploadTool.esptool.upload.verbose=-vv

	sparkfunBlynk.menu.UploadSpeed.115200=115200
	sparkfunBlynk.menu.UploadSpeed.115200.upload.speed=115200
	sparkfunBlynk.menu.UploadSpeed.9600=9600
	sparkfunBlynk.menu.UploadSpeed.9600.upload.speed=9600
	sparkfunBlynk.menu.UploadSpeed.57600=57600
	sparkfunBlynk.menu.UploadSpeed.57600.upload.speed=57600
	sparkfunBlynk.menu.UploadSpeed.256000=256000
	sparkfunBlynk.menu.UploadSpeed.256000.upload.speed=256000
	sparkfunBlynk.menu.UploadSpeed.921600=921600
	sparkfunBlynk.menu.UploadSpeed.921600.upload.speed=921600

	sparkfunBlynk.menu.FlashSize.4M1M=4M (1M SPIFFS)
	sparkfunBlynk.menu.FlashSize.4M1M.build.flash_size=4M
	sparkfunBlynk.menu.FlashSize.4M1M.build.flash_ld=eagle.flash.4m1m.ld
	sparkfunBlynk.menu.FlashSize.4M1M.build.spiffs_start=0x300000
	sparkfunBlynk.menu.FlashSize.4M1M.build.spiffs_end=0x3FB000
	sparkfunBlynk.menu.FlashSize.4M1M.build.spiffs_blocksize=8192
	sparkfunBlynk.menu.FlashSize.4M1M.build.spiffs_pagesize=256

Restart Arduino, and you should find an entry for "SparkFun Blynk Board" at the bottom of the "ESP8266 Modules" board list.

[![Blynk board in arduino](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/4/8/6/blynk-board-selection.png)](https://cdn.sparkfun.com/assets/learn_tutorials/4/8/6/blynk-board-selection.png)

Any upload speed can be used to load code onto the Blynk Board. 921600 is recommended, though may result in the occasional upload error.

### Custom Libraries Used

* [Blynk Arduino Library](https://github.com/blynkkk/blynk-library/releases/tag/v0.3.1)
* [SparkFun HTU21D](https://github.com/sparkfun/SparkFun_HTU21D_Breakout_Arduino_Library/releases/tag/V_1.1.1)
* [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
* [SparkFun TSL2561 Library](https://github.com/sparkfun/SparkFun_TSL2561_Arduino_Library/releases/tag/V_1.1.0)
