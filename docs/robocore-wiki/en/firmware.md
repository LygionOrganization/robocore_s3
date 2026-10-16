# Restore complete firmware

Use this package to restore the web console, bus-device control and JSON communication after uploading a unit sketch.

## Download and prepare

1. Download the [complete Windows download tool](../../../assets/files/robocore-s3/download-tool.zip) and extract it locally. It includes matching firmware images, preset addresses and bilingual instructions.
2. Back up missions and network settings you wish to keep. Disconnect actuators and motor power, then connect the board over USB.
3. Optionally run `Verify_Package.ps1` to check SHA-256 hashes. This check does not connect to or modify the board.
4. Open `Start_Download_Tool.bat`, choose ESP32-S3, Factory and UART, then manually select the board’s actual COM port.

## Flash through Type-C UART

1. Connect a data cable to **Type-C UART**, identify its port in Device Manager, and close any serial monitor or other application holding that port.
2. Ensure all three image paths exist and are checked, and verify these offsets:

| Image | Offset |
| --- | --- |
| bootloader.bin | 0x0 |
| partitions.bin | 0x8000 |
| firmware.bin | 0x10000 |

3. Choose COM and baud (start with 921600; lower it if unreliable), click START, wait for completion, and press RESET.
4. Connect the external antenna before wireless use. Preserved settings retain your previous network; a fresh configuration uses hotspot **Robot**, password **12345678**, at `http://192.168.4.1`.
5. Check the console, language switch and onboard IMU status. Reconnect bus devices and rated motor power, scan IDs, then begin control.

Type-C UART supports automatic download without normally pressing BOOT. If connection fails, hold BOOT, press/release RESET, release BOOT, and retry.

## Images and saved settings

Do not write an ordinary `firmware.bin` at 0x0. Use three images from the same build; the offsets above apply to this package. Custom partitions or merged images require their own addresses.

**ERASE deletes saved missions, network settings and other data.** Use it only for an intentional full reset after backup, then write all images. Ordinary reflashing may preserve the filesystem and therefore may retain existing Wi-Fi settings.

If you already have a compatible flashing utility, download just the [firmware image archive](../../../assets/files/robocore-s3/firmware.zip). To modify source, obtain the [complete firmware project](https://github.com/LygionOrganization/robocore_s3) and build with PlatformIO; images appear in `.pio/build/esp32-s3-devkitc-1-n8r8`.
