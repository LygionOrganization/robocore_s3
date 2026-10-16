# RoboCore S3 固件下载工具

[English](README.md) | **简体中文**

本 Windows 工具包适用于 **RoboCore S3 与 Robot Driver with ESP32S3 Lite**，包含仓库现有的 Espressif Flash Download Tool 3.9.9 R2，以及从当前 `robocore_s3` 源码构建的完整固件。使用者无需安装 Python 或 PlatformIO。

固件包含中英双语网页、总线设备控制、JSON 上位机接口和 IMU 探测。Lite 没有板载 IMU 时仍正常启动；两款板卡的供电规格应分别以各自 Wiki 为准。

## 烧录步骤

1. 将 ZIP **完整解压**到本地文件夹，不要直接在压缩包内运行。备份任务与配置，断开执行器和动力电源，再用数据线连接主板 **Type-C UART** 接口。
2. 双击 `Start_Download_Tool.bat`。启动器会设置正确的工作目录，仅打开工具，不会自动开始烧录。
3. 选择 **ESP32-S3 / Factory / UART**。选择实际新增的 COM 端口；工具包没有预选任何 COM 端口。
4. 确认以下三项已勾选。路径相对于解压后的工具目录；若工具显示空路径，请手动选择对应文件：

| 镜像 | 烧录地址 |
| --- | --- |
| `bin/bootloader.bin` | `0x0` |
| `bin/partitions.bin` | `0x8000` |
| `bin/firmware.bin` | `0x10000` |

5. 保持镜像中的 SPI 设置（启用 DoNotChgBin），目标 Flash 容量为 **8MB**。首次建议选择 **115200** 波特率，连接稳定时可使用 921600。
6. 点击 **START**，等待工具报告成功后按 RESET 重启。烧录过程中不要拔掉 USB。
7. 连接热点 **Robot**，密码 **12345678**，浏览器打开 **http://192.168.4.1**。如果保留了旧网络配置，请使用原有热点或地址。主网页默认英文，右上角可切换中文。

自动下载失败时，按住 BOOT，按下并松开 RESET，再松开 BOOT，然后重试。使用原生 Type-C USB 时需手动进入下载模式并选择新枚举的端口；本工具包优先推荐 Type-C UART。

**ERASE 会删除 Wi-Fi 配置、任务和其他 Flash 数据。** 只有确实需要完整重置且已备份时才使用。普通三镜像更新不会主动执行整片擦除。不要将独立的 `firmware.bin` 写到 `0x0`。

## 版本与校验

`manifest.json` 记录北京时间、源码提交、是否包含本地未提交修改、源码哈希、分区布局、烧录地址和文件 SHA-256。包含本地修改时，不能仅用 Git 提交号标识该构建。

首次运行前，可在解压目录中打开 PowerShell 执行只读校验：

```powershell
powershell -NoProfile -File .\Verify_Package.ps1
```

也可以用 `Get-FileHash -Algorithm SHA256` 对照 `SHA256SUMS.txt`。校验值用于检查包内文件一致性，不是发布者数字签名。下载程序运行后可能保存配置，而校验脚本检查的是初始打包文件。

三个镜像配套使用。烧录完成后按 RESET，连接主板热点并打开网页，先确认设备状态，再连接和控制执行器。

## 更新与资料

后续更新请整体替换工具包。本包不包含旧的单镜像在线更新脚本、历史设备日志或固件缓存。

- [源码仓库](https://github.com/LygionOrganization/robocore_s3)
- [RoboCore S3 Wiki](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/)
- [Lite Wiki](https://wiki.lygion.ai/reference/bus-devices/robot-driver-with-esp32s3-lite/)

