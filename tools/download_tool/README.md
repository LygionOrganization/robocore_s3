# RoboCore S3 Firmware Download Tool

## Update: robocore-1.3.6

Fixes an Arduino word(...) macro collision in IMU decoding. Calls to a local word(offset) decoder expanded to makeWord(offset), returning offsets 0,2,4,6,8,10,12 instead of sensor values. Renamed the decoder to decodeInt16LE. The standalone sketch is unchanged.

Added the real Arduino macro to host tests and verified failure before the fix and success after it. Retains locked sampling and STA log deduplication from 1.3.5. Confirm version 1.3.6 and validate changing readings on hardware.

## Update: robocore-1.3.5

Uses QMI8658A SyncSample locking per datasheet section 13, including disabling AHB clock gating through CTRL9, STATUSINT handshake, and automatic unlock on GZ_H read. Keeps tested ranges, 112.1 Hz ODR, 400 kHz I2C and 100 ms freshness limit. The standalone Arduino sketch is unchanged. Verify the new locking path on hardware.

HTTP result polling accepts only the current rid and identical responses are deduplicated, preventing repeated stale STA messages. IMU error JSON is shown in the page. Verify sampling_mode=sync_locked, increasing samples_acquired, low sample_age_ms and no repeated sensor_restarts.

## Update: robocore-1.3.4

Fixes status 16 (automatic current limiting) being classified as a device fault. Reads driver register 0x41 separately and reports current limiting, limits and target range independently of voltage/thermal/overcurrent faults. Stepper acknowledgements are checked for communication errors.

IMU now samples continuously in a dedicated task after peripheral setup. T740/web reads a snapshot no older than 100 ms, with sample_age_ms, samples_acquired and sensor_restarts. Failures report read_state and before/after counters; stale values are never returned as live data. Hardware verification is still required.

## Update: robocore-1.3.3

Keeps shared OLED/IMU I2C at 400 kHz, serializes complete accesses and retries transient IMU snapshots up to four times without returning cached data. Stepper/encoder feedback now separates communication failures from device status, returning valid position/speed alongside device_status, device_fault and warning. Output is not disabled and faults are not cleared.

Verify single/live IMU reads during OLED updates and stepper reads after motion while enabled on hardware. Preserve the full JSON response when device_fault is true.

## Update: robocore-1.3.2

Adds live 3D IMU orientation, roll/pitch/relative heading, heading reset and pause. Leaving the page or hiding the tab stops polling. Six-axis heading drifts; motion and network latency limit accuracy. Visualization only, with no external web dependencies. Refresh the web page after flashing and verify orientation and updates on hardware.

## Update: robocore-1.3.1

Ports the board-tested QMI-FINAL-1 initialization and reads: 400 kHz I2C, 112.1 Hz ODR, individually addressed counter bytes and a 14-byte measurement burst. Removes status-bit waiting and 300 ms retries. Duplicate or interrupted samples return imu_data_not_ready; retry later.

Confirm fw_ver is robocore-1.3.1 after flashing. Change board orientation and read the web IMU page repeatedly, or send {"T":740}. Check that sample_count advances and accel_g / gyro_dps change. Integrated firmware still requires hardware validation; no bias calibration or attitude fusion is included.

**English** | [简体中文](README.zh-CN.md)

Windows package for **RoboCore S3 and Robot Driver with ESP32S3 Lite**. Includes the existing Espressif Flash Download Tool 3.9.9 R2 and the complete firmware built from the current `robocore_s3` source tree. No Python or PlatformIO installation is needed to use this package.

The firmware includes the English/Chinese web console, bus-device controls, JSON host interfaces and optional IMU detection. Lite runs normally without an onboard IMU. Hardware power ratings remain board-specific.

## Flash the board

1. Extract the entire ZIP to a local folder. Do not run the executable inside the ZIP. Back up saved tasks and settings. Disconnect actuators and motor power, then connect the board's **Type-C UART** port with a data cable.
2. Open `Start_Download_Tool.bat`. It sets the correct working directory and opens the tool; it does not flash automatically.
3. Select **ESP32-S3 / Factory / UART**. Select the board's actual COM port. The package does not preselect a COM port.
4. Check that these three rows are selected. Paths are relative to the extracted tool folder. If your tool opens with blank paths, select the files manually:

| Image | Address |
| --- | --- |
| `bin/bootloader.bin` | `0x0` |
| `bin/partitions.bin` | `0x8000` |
| `bin/firmware.bin` | `0x10000` |

5. Keep the SPI settings from the images (DoNotChgBin enabled). The target has **8MB flash**. Select **115200 baud** for an initial download; 921600 may be used with a stable connection.
6. Click **START**, wait for the tool to report success, then press RESET. Do not disconnect USB during programming.
7. Join **Robot**, password **12345678**, and open **http://192.168.4.1**. Existing saved network settings may remain; use those if the defaults do not appear. The main console defaults to English and has a Chinese language button.

If automatic download fails, hold BOOT, press and release RESET, then release BOOT and retry. Native Type-C USB requires the manual boot sequence; select the newly enumerated port. Type-C UART is the recommended path for this package.

**ERASE removes saved Wi-Fi settings, tasks and other flash data.** Use it only when a full reset is intended and you have backed up what you need. A normal three-image update does not intentionally erase the entire flash. Never write the standalone `firmware.bin` at `0x0`.

## Package identity and verification

`manifest.json` records the build time (UTC+08:00), source commit, whether local changes were present, source hashes, partition layout, image addresses and SHA-256 hashes. A dirty source tree is identified explicitly; the commit alone does not identify that build.

Before first use, you can run this read-only check in PowerShell from the extracted directory:

```powershell
powershell -NoProfile -File .\Verify_Package.ps1
```

Or compare `Get-FileHash -Algorithm SHA256` results with `SHA256SUMS.txt`. These hashes check package consistency, not publisher authenticity. The vendor application may update its configuration after use; the verification script checks the original packaged files.

Use the three matching images together. After flashing, press RESET, reconnect to the board and check its web console before reconnecting and controlling actuators.

## Updates and documentation

Replace the complete package when updating. This release does not include the old single-image online updater, previous device logs or cached firmware.

- [Source repository](https://github.com/LygionOrganization/robocore_s3)
- [RoboCore S3 Wiki](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/)
- [Lite Wiki](https://wiki.lygion.ai/en/reference/bus-devices/robot-driver-with-esp32s3-lite/)

