# QMI8658A Standalone Example: QMI-FINAL-1

[English](README.md) | [简体中文](README.zh-CN.md)

## Setup

1. Open `QMI8658A.ino` in this folder. Install **esp32 by Espressif Systems 3.3.0** in Arduino Boards Manager and select **ESP32S3 Dev Module**.
2. Set **USB CDC On Boot = Disabled**, upload through **Type-C UART**, and set the Serial Monitor to **115200**. Uploading replaces the program currently on the board.
3. No extra library is required; this example does not use RoboCoreSupport. Dependency instructions are also provided in English at the top of the `.ino` file.
4. On startup, the monitor shows `QMI-FINAL-1` and `IMU ready`. Acceleration changes when the board's pose changes, angular velocity changes when it rotates, and the acceleration magnitude should be near 1 g while still.

## Sampling

The example uses 400 kHz I²C, repeated START, a 14-byte continuous read, and a sensor output rate of 112.1 Hz. Ranges are ±8 g and ±512 °/s. SDA=GPIO6, SCL=GPIO7; the sensor is detected at 0x6A or 0x6B.

Register dumps, serial commands, long raw-data logs and the 350 ms retry loop have been removed. The sketch checks for a new sample about every 2 ms and prints the latest result every 200 ms; print rate is not the sample rate. It compares the sensor counter before and after reading and discards data if the sensor updated during the read. An I²C error or no new sample ends the attempt immediately and leaves the previous data untouched. A notice appears after 100 ms without new data.

Process data only when `readImu(sample)` returns true. A control-code entry point is left there. Units are g, °/s and °C; `count` is the 24-bit sensor sample counter and `receivedUs` is the time the controller received the data. Estimate the sensor interval with `(newCount - oldCount) & 0xFFFFFF` divided by 112.1; initialize the first sample separately.

## For further control development

- Set `PRINT_IMU=false` to disable periodic output. Printing is skipped when the serial buffer is too small, avoiding waits. I²C remains synchronous and is limited by a 20 ms timeout on errors; this is not a hard real-time interface.
- The default ODR remains the measured 112.1 Hz and does not guarantee that every frame will be read. Validate higher ODRs, actual read frequency and the control period on the board.
- Control software must handle missing fresh data and must not treat cached samples as new feedback. A balancing robot also needs gyroscope bias calibration and sensor fusion; this example does not calibrate automatically or control motors.
