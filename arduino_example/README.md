# RoboCore S3 Arduino Examples

[English](README.md) | [简体中文](README.zh-CN.md)

Each folder is a standalone Arduino project. **Uploading an example replaces the complete factory firmware**; it does not preserve the web console. Restore the complete firmware by uploading this repository's PlatformIO project or the matching factory recovery package.

## Arduino IDE setup

1. Install Arduino IDE 2 and **esp32 by Espressif Systems 3.3.0** through Boards Manager.
2. Select **ESP32S3 Dev Module**, Flash Size **8MB**, and PSRAM **OPI PSRAM**. For all examples except `USB_CDC_JSON`, set USB CDC On Boot to **Disabled**, connect **Type-C UART**, and use that port for upload and serial output. For `USB_CDC_JSON`, select USB Mode **USB-OTG (TinyUSB)**, enable USB CDC On Boot, and use Type-C USB.
3. Install the dependencies in the table below. `lygion_devs` and `RoboCoreSupport` are folders under this repository's `lib`; copy each whole folder into the Arduino sketchbook's `libraries` directory, then restart the IDE. The SDK is also available from [LygionOrganization/lygion_devs_cpp](https://github.com/LygionOrganization/lygion_devs_cpp); these examples use the version bundled here.
4. Open the `.ino` file matching the folder name, then compile and upload. Set the Serial Monitor to **115200** on the Type-C UART port. Standard examples explicitly use Serial0 and are not affected by Serial's USB mapping. `USB_CDC_JSON` uses native USB CDC and its baud rate is a virtual parameter.
5. Servo and motor examples do not move on startup. Enter the character shown in the serial prompt to trigger an action. Check rated supply voltage and mechanical travel first.

RoboCore S3 accepts 5–48V, and bus power equals the board input voltage. Every device sharing that power must support the selected voltage. GH1.25-3P is for board-to-board TTL communication and power decoupling.

## Examples and dependencies

| Folder | Function | Additional dependencies |
| --- | --- | --- |
| SCS_Servo | SCS position, feedback, torque disable | FTServo 2.0.0 |
| HLS_Servo | HLS position, current limit, feedback | FTServo 2.0.0 |
| STS_Servo | STS position, feedback, torque disable | FTServo 2.0.0 |
| SMS_Servo | SMS position, feedback, torque disable | FTServo 2.0.0 |
| Bus_Scan | Scan IDs at the current baud rate | lygion_devs |
| TTL_Node | PWM, RGB, S.BUS, change ID | lygion_devs |
| TTL_Stepper | Position/continuous/follower modes, enable, motion, set current position, feedback | lygion_devs |
| TTL_Encoder | E02 position, speed, reference calibration | lygion_devs |
| CAN_Communication | CAN mode, CAN 2+2, RX13/TX14, 500 kbps | Built-in TWAI |
| RS485_Communication | RS485 mode, GH1.25-5P, RX13/TX14, 115200 8N1 | Built-in HardwareSerial |
| User_Interface_Module_A | Five-way keys, buzzer, 128×32 OLED demo | Adafruit SSD1306, GFX Library, BusIO |
| OLED | 128×32 OLED display | Adafruit SSD1306, Adafruit GFX Library (and Adafruit BusIO if prompted) |
| QMI8658A | Onboard six-axis sensor | RoboCoreSupport; Wire is included with the core |
| HTTP_JSON | HTTP JSON request/response | ArduinoJson 7.x; built-in WebServer/WiFi |
| WebSocket_JSON | WebSocket JSON request/response | ArduinoJson 7.x, ESP32Async/AsyncTCP, ESP32Async/ESPAsyncWebServer |
| USB_CDC_JSON | Event-based receive, fixed-size framing, latest command wins | ArduinoJson 7.x; built-in USB/USBCDC |
| ESP_NOW_JSON | JSON packets between two boards | ArduinoJson 7.x; built-in esp_now/WiFi |
| RGB_Buzzer | External GPIO26 WS2812 strip and UI Module (A) buzzer (not onboard hardware) | Adafruit NeoPixel |

Dependency versions: ArduinoJson 7.4.3, AsyncTCP 3.5.0, ESPAsyncWebServer 3.12.1, Adafruit NeoPixel 1.15.5, GFX 1.12.6, SSD1306 2.5.17. Check library authors when installing similarly named libraries; avoid outdated AsyncWebServer forks.

## Pins

RoboCore S3 GPIO: Serial1 RX=5/TX=4, I²C SDA=6/SCL=7, CAN TX=14/RX=13; UI Module (A) buzzer=21. OLED address is 0x3C; QMI8658A tries 0x6A and 0x6B automatically. Sensor communication errors are reported without stopping the sketch.

## Standalone examples use a different protocol from the complete firmware

`HTTP_JSON`, `WebSocket_JSON`, and `USB_CDC_JSON` demonstrate storing one RAM value: `{"T":1,"value":42}` writes it and `{"T":2}` reads it. They do not control motors. HTTP/WS examples create the `RoboCoreDemo` / `12345678` access point and use `http://192.168.4.1/api/cmd` and `ws://192.168.4.1/ws`. For USB, end each JSON line with a newline and keep reading the response.

The complete factory firmware uses commands such as `T=700…740`; do not use the standalone examples' `T=1/2` as the factory API. See the [RoboCore API](../docs/ROBOCORE_API.md).

The ESP-NOW example requires both boards on the same channel. Set `peerMac` to the other board's STA MAC. Frames are 250 bytes and zero-padded to match the complete firmware's JSON frame. To connect to the complete firmware, configure its receive mode/trusted MAC (T=411, 414) and keep the Wi-Fi channel the same.

## Stepper driver notes

Commands `0` and `1` switch between position and speed modes during one power session without a restart; only a phase configuration change requires restart. A mode change disables output; send `e` to enable it again. Command `2` sets follower mode with example primary ID 2; connect the EDS as shown in its product tutorial. Speed, current and acceleration use register units; check device settings and motor rated current first. `s` sets speed to zero only in continuous-rotation mode; `x` disables output.

## Use an example with PlatformIO

Each example already includes `#include <Arduino.h>`. Copy one `.ino` into `src/main.cpp` in a separate PlatformIO project and configure ESP32-S3 and dependencies as in this repository's `platformio.ini`. Do not put multiple examples in one `src` directory because their `setup`/`loop` functions conflict. Do not add standalone examples to the complete firmware's `src` directory.

If you copy the complete `platformio.ini`, also copy `tools/pio_python_path.py` to the same relative path in the new project. It fixes the platform's Python tool lookup path on Windows. Arduino IDE does not need this script.

## External communications and bus-device interface

The CAN/RS485 switch only selects IO13 (RX)/IO14 (TX): CAN mode uses CAN 2+2, and RS485 mode uses GH1.25-5P. These external protocols cannot use those pins at the same time. Stop communication before switching and run the example for the selected protocol. `RS485_Communication` uses UART2; the onboard circuit controls transmit/receive direction, so no DE GPIO is needed. It demonstrates byte communication and is not a factory JSON gateway.

XH2.54-4P bus-device RS485 and 5264-3P single-wire TTL share UART1 (RX5/TX4), including the baud rate and transmitted data; they are not two independent serial ports. External UART2 RS485 does not conflict with this interface. CAN has an onboard 120Ω termination resistor; check termination across the full bus when connecting multiple nodes.

## User Interface Module (A)

With power disconnected, connect the module to the PH1.25-10P expansion port. Pins: Up=10, Down=11, Left=12, Right=9, OK=2 (active low); buzzer=21; OLED SDA=6/SCL=7, address 0x3C, 128×32. After debounce, key names appear on the serial monitor and OLED and trigger one short beep; holding a key does not repeat. Keys and buzzer still work when the OLED is absent. This example sends no motor commands. The OLED shares I²C with the onboard IMU.

The OLED, buttons and buzzer belong to the external UI Module (A), not the RoboCore S3 board. The WS2812 feature in `RGB_Buzzer` is for a separately wired light strip; GPIO26 is not on the PH1.25-10P connector, so this is not an onboard-light demo.

## Detailed tutorials

Each tutorial includes wiring, dependency installation, upload steps, expected results and complete code: [English tutorials](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/arduino/) / [中文教程](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/arduino/).
