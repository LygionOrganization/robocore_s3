# RoboCore S3 验证与交付说明

本次实现保留永久任务循环，使用 USB CDC EVENT 接收、固定长度缓冲和单元素覆盖队列。执行中的指令完成后只取最新待执行指令；T=0 保留原有停止检查点。HTTP/WS/USB 共用此行为，不做可靠消息队列。

## 软件验证

- 完整固件：PlatformIO 编译通过，USB_MODE=0 / CDC On Boot。
- 18 个 Arduino 单元例程：全部通过相同工具链的独立编译。不是 Arduino IDE 所有版本的兼容性认证。
- `python test/run_usb_cdc_tests.py`：真实接收函数的分包、粘包、长度边界、断连、溢出恢复、停止和 200,000 条覆盖测试通过。
- `python test/run_robocore_tests.py`：真实 BusDevices/OptionalQMI8658/UsbOutput 模块通过主机模拟测试，覆盖无 IMU、I²C 失败、ID 冲突、参数边界、模式切换、反馈失败，以及 USB 主机停止读取时的限时输出和换行恢复。
- 新版网页：桌面与 390px 移动端检查；模拟扫描、ID 联动、S.BUS 16 通道、无 IMU 响应通过。截图中的设备是模拟数据。
- lygion-wiki：中英文共 22 页 RoboCore 教程、导航、例程下载与 AI 指南已写入，`mkdocs build --strict` 通过。旧步进驱动教程的模式重启说明已同步更正。

## 可复现依赖

`platformio.ini` 固定 pioarduino 55.03.30（Arduino 3.3.0 / ESP-IDF 5.5.0）和本次测试的库版本。跨电脑应使用本仓库完整的本地 `lib` 目录，不要只复制 src。若已有旧缓存，先对比 `pio pkg list`，再重新构建；不要混用官方旧平台与 pioarduino 平台。

USB 回调的重复定义来自核心和应用同时定义 TinyUSB 回调。现在由 Arduino 核心拥有该回调，应用注册公开 CDC EVENT。固定依赖可以减少环境差异，但不代表任意未来核心或库版本都无需适配。

如果 Windows 首次安装平台时出现 Python 依赖安装超时、esptool 元数据错误或 `esptool is not recognized`，这是工具环境问题。可在 PlatformIO 的 Python 环境中安装官方包后重新构建：

```powershell
& "$env:USERPROFILE/.platformio/penv/Scripts/python.exe" -m pip install --upgrade esptool==5.0.2
pio run
```

应先确认网络能够访问 PyPI/GitHub。不要用修改 USB 回调来处理工具下载或安装失败。

项目的 `tools/pio_python_path.py` 还修正了此平台版本在 Windows 中查找 Scripts 目录的路径，避免裸 `esptool` 命令被系统 `.py` 编辑器关联截获。复制完整项目配置时保留该脚本。

## 输出拥堵

当前核心的 USB write 在 FIFO 满时可能持续等待。应用输出统一经过 `UsbOutput`：每次只写 FIFO 可用空间，每个 write 最多等 10ms，超时允许丢弃输出。恢复时补换行，主机应忽略截断的非 JSON 行。主机仍应持续读取反馈。USB EVENT 中没有总线操作、日志输出或等待执行完成。

## 发布前实机验收

尚未烧录或操作真实电机。以下项目需在实际板卡上确认：

1. RoboCore S3 与 RD Lite 各自启动，旧板无 IMU 时正常进入网页；OLED 与 IMU 同时工作。
2. USB 连续高频输入至少 30 分钟，分别测试正常读取、停止读取、断开重连；再验证最新目标值最终生效。
3. Node 的 PWM/RGB/S.BUS、Stepper 的 0/1 无重启切换和从动接线、E02 的位置速度与校准。
4. QMI8658A 的安装方向和物理量、各系列舵机、CAN 与 ESP-NOW 通信。
5. 使用本次同一构建生成的 bootloader/partitions/firmware 三文件验证 download-tool 恢复，不混用旧镜像。

RoboCore 输入为用户确认的 5–48V；动力总线电压等于输入电压。产品尺寸由用户提供，GH1.25-3P 用途按用户确认更新。Wiki 修改尚未提交、推送或公开发布。

## 2026-10-15 双语页面与量产文档更新

- `pio run`：最终网页固件构建成功，Flash 1,429,847 字节，RAM 69,052 字节。
- `python tools/check_examples.py`：18/18 通过，包括新增 User_Interface_Module_A、RS485_Communication 及更新的 CAN_Communication。
- 两项主机回归测试重新通过：总线/IMU 验证与 USB CDC 200,000 条突发测试。
- `test/run_web_tests.cjs`：本地模拟服务器上验证 9 个页面默认英文、中英切换及持久化、表单值保留、导航标题、扫描数量与 ID 通知、两种语言确认对话框；1440px 桌面及 390px 手机布局。
- Wiki 中英文源码与导航通过 `mkdocs build --strict`；源码位于 `docs/robocore-wiki`，打包脚本使用这些文件，避免重新生成过时描述。
- 下载包只收录指定源码目录、指南与构建镜像；未收录私有设计文件。产品方进一步确认接口以原理图为准：舵机 RS485 为 XH2.54-4P；GH1.25-3P 用于板间 TTL 通信、供电解耦，不介绍具体接线操作。
- 本次未进行烧录或电气实测；CAN/RS485 收发、按键方向、蜂鸣器和 OLED 仍需量产硬件验收。主网页为双语，`/legacy` 保持原有界面。
