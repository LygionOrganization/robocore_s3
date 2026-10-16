# RoboCore S3 delivery progress

User scope: shared firmware for RD Lite and RoboCore S3; optional QMI8658A; modern responsive embedded web UI with bus scan, TTL Node A, TTL Stepper A, Encoder E02; JSON over HTTP/WS/USB; Arduino examples with English source comments; beginner-to-advanced Chinese/English wiki series and AI AGENTS.md.

Updated user confirmation: use the schematic for connector specifications. Servo RS485: XH2.54-4P. GH1.25-3P: board-to-board TTL communication and power decoupling. User confirmed with a schematic close-up that CN4 pin 2 itself is unconnected (NC), not PVCC. No cable modification instructions are needed. Input 5–48V; bus power follows input. Dimensions 47 x 52 mm, four 3 mm holes at 40 x 45 mm center spacing.

Repositories:
- Firmware: D:/Backup/Documents/PlatformIO/Projects/robot_driver_with_esp32s3_lite
- Wiki: G:/lygion/产品资料包/lygion-wiki (MkDocs bilingual; clean at start). User explicitly authorized editing. Tool sandbox may require escalation for G: writes.
- Firmware origin: https://github.com/LygionOrganization/robocore_s3
- SDK: https://github.com/LygionOrganization/lygion_devs_cpp

Preserve user's preexisting lib/lygion_devs changes and earlier USB EVENT/latest-wins implementation. No agents spawned (user has not asked for delegation).

Automatic follow-up: heartbeat id `robocore-s3`, hourly, current chat; disable upon completion. Useful for quota-reset continuation; not a guarantee of exact reset timing.

## Status
- [x] Inspect firmware, SDK, product docs; establish hardware pins and voltage.
- [x] Optional IMU module and validated device JSON API, result routing.
- [x] Bus scan and modern responsive web control/settings/JSON UI.
- [x] Build and regression tests, browser desktop/mobile verification.
- [x] Arduino examples and Python transport examples, dependency instructions.
- [x] Wiki bilingual tutorial series, navigation, downloadable AI guide.
- [x] Validate examples/docs; record remaining hardware-only tests.

## Findings
- Serial1 RX5 TX4; I2C SDA6 SCL7; CAN TX14 RX13. Bus baud defaults 1Mbps.
- TTLSDClass exists locally, SetMode exists; old wiki incorrectly refers to C++ WheelMode (do not copy).
- Stepper mode 0 position / 1 speed / 2 follower. USER CORRECTION: modes 0 and 1 can switch freely without reboot; only phase configuration changes require reboot. Correct existing Chinese AND English stepper wiki pages, not just new tutorials. Follower also needs secondary ID matching leader (verify register in wiki memory-map).
- Stepper position 0–65534, speed signed magnitude -32767..32767, acceleration 0..254, current 0..1000. Current and speed are register units, not A or steps/s.
- E02 uses TTLSDClass ReadPos/ReadSpeed/CalibrationOfs, position 0–65534, 4096 counts/turn. Calibration persistent, multiturn count not persistent.
- Node PWM channels 1–2 0–1000; RGB packing implemented in SDK, use its accepted ranges after verifying examples; S.BUS 16 channels, read 33 bytes.
- Existing feedback mostly USB only. New commands need HTTP result polling and WS routing; accepted != executed, single pending command may be overwritten.
- QMI8658A WHO_AM_I 0x05 at addresses 0x6A/0x6B. Probe with finite timeout, no fatal loop if absent. Share Wire, do not reinitialize OLED bus.
- Previous USB work built successfully under pioarduino 55.3.30 / Arduino 3.3.0; host tests `python test/run_usb_cdc_tests.py` pass.

Update this file at milestones so another turn can continue without repeating work.


## Final verification / 2026-10-14
- Firmware build passed (Arduino 3.3.0 / pioarduino 55.3.30), 16/16 unit examples compiled. Final USB adapter and pinned-platform build also passed.
- USB RX host tests passed: framing, overwrite queue, stop, overflow recovery, 200,000 commands.
- Production BusDevices host tests passed: absent/present/failing IMU, ID collision/change, validation, SBUS, mode 0/1/follower, signed feedback. Found and fixed commented-out return in STEPPER_MODE.
- Added UsbOutput.h to bound application USB output to available FIFO space and 10ms per write; all application Serial print/write calls routed through it, USB TX mutex timeout=0. Host fake asserts no oversized writes, timeout and newline recovery. This addresses core3.3.0 infinite TX loop when host stops draining.
- Removed Wi-Fi credentials from shared rid result publication.
- Wiki installed to authorized G: repo; strict bilingual build passed after excluding downloadable AGENTS.md from nav.
- Desktop/mobile browser checks used localhost mock only: scan IDs, selected ID propagation, SBUS16 channels, absent IMU response. Screenshot .pio/robocore-ui.png.
- Final artifacts: .pio/robocore-firmware.zip (images, offsets, SHA256), docs/ROBOCORE_VALIDATION.md, README_ROBOCORE.md. Wiki downloads refreshed. Preview stopped. Final pinned-config examples: 16/16 PASS. Heartbeat paused after completion. No flashing, live movement, Git commit/push or public publication.
- Hardware acceptance remains user-side: sustained USB flood with reader and without reader, both board variants startup, physical QMI axes/scaling, bus Node/Stepper/E02 and CAN/ESP-NOW.

- Pinned platform 55.03.30 and library versions. Fixed upstream Windows Scripts/Scripts tool lookup with tools/pio_python_path.py; repaired local esptool through official PyPI 5.0.2. Final complete firmware build SUCCESS.

## Completed
All authorized software/documentation deliverables are implemented and verified locally. Final firmware build, 16/16 examples, host regressions, and updated wiki strict build passed. Wiki recovery download now includes this build with hardware-validation status explicitly false. Automatic follow-up paused. Physical acceptance and public publishing remain outside completed local validation.
