# RoboCore S3 Firmware Download Tool

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

