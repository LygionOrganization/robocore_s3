# 构建与验证 / Build and verification

## Build

Install PlatformIO and run from the repository root:

```sh
python tools/embed_web.py
pio run
```

`platformio.ini` pins pioarduino 55.03.30 (Arduino ESP32 3.3.0) and library versions.
Keep `src`, `include`, `lib`, `web` and `tools` with the configuration. The Windows
Python tool-path helper `tools/pio_python_path.py` is part of the build.
Outputs go to `.pio/build/esp32-s3-devkitc-1-n8r8/`.

For a first-install esptool failure, check network access and the PlatformIO Python
environment before changing firmware. On Windows the managed interpreter is normally
`%USERPROFILE%/.platformio/penv/Scripts/python.exe`. Install `esptool==5.0.2` there
if that dependency is missing. Do not replace the Arduino core USB callback.

## Automated checks

| Command | Coverage |
| --- | --- |
| `python test/run_robocore_tests.py` | Production IMU/bus handlers with fake I2C and serial; decode, faults, stale data, bounded output and snapshot stress |
| `python test/run_usb_cdc_tests.py` | Production USB framing, size limits, overflow recovery and latest-wins bursts |
| `python test/run_mission_tests.py` | Mission storage and command behavior |
| `python tools/check_examples.py` | Compile all standalone Arduino examples with the configured toolchain |
| `node test/run_web_tests.cjs` | Bilingual UI and responsive controls |
| `node test/web_imu.cjs` | Attitude visualization, sequential requests, pause and response matching |
| `node test/web_connectivity.cjs` | Connection handling |
| `node test/web_imu_soak.cjs` | Seven-minute simulated live-IMU workload |

Host C++ tests require the compiler selected by their runner (MSVC on Windows).
Browser tests require Node.js and Playwright (`npm install --no-save playwright`),
a Chromium installation, and `python tools/preview_web.py` running at
`http://127.0.0.1:8765`. Set `CHROME_PATH` to a local Chrome executable or install
Playwright Chromium with `npx playwright install chromium`.
Tests use simulated devices and never prove electrical behavior or hardware endurance.

## 实机验收 / Hardware acceptance

对每个发布候选版本记录板型、固件版本、供电、外设、测试时长和结果：

1. 启动与 AP/STA、网页语言、USB 连接；无 IMU 板型也应正常启动。
2. IMU 静止、旋转、单次与实时读取；同时连接 OLED，连续运行至少 30 分钟。
3. 低频命令逐条等待反馈；高频输入、暂停读取、断开重连后仍能恢复正常操作。
4. 各总线设备反馈与受控运动、步进停转及使能状态、CAN/RS485/ESP-NOW。
5. 备份配置后，用同次构建的三个镜像验证烧录与恢复。

保留必要的错误反馈、设备状态和 `/api/info` 的运行时间、堆内存与重启原因用于维护。
不要把软件测试通过表述为硬件测试通过；发布构建需独立验收。
