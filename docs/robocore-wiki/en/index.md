# RoboCore S3

RoboCore S3 is an ESP32-S3 robot controller. Its factory firmware lets you discover bus IDs, control servos and steppers, configure TTL Node modules and read encoders from a phone or desktop browser without writing code.

## Product specifications

| Item | RoboCore S3 |
| --- | --- |
| Size / mounting | 47 × 52 mm; four Ø3 mm holes, 40 × 45 mm center spacing |
| Controller | ESP32-S3, 8 MB flash and 8 MB PSRAM |
| Input power | 5–48V through XT30; powered bus outputs follow input |
| Device bus | Single-wire TTL and bus-device RS485, sharing IO4 TX / IO5 RX |
| External communication | CAN or RS485 selected by switch, sharing IO13 RX / IO14 TX |
| Sensing / expansion | Onboard QMI8658A; external User Interface Module (A) support |
| Development | Open-source PlatformIO and Arduino IDE examples |

See [power and connectors](hardware.md) before wiring.

!!! danger "Bus power is not regulated down for each device"
    A 48V-rated controller does not make low-voltage servos compatible with 48V. Match the supply to every powered bus device.

## Choose a path

- No coding: [Quick start](quickstart.md), [Web console](web-console.md), [Bus devices](devices.md).
- Host programming: [JSON API](json-api.md), [PC / host control](python.md).
- Custom firmware: [Arduino examples](arduino.md), [AI-assisted development](ai-development.md).
- Restore the complete software: [Firmware recovery](firmware.md).

[Get started](quickstart.md){ .md-button .md-button--primary }
[Example download](../../../assets/files/robocore-s3/examples.zip){ .md-button }

Sources: [firmware](https://github.com/LygionOrganization/robocore_s3), [TTL SDK](https://github.com/LygionOrganization/lygion_devs_cpp), [wiki](https://github.com/LygionOrganization/lygion-wiki).

[TTL 6DOF IMU: live feedback and ID configuration](json-api.md#ttl-6dof-imu)
