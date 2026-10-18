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

On the Onboard IMU page, use Read once or enable live display, then gently tilt and rotate the board. At rest, acceleration magnitude is about 1 g and angular velocity is near zero. Tilting changes the acceleration components; rotation changes angular velocity. The 3D view is a visual estimate, and relative heading drifts over time.

Send `{"T":740}` to inspect raw feedback. Use measurements only when `ok:true`. The wrapping 24-bit `sample_count` identifies samples; equal counts mean the same sample. `sample_age_ms` is the cache age. Readings older than 100 ms return `imu_no_fresh_sample`.

If an error occurs, retain the complete reply, check power, and inspect shared I²C peripherals such as the external OLED with power disconnected. `imu_not_present` means the sensor was not detected; `imu_reset_failed` or `imu_config_failed` indicates initialization failure. Live display pauses when you leave the page or hide the browser tab; enable it again when you return.
