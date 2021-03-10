# HomeKit enabled ESP32 based Night Light
- Read my [blog](https://darshantechie.blogspot.com/2021/03/homemade-homekit-supported-night-light.html) to understand the purpose of this project and to watch demo video.
- This project can be used with any ESP32 or ESP32-S2 board having WS2812 LED on the board.
- Using this project, you can make home made night light having HomeKit support, which you can control from your iOS Home App.

## How to build and use Night Light

- Checkout this repository.
- Checkout [esp-idf](https://github.com/espressif/esp-idf) repository.
- Checkout [esp-homekit-sdk](https://github.com/espressif/esp-homekit-sdk) repository. If you have MFi license, you can get MFi compliant SDK from Espressif.
- Follow the [guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation-step-by-step) to install prerequisites for build and compile one of the example project.
- Open a terminal and set `HOMEKIT_PATH` environment variable with path of esp-homekit-sdk.
- Go to esp-idf repository in terminal and run `./install.sh` command.
- Now run `. ./export.sh` command.
- Go to this repository in terminal.
- Run command `idf.py set-target esp32`. If you are using ESP32-S2, replace esp32 with esp32s2 in command.
- Run command `idf.py menuconfig` and go to Example Configuration to configure HomeKit setup code and setup ID. We will use hard coded setup code here. Also configure GPIO number of WS2812 LED.
- Go to Component config -> HomeKit to enable MFi if you want to use WAC for Wi-Fi configuration and have MFi chip (or Software Authentication) and MFi supported SDK. By default it is off and you need to use Espressif provisioning app to provide Wi-Fi credentials.
- Run `idf.py build` to build the project.
- Run `idf.py -p PORT flash` to flash your board. Replace PORT with your board port, something like /dev/ttyUSB0.
- Download Espressif provisioning app for [Android](https://play.google.com/store/apps/details?id=com.espressif.provsoftap) or [iOS](https://apps.apple.com/us/app/esp-softap-provisioning/id1474040630).
- Now use Espressif provisioning app for Wi-Fi provisioning. If you are using MFi chip or Software Authentication, you can use iOS Home app to provision Wi-Fi and HomeKit pairing.
- Once you have provided Wi-Fi credentials with Espressif provisioning app, open iOS Home app and add accessory. Use setup code which you configured in above steps for iOS Home app pairing.

You can change the color and brightness using iOS Home app. Enjoy the home made Night Light!

## Factory reset and changing Wi-Fi credentials

There may be a need to factory reset the board and pair with another iOS device. Or just change Wi-Fi credentials.

- To factory reset the board, press RST button (GPIO 0) on the board for 5 seconds. After that use above steps to configure Wi-Fi credentials and pair with new iOS device.
- To change Wi-Fi credentials, press RST button for 2 seconds and leave. Board will erase Wi-Fi credentials. Use Espressif provisioning app to provide new Wi-Fi credentials. Once new credentials are given, device will be visible in iOS Home app, no need to pair again.

## Eclipse support

- This project supports Eclipse and is having Eclipse project files.
- You can install [ESP-IDF Eclipse plugin](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md) and then open this project in Eclipse.
- You will be able to build, flash and debug this project right from Eclipse!
