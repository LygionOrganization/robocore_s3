# Power, connectors and onboard resources

## Package contents

| Item | Quantity | Purpose |
| --- | --- | --- |
| RoboCore S3 board | 1 | Robot control and device communication |
| DC5521 female socket with quick-connect terminals | 1 | Accepts DC5521 / DC5525 power plugs |
| XT30U female cable, 30 cm | 1 | Cable rated for 10A; connects to the included DC5521 socket without soldering |
| USB-A to USB-C data cable, 1 m | 1 | Firmware download and PC/host communication |
| 2.4GHz bulkhead antenna | 1 | Panel mounting lets you position the antenna outside a metal robot enclosure for wireless communication |

10A is the rating of the included XT30U cable; also respect the ratings of the supply, socket, connectors and connected devices.

!!! warning "Connect the external antenna before wireless use"
    Attach the antenna before using Wi-Fi or ESP-NOW. Operating without it degrades wireless communication and may cause the ESP32-S3 module to heat up. For a metal enclosure, use the bulkhead mounting to place the antenna outside.

## Dimensions and mounting

| Item | Specification |
| --- | --- |
| Board size | 47 × 52 mm |
| Mounting holes | 4 |
| Hole diameter | 3 mm |
| Hole center spacing | 40 × 45 mm |
| Controller module | ESP32-S3-WROOM-1U-MCR8N8; 8 MB flash, 8 MB PSRAM |
| Development | Open-source examples for PlatformIO and Arduino IDE |

## Power

RoboCore S3 accepts **5–48V** through the XT30 power contacts of the CAN 2+2 connector. The powered 5264-3P TTL and XH2.54-4P bus-device RS485 connectors carry this input voltage, without individual voltage conversion. USB alone does not supply actuator power.

!!! danger "Match the supply to every powered device"
    A 48V-rated board does not make low-voltage servos compatible with 48V. Wire with power off and check polarity, pin assignments and device ratings. The GH1.25-5P 5V pin is an output: never apply motor supply voltage to it or tie it to another supply output.

GH1.25-3P provides board-to-board TTL communication and power decoupling.

## Connectors and resources

| Resource | Signals | Use |
| --- | --- | --- |
| Type-C UART | TXD0 / RXD0 via USB-to-UART | Firmware download and debugging, with automatic download circuit |
| Type-C USB | IO19 USB D−, IO20 USB D+ | Native ESP32-S3 USB, including host USB CDC communication |
| 5264-3P | IO4 TX / IO5 RX via single-wire TTL circuit | TTL bus devices; power follows XT30 input |
| XH2.54-4P | IO4 TX / IO5 RX via RS485 circuit | RS485 bus devices; power follows XT30 input |
| GH1.25-3P | Single-wire TTL | Board-to-board TTL communication and power decoupling |
| PH1.25-10P expansion | IO6 SDA, IO7 SCL, IO2, IO9, IO10, IO11, IO12, IO21, 3.3V, GND | Lygion User Interface Module (A) |
| CAN/RS485 selector | IO13 RX / IO14 TX | Select the external communication circuit |
| CAN 2+2 | CAN H/L and XT30 power contacts | Board power input; CAN communication on RX13/TX14 in CAN position |
| GH1.25-5P | RS485 A/B, GND, 5V output | External RS485 on RX13/TX14 in 485 position |
| QMI8658A six-axis onboard IMU | IO6 SDA / IO7 SCL | Three-axis acceleration and angular velocity; shares I²C with external OLED |
| BOOT button | IO0, active low | Enter download mode together with RESET |
| RESET button | EN, active low | Holds the MCU in reset while pressed; restarts on release, without disconnecting board power |

## CAN/RS485 selector

!!! warning "External communication and the device bus use different GPIO pairs"
    **CAN position: IO13 RX / IO14 TX communicate over the CAN 2+2 connector.**
    **485 position: the same IO13 RX / IO14 TX communicate over GH1.25-5P RS485.**
    These two external interfaces cannot use this GPIO pair simultaneously. The switch selects the physical circuit; firmware must also use the corresponding CAN or UART protocol. Stop communication before switching.

External RS485 on IO13/IO14 is independent of bus-device RS485 on IO4/IO5. **Bus-device RS485 and single-wire TTL share IO4 TX / IO5 RX**, including the UART data and baud rate. They are not independent serial ports; avoid duplicate IDs and reply collisions when both are connected. The CAN/RS485 selector does not affect this device-bus GPIO pair.

The CAN example uses 500 kbps and needs a matching peer and common ground. The board has a 120Ω CAN termination resistor; account for existing termination across the entire network. The external RS485 example uses 115200 8N1 and automatic hardware direction control. Coordinate half-duplex access between peers. See [Arduino examples](arduino.md). Switching to 485 does not automatically create a factory JSON gateway.

## User Interface Module (A)

Connect this external module to PH1.25-10P for five buttons, buzzer alerts and OLED display. These components are on the module, not onboard RoboCore S3.

| Function | GPIO / settings |
| --- | --- |
| Up / Down / Left / Right / OK | IO10 / IO11 / IO12 / IO9 / IO2, active low |
| Buzzer | IO21 |
| OLED | SDA6 / SCL7, SSD1306 128×32, address 0x3C |
| Onboard IMU | Same I²C bus; driver probes 0x6A / 0x6B |

The `User_Interface_Module_A` sketch debounces keys, beeps briefly and displays the pressed key. Buttons remain available if the OLED is missing. Initialize Wire once and keep its pins consistent across OLED and onboard IMU drivers.

## Download and USB communication

- **Type-C UART:** the automatic download circuit uses DTR/RTS from the tool; normally no manual BOOT sequence is needed.
- **Type-C USB:** to enter download mode manually, hold BOOT, press and release RESET, then release BOOT and select the enumerated USB port.
- **Normal operation:** native USB CDC exchanges JSON with a host and avoids the UART bridge bottleneck. Actual throughput depends on firmware and host processing; it is not an actuator execution rate.

Use a data-capable cable, identify the actual port and close competing serial monitors. See [firmware recovery](firmware.md).
