# RoboCore S3 固件下载工具

## 本次更新：robocore-1.3.6

修复 IMU 数据解码的 Arduino 宏冲突：原局部函数 word(offset) 被 Arduino 的 word(...) 宏替换为 makeWord(offset)，导致把偏移量 0、2、4、6、8、10、12 当成测量值。改名为 decodeInt16LE，实际解码传感器字节。独立 Arduino 例程未受影响且保持不变。

测试环境补入与真实 Arduino 一致的宏，已验证修复前失败、修复后通过。保留1.3.5的锁定采样和STA日志去重。烧录后确认版本1.3.6，再改变姿态检查T740及网页实时显示；硬件验收仍由实际板子确认。

## 本次更新：robocore-1.3.5

IMU 改用 QMI8658A 数据手册第13章的 SyncSample 锁定读取：关闭内部 AHB 时钟门控，完成 CTRL9 握手后启用 0x83；读取 STATUSINT 锁定样本，读至 GZ_H 自动释放。解决多任务读取跨更新边界时反复丢帧的问题；不是简单放宽数据过期门限。保留 400 kHz、112.1 Hz、原量程和100ms新鲜度检查。独立 Arduino 例程不变。

网页 HTTP 轮询只接收当前 rid 的结果，重复结果去重；旧 STA 配置反馈不会被不断记录。IMU 错误详情直接显示在页面结果区。

烧录后确认版本1.3.5、T740中 sampling_mode=sync_locked；观察 samples_acquired 递增、sample_age_ms 较小、sensor_restarts 不持续增加。此硬件锁定路径需要板上验收。

## 本次更新：robocore-1.3.4

修正状态 16（BIT4 自动限流）误报故障：步进指令应答按通信结果处理，反馈单独读取 0x41 状态寄存器。current_limited 表示限流；device_fault 仅标记电压、过热、过流，限位及目标超限另有独立字段。

IMU 在外设初始化结束后启动独立任务持续采集，T740 和网页读取 100 ms 内的最新快照。反馈增加 sample_age_ms、samples_acquired、sensor_restarts；无新数据时输出 read_state、count_before、count_after，不返回过期值。连续失去新数据会有限频率地重新初始化传感器。主程序尚需板上确认。

## 本次更新：robocore-1.3.3

修复 OLED 刷新后将共享 I2C 降回 100 kHz 的问题，统一保持 400 kHz，并对 OLED / IMU 完整访问加互斥锁。网页 IMU 请求对跨帧或暂未更新的样本最多尝试 4 次，不返回缓存数据。

步进/编码器状态读取将通信错误与设备状态分开：有效的位置、速度反馈不再因设备状态非零被丢弃，返回 device_status、device_fault 和 warning；不自动关闭输出、不清除设备告警。

板上验收：确认版本 robocore-1.3.3，测试网页 IMU 单次/实时读取及 OLED 刷新时的数据变化；使能电机并运动后直接读取状态，无需失能。若 device_fault 为 true，请保留完整 JSON 反馈以排查设备状态。

## 本次更新：robocore-1.3.2

板载 IMU 页面新增实时 3D 姿态、横滚/俯仰/相对航向、航向归零和采集暂停。切换页面或进入后台停止采集。六轴估算航向会漂移，快速运动及网络延迟影响精度，仅用于观察。界面无需外网资源。烧录后请刷新网页并在板上检查运动方向和刷新效果。

## 本次更新：robocore-1.3.1

IMU 使用已在 QMI-FINAL-1 独立例程测通的初始化和读取流程：400 kHz I2C、112.1 Hz 输出率、计数器逐字节读取、测量值 14 字节连续读取。取消状态位等待和 300 ms 重试；重复或跨帧数据返回 imu_data_not_ready，可稍后重试。

烧录后在设备信息确认 fw_ver 为 robocore-1.3.1。在网页 IMU 页面改变板子姿态并多次读取，或发送 {"T":740}，检查 sample_count 递增及 accel_g / gyro_dps 变化。主程序整合版仍需板上验证；不包含零偏校准和姿态融合。

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

