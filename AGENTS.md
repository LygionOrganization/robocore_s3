# RoboCore S3 development guide for AI agents

## Start here

This is firmware for a low-level device controller, not a complete robot application.
Keep perception, planning and application-specific behavior on the host where possible.
Read `README.md` (English) or `README.zh-CN.md` before changing code. This file applies
to the whole repository; preserve existing customer changes and follow the user's scope.

| Area | Source of truth |
| --- | --- |
| Build, board options, pinned dependencies | `platformio.ini`, `src/Config.h` |
| Startup, HTTP/WS routes, command dispatch, missions | `src/main.cpp` |
| Validated bus-device commands and IMU snapshots | `src/BusDevices.h`, `src/BusDevices.cpp` |
| QMI8658A driver | `lib/RoboCoreSupport/src/OptionalQMI8658.h` |
| TTL device SDK / servo SDK | `lib/lygion_devs/`, dependencies in `platformio.ini` |
| Bus/I2C ownership and bounded USB output | `include/BusGuard.h`, `include/I2cGuard.h`, `include/UsbOutput.h` |
| OLED, CAN, ESP-NOW | `src/ScreenCtrl.cpp`, `src/Can.h`, `src/Wireless.cpp` |
| Web UI, attitude visualization, JSON tools | `web/index.html`, `web/imu-view.js`, `web/json-tools.js` |
| Command catalogue and bilingual API | `docs/command-catalog.json`, `docs/ROBOCORE_API*.md` |
| Standalone examples / host clients | `arduino_example/`, `python_example/robocore/` |
| Regression checks and release instructions | `test/`, `docs/ROBOCORE_VALIDATION.md`, `docs/RELEASING.md` |

## Data flow and stability requirements

HTTP `/api/cmd`, WebSocket `/ws`, and USB CDC feed the shared command ingress.
`src/main.cpp` executes commands and publishes results; `/api/result` is only the
latest result, not a result queue. Read current route definitions before adding clients.
Match `rid` and `ok`; a transport acknowledgement is not device execution success.

An independent IMU task initializes and samples the sensor. It publishes a fixed-size
snapshot under a short mutex. `T=740` and read-only `GET /api/imu` consume that snapshot;
network handlers must not wait for sensor initialization. The live web view uses
sequential requests and pauses when hidden. Reject stale samples rather than presenting
old measurements as live. `sample_count` is a wrapping 24-bit sensor counter;
`sample_age_ms` is host cache age. Acceleration is in g, angular velocity in degrees/s,
temperature in degrees C. Six-axis heading is relative and drifts.

Keep the tested SyncSample handshake, AHB clock configuration, GZ_H unlock and shared
400 kHz I2C clock. Lock waits and recovery attempts must stay bounded. Never name a
decoder `word(...)`: Arduino defines this as a macro; use `decodeInt16LE` instead.
Do not use the simpler standalone sketch's reading strategy as an untested replacement
for the concurrent firmware driver.

Reliability takes priority over executing every command during bursts. Keep one pending
command, bounded input buffers, bounded output, scheduler yields, and WebSocket limits.
Do not add unbounded queues, automatic write retries, or two-slot command architectures.
At low traffic rates, valid commands must execute normally. Under congestion, drop excess
work instead of blocking sensor, USB or network callbacks. Never hold a snapshot mutex
while doing I2C, JSON serialization, delays or network output. Use unsigned elapsed-time
arithmetic across timer wraparound.

Stepper acknowledgement bytes are not the driver fault register. In `T=723`, actual
register 0x41 distinguishes faults from current limiting; status 16 alone is normal.
Do not restore the old rule that any nonzero status is a fault.

## Common development workflows

- Add a device command: check its SDK/register map, validate inputs in the handler,
  add a host regression, then update the reviewed `docs/command-catalog.json`
  (the catalogue generator preserves existing reviewed entries), run
  `python tools/build_api_reference.py`, and regenerate the embedded web page.
- Edit the UI: change the source in `web/`, including both languages. Run
  `python tools/embed_web.py`; do not hand-edit generated `src/Web_page.h`.
- Build: `pio run`. Run applicable checks from `docs/ROBOCORE_VALIDATION.md`.
- Release: follow `docs/RELEASING.md`; generated outputs belong in `.pio/`.
- Arduino examples: keep English dependency instructions in each `.ino`. Use Serial0
  at 115200 through Type-C UART unless the example specifically teaches native USB CDC.
  The standalone sketch replaces the complete firmware. Preserve the working QMI example.

Public artifacts must exclude local logs, credentials, build caches and internal work
diaries. Keep useful API errors, freshness information and hardware faults; do not add
investigation counters to routine responses without an application requirement.

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
