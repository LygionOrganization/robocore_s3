# Troubleshooting

## Cannot open the console

Stay on the Robot AP and enter http://192.168.4.1 directly. Prevent automatic network switching. In router mode use the assigned DHCP address. A USB cable alone does not establish Wi-Fi.

## No IDs found

Check external power, common ground, TTL wiring and baud rate. The controller baud setting does not reprogram device baud rates. Configure duplicate IDs one device at a time; scanning cannot identify duplicates or product types.

## Stepper does not move after changing mode

Mode configuration disables output; enable deliberately afterward. Modes 0 and 1 switch without restart. Only phase configuration changes require restart. Match the position/speed command to the current mode and check current/limit settings.

## onboard IMU absent

Check the onboard sensor and shared I²C wiring, including attached expansion modules. Firmware reports the unavailable sensor and continues operating.

## USB sends but receives nothing

Use native USB CDC, newline termination and at most 255 bytes per command. Close other serial monitors and continuously read replies. Re-enable USB output if T=604 disabled it. Ignore non-JSON logs.

## Timeouts or skipped commands

Only one pending command is stored, so newer commands intentionally replace older ones. Exit an infinite mission with T=0 first. Multiple clients compete for the mailbox and latest result. Read state before retrying configuration writes.

## Stop loop did not stop the motor

T=0 exits a mission; it does not disable motor drive. Set continuous speed to zero or disable output. A mechanical emergency stop requires a suitable system-level design.

## UI disappeared after uploading a sketch

A unit sketch replaces factory firmware. Follow [recovery](firmware.md). Missing headers during compilation usually mean the required library or correct author/version is not installed; see [Arduino dependencies](arduino.md).


## IMU readings do not update

Firmware 1.3 resets the QMI8658A at startup, verifies configuration and checks the sensor sample counter. Successful T=740 replies include `sample_count`, which advances between readings and wraps at 24 bits. No fresh sample within 300 ms returns `imu_data_timeout`, rather than a successful stale reading. `imu_reset_failed` and `imu_config_failed` identify reset or configuration readback failures.

The web button reads once. Click again after tilting the board. Near-zero gyro values at rest are normal; watch acceleration change with tilt or read angular rate during rotation. If it still fails, retain the complete T=740 response and check power and shared I²C peripherals.
