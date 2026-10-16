# 在 AI 帮助下开发

下载 [AGENTS.md](../../../assets/files/robocore-s3/ai-guide.zip)，放到自己的项目根目录。它说明硬件约束、API、依赖和验证方法。资料源均在 [LygionOrganization](https://github.com/LygionOrganization/) 组织下。

## 提供给 AI 的信息

- 板卡：RoboCore S3。
- 设备型号、ID、波特率、供电电压与实际接线。
- 使用出厂固件的 Python 控制，还是替换固件的 Arduino 项目。
- 目标动作、机械行程、停止条件、允许丢失旧指令与否。

可从这个提示开始：

> 请先阅读 AGENTS.md 和仓库实际 API。我使用 RoboCore S3，通过 USB CDC 控制 ID 1 的 TTL Encoder E02，先实现只读取位置和速度的 Python 程序，处理超时与断线，不要修改校准参数。

下一步再让 AI 增加实际项目行为。运动项目先限定速度与行程，不要让生成代码在上电后立即执行大范围动作。

## 检查 AI 的结果

核对函数是否存在于附带 SDK，尤其不要把旧文档的 `WheelMode()` 直接套到当前 C++ TTLSDClass，应以 `SetMode()` 为准。确认模式 0/1 切换不要求重启、GPIO 未改变、无 板载 IMU 不死循环、USB 使用 EVENT。

让 AI 列出实际跑过的编译和测试，不把“生成代码”或模拟测试称为实机验证。完整项目可运行 `pio run`、USB 主机测试和例程编译脚本。将可运行的代码和自己的接线、参数记录一起保存，便于后续复现。

所有自动生成的运动、烧录和擦除操作，应由你确认具体接线与目标后再在硬件上执行。
