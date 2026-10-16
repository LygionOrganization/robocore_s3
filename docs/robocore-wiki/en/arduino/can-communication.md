# External CAN communication

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Set selector to CAN. Connect another 500 kbps node through CAN 2+2, matching H/L and common ground; RX13/TX14. Account for onboard 120Ω termination when arranging the two bus ends.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/CAN_Communication/CAN_Communication.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

No additional library is needed. `driver/twai.h` comes with the esp32 board package. If a header is missing, verify the selected ESP32S3 board and installed core instead of adding an unrelated library.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Send `t` to transmit standard ID 0x100 with bytes 0x12 0x34 and inspect Transmit result. Incoming peer frames print CAN ID, DLC and payload bytes. A successful transmit call means submission, not confirmed application receipt; verify at the peer.

## Troubleshooting and next steps

If transmission fails, check selector, baud, H/L, ground, termination and an active acknowledging peer. GPIO13/14 are not CAN H/L terminals.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// No external libraries required; all headers are included with the ESP32 board package.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <driver/twai.h>
bool ready = false;
void setup() {
  Serial0.begin(115200);
  // Set the physical selector to CAN. CAN and external RS485 share RX13/TX14.
  // GPIO38 drives the CAN transceiver slope-control input; LOW selects normal speed.
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);
  twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_14, GPIO_NUM_13, TWAI_MODE_NORMAL);
  twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  ready = twai_driver_install(&general, &timing, &filter) == ESP_OK;
  if (ready) ready = twai_start() == ESP_OK;
  Serial0.println("Selector: CAN. Use CAN 2+2, another 500 kbps node and common ground.");
  Serial0.println("This board has 120 ohm termination. Check termination across the full bus.");
  Serial0.println("Send t to transmit ID 0x100 with two bytes.");
}
void loop() {
  if (!ready) { delay(100); return; }
  if (Serial0.read() == 't') {
    twai_message_t tx = {};
    tx.identifier = 0x100; tx.data_length_code = 2; tx.data[0] = 0x12; tx.data[1] = 0x34;
    Serial0.printf("Transmit result=%d\n", twai_transmit(&tx, pdMS_TO_TICKS(100)));
  }
  twai_message_t rx = {};
  if (twai_receive(&rx, 0) == ESP_OK) {
    Serial0.printf("CAN ID=%lx DLC=%u", rx.identifier, rx.data_length_code);
    for (int i = 0; i < rx.data_length_code && i < 8; ++i) Serial0.printf(" %02x", rx.data[i]);
    Serial0.println();
  }
  delay(1);
}
```
