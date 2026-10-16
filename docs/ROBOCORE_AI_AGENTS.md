# RoboCore S3 development guide for AI agents

## Authoritative project resources

All Lygion project resources are maintained under https://github.com/LygionOrganization/ . Start with:

- Firmware: https://github.com/LygionOrganization/robocore_s3
- Wiki: https://github.com/LygionOrganization/lygion-wiki
- TTL SDK: https://github.com/LygionOrganization/lygion_devs_cpp

Read the checked-out versions, not remembered APIs. In the firmware, inspect `src/Config.h`, `src/BusDevices.h/.cpp`, `docs/ROBOCORE_API.md`, `web/index.html`, `arduino_example/README.md` and the relevant example. In the wiki, inspect `docs/reference/bus-devices/robocore-s3/` and the connected device's tutorial and memory map. Never invent product dimensions, connector order, register addresses, library functions or firmware download URLs.

## Hardware contract

- RoboCore S3: 5–48V input. Powered bus connectors pass through the input voltage; there is no per-device voltage conversion.
- Bus-device RS485 and single-wire TTL share UART1 RX5/TX4. External CAN and RS485 share RX13/TX14, selected by the physical switch (CAN 2+2 versus GH1.25-5P). External RS485 is independent of the device bus.
- I2C SDA6/SCL7 serves the onboard IMU and external OLED. UI Module (A): keys Up10/Down11/Left12/Right9/OK2, buzzer21. OLED/buttons/buzzer belong to the external module. There are no onboard addressable RGB LEDs.
- Board dimensions: 47 x 52 mm; four 3 mm mounting holes at 40 x 45 mm center spacing.
- GH1.25-3P is for board-to-board TTL communication and power decoupling. CN4 pin 2 is unconnected (NC), not connected to PVCC. Public documentation should describe the purpose only, without cable modification instructions. Bus-device RS485 uses XH2.54-4P.
- RoboCore is not a Raspberry Pi HAT and does not supply Raspberry Pi power.
- QMI8658A is onboard. Probe 0x6A/0x6B with a bounded timeout; WHO_AM_I is 0x05. Missing hardware must never halt startup. Share the existing Wire bus with the OLED.

## Software contract

- Describe RoboCore S3 as an independent product. Do not add comparisons with other controller boards.
- Private design reference documents must never be copied into repositories, downloads or public documentation. Document production features only.
- USB reception uses `USBCDC::onEvent`, not a replacement `tud_cdc_rx_cb` and not loop polling. Callbacks never execute motor commands or print logs.
- Newline-delimited UTF-8 input, maximum 255 bytes excluding newline. Discard an entire oversized/damaged line and resynchronize at newline.
- One pending command, latest wins. Do not add separate reliable/high-frequency command queues. The active command completes; later pending commands overwrite earlier ones.
- T=0 is an out-of-band loop exit request, not an electrical emergency stop.
- Preserve runMission's intentional infinite-loop support. Protect individual physical bus operations, never hold a bus mutex across an infinite mission.
- HTTP 202 means accepted, not executed. Match responses by integer `rid`; no automatic retries of configuration writes. `/api/result` stores only the most recent published result.
- New device commands are validated in `BusDevices.cpp`. Do not allow silent integer wrapping, broadcast EEPROM writes or false success on missing replies.
- Stepper modes 0 and 1 can switch freely during a single power session without restart. Only phase configuration changes require restart. Mode writes disable output; enable deliberately afterward. Follower mode uses a distinct primary ID plus a secondary ID matching the leader.
- Use local JSON documents in concurrent tasks. Keep receive buffers and UI logs bounded. SDK methods and register units must match the checked-in library.

## Changes and verification

Preserve user changes. Keep `.ino` comments in English and explain dependencies in the README. A unit sketch replaces the complete firmware; never imply it preserves the factory UI.

Edit `web/index.html`, then run `python tools/embed_web.py`. The embedded page must work offline without CDN assets. Check desktop and phone widths, input validation, timeouts, connection loss, scan selection and absent IMU states.

Build with `pio run`; do not upload, erase, calibrate or move real hardware without an explicit instruction to perform that physical action. Run `python test/run_usb_cdc_tests.py`, applicable device tests and `python tools/check_examples.py`. Report exactly which checks ran. Mock tests do not demonstrate motor direction, electrical compatibility, sensor accuracy or long-term stability.

For docs, update both Chinese and English navigation and build MkDocs. Keep output artifacts separate from source. Do not publish, push, or substitute untested factory binaries merely because a document references them.

When asking an AI to implement a project, provide the board, connected devices, IDs, baud rate, power supply, desired behavior and acceptable stop behavior. First request a read-only scan/feedback test, then a bounded motion test.
