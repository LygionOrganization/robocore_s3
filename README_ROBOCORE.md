# RoboCore S3

项目仓库：[robocore_s3](https://github.com/LygionOrganization/robocore_s3)。同一套固件也适用于 Robot Driver with ESP32S3 Lite；Lite 的供电、接口与外设请以 [Lite Wiki](https://wiki.lygion.ai/reference/bus-devices/robot-driver-with-esp32s3-lite/) 为准。

完整下位机固件：中英双语网页控制、HTTP/WebSocket/USB JSON、任务脚本与板载 QMI8658A。网页默认英文，可在右上角切换中文。

## 编译完整固件

1. 安装 VS Code + PlatformIO，打开本项目根目录。保留 `src`、`include`、`lib` 和 `platformio.ini`，不能只复制主程序。
2. 使用仓库固定的平台与库版本，等待首次依赖下载完成。原生 USB 使用 TinyUSB（USB_MODE=0）。不要自行定义 `tud_cdc_rx_cb`。
3. 在 PlatformIO 终端运行 `pio run`。若首次平台安装出现 esptool 元数据/找不到命令错误，参照 [验证说明](docs/ROBOCORE_VALIDATION.md) 修复 PlatformIO Python 环境后重试。
4. 修改网页源文件 `web/index.html` 后先运行 `python tools/embed_web.py`，再编译。

## 使用与开发资料

- [JSON 接口与指令表](docs/ROBOCORE_API.md)
- [18 个 Arduino 单元例程及依赖](arduino_example/README.md)
- [Python HTTP / WebSocket / USB 客户端](python_example/robocore/client.py)
- [验证结果与实机验收清单](docs/ROBOCORE_VALIDATION.md)
- [交给 AI 的 AGENTS.md 内容](docs/ROBOCORE_AI_AGENTS.md)
- [USB CDC 压力测试](test/USB_CDC.md)

成功构建后可运行 `python tools/package_robocore_firmware.py`，生成 `.pio/robocore-firmware.zip`，包含同一次构建的三个镜像、烧录偏移、SHA-256 和依赖配置。发布前需完成实机验收。Wiki 教程的本地文件位于 lygion-wiki 仓库 `docs/reference/bus-devices/robocore-s3/`。

RoboCore S3 输入为 5–48V，动力总线电压等于输入。GH1.25-3P 用于板间 TTL 通信、供电解耦。
