# RoboCore S3

**Open-source ESP32-S3 firmware and examples for robot control.**

**English** | [简体中文](README.zh-CN.md)

[English Wiki](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/) · [中文 Wiki](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/) · [JSON API](docs/ROBOCORE_API_EN.md) · [Arduino examples](arduino_example/)

## What is this project?

RoboCore S3 connects your robot's bus devices to a browser or a host computer. This repository contains the complete controller firmware, a bilingual web console, independent Arduino sketches, and Python clients.

Use it as a standalone controller for device setup and task execution, or as the low-level controller in a PC/SBC-based robot. The host handles perception, planning and application logic; RoboCore S3 sends device commands and returns feedback through human-readable JSON.

The [`robocore_s3`](https://github.com/LygionOrganization/robocore_s3) firmware project supports **both RoboCore S3 and Robot Driver with ESP32S3 Lite**. This guide focuses on RoboCore S3, while Lite users can use the same firmware and the examples applicable to their hardware.

For Lite power ratings, connectors and peripherals, follow the [Lite Wiki](https://wiki.lygion.ai/en/reference/bus-devices/robot-driver-with-esp32s3-lite/). The hardware specifications below apply to RoboCore S3; do not apply its 5–48V input range to Lite. Lite has no onboard QMI8658A, and the firmware continues running when the IMU is absent.

## What you can do

| Capability | Use |
| --- | --- |
| Browser control | Discover bus IDs, configure devices and read feedback without writing code |
| Bus device control | Work with SCS, HLS, STS and SMS servos, TTL Node, TTL Stepper Driver and TTL Encoder E02 |
| Host integration | Send JSON over HTTP, WebSocket or native USB CDC |
| Motion sensing | Read the onboard QMI8658A accelerometer and gyroscope |
| Local interaction | Add User Interface Module (A) for buttons, buzzer alerts and OLED display |
| Custom development | Build the complete firmware with PlatformIO or start from a focused Arduino example |

The main web console works offline, opens in English by default and offers a **中文** button in the upper-right corner. Advanced mission and multi-axis tools remain available through the classic console at `/legacy`.

## Start with the web console

With the complete firmware installed:

1. Connect a bus device and select a power supply suitable for that device. Powered bus outputs carry the board's input voltage.
2. Join the board's default Wi-Fi network: **`Robot`**, password **`12345678`**. If it has been configured before, use its current network settings.
3. Open **[http://192.168.4.1](http://192.168.4.1)** in a browser. No internet connection is required.
4. Open **Bus devices**, match the bus baud rate and scan IDs. Select the appropriate device panel and read its status before commanding motion.

See the [quick-start guide](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/quickstart/) for the full walkthrough, or [firmware recovery](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/firmware/) to restore the complete firmware.

## Control it from a PC or SBC

The same JSON commands can be sent through HTTP, WebSocket or USB CDC. For example, this read-only command scans IDs 0–20:

```json
{"T":700,"first":0,"last":20}
```

The included Python client handles request IDs and waits for a matching result. From the repository root:

```sh
python -m pip install requests websocket-client pyserial
```

```python
from python_example.robocore.client import request

result = request(
    'http',
    'http://192.168.4.1',
    {'T': 700, 'first': 0, 'last': 20},
)
print(result)
```

Use `ws` with `ws://192.168.4.1/ws`, or `usb` with your actual serial port, to select another transport. USB CDC uses the board's native **Type-C USB** connector.

Python is only one option. Use the [JSON command reference](docs/ROBOCORE_API_EN.md) to write clients in C++, C#, JavaScript or another language, including with AI-assisted development.

**Command behavior:** only the latest pending command is retained. HTTP `202` means accepted, not executed. Wait for the matching result when changing settings, and read device state before retrying a timed-out write. **Stop loop (`T=0`) exits a mission loop; it is not a motor emergency stop.**

## Windows firmware download tool

The [download tool folder](download_tool/RoboCore_S3_Download_Tool/) includes matching firmware images and bilingual instructions for RoboCore S3 and Lite. Extract the complete package and run `Start_Download_Tool.bat`; choose the board’s COM port before starting. Maintainers can rebuild the package with `python tools/package_download_tool.py`.

## Build your own firmware

### Complete firmware — PlatformIO

Open this repository in VS Code with PlatformIO. Keep the supplied `platformio.ini`, `src/`, `include/`, `lib/` and `tools/` together; the project pins its platform and library versions.

```sh
pio run
```

To change the web console, edit `web/index.html`, regenerate the embedded page, then rebuild:

```sh
python tools/embed_web.py
pio run
```

Build output is under `.pio/build/esp32-s3-devkitc-1-n8r8/`. For toolchain details and verification scope, see [validation notes](docs/ROBOCORE_VALIDATION.md).

### Focused examples — Arduino IDE

The [`arduino_example/`](arduino_example/) directory contains 18 standalone sketches covering bus devices, CAN, external RS485, the IMU, User Interface Module (A), and JSON transports.

Start with the [English Arduino guide](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/arduino/) or the [local setup and dependency list](arduino_example/README.md). The checked configuration uses **ESP32S3 Dev Module**, **8 MB flash**, **OPI PSRAM**, **USB-OTG (TinyUSB)** and **USB CDC On Boot enabled**.

**Uploading a standalone sketch replaces the complete firmware and its web console.** The HTTP/WebSocket/USB teaching sketches use their own minimal protocol; they do not implement the full controller's JSON command set.

## Hardware at a glance

| Resource | RoboCore S3 |
| --- | --- |
| Controller | ESP32-S3-WROOM-1U-MCR8N8; 8 MB flash, 8 MB PSRAM |
| Power input | 5–48V through the XT30 power contacts of CAN 2+2 |
| Servo/device bus | 5264-3P single-wire TTL and XH2.54-4P RS485 share IO4 TX / IO5 RX |
| External communication | CAN/RS485 switch selects CAN 2+2 or GH1.25-5P; both use IO13 RX / IO14 TX |
| Board interconnect | GH1.25-3P for board-to-board TTL communication and power decoupling |
| USB | Type-C UART for serial download/debugging; Type-C USB for native USB communication |
| IMU | Onboard QMI8658A on IO6 SDA / IO7 SCL |
| Expansion | PH1.25-10P for User Interface Module (A); OLED, buttons and buzzer are on the external module |
| Dimensions | 47 × 52 mm; four Ø3 mm mounting holes, 40 × 45 mm center spacing |

**Match the supply voltage to every powered bus device.** A 48V-capable controller does not make a low-voltage servo compatible with 48V. USB alone does not provide actuator power.

External RS485 on IO13/IO14 is independent of the servo bus on IO4/IO5. Servo RS485 and TTL share one UART. The CAN/RS485 switch selects the hardware path; your firmware must use the matching protocol. See the [hardware guide](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/hardware/) before wiring.

## Repository guide

| Path | Contents |
| --- | --- |
| [`src/`](src/) | Complete firmware and generated web page |
| [`web/`](web/) | Editable bilingual console |
| [`arduino_example/`](arduino_example/) | Standalone Arduino sketches |
| [`python_example/robocore/`](python_example/robocore/) | HTTP, WebSocket and USB CDC client |
| [`lib/`](lib/) | Local device libraries and support code |
| [`docs/`](docs/) | JSON references, development guidance and Wiki sources |
| [`tools/`](tools/) | Web embedding, example checks and packaging |
| [`test/`](test/) | Host regression checks and web-console checks |

For AI-assisted projects, provide the [development guide](docs/ROBOCORE_AI_AGENTS.md), JSON reference, device models, IDs, baud rate and intended behavior. Begin with a read-only feedback test.

## License

This project is licensed under [GNU AGPL v3](LICENSE). Bundled third-party components retain their respective licenses.
