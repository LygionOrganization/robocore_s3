# RoboCore S3 1.2 验证记录

- 网页：调整工作台、总线扫描、TTL Node、步进驱动和编码器文案；停止循环移至 JSON 调试。
- 设备 ID：编码器复用 T=720；STS/SMS、HLS、SCS 使用 T=13/23/33，并验证输入、目标 ID 占用、写入结果及设备故障状态。
- Wi-Fi：后台连接 STA，保留失败配置；网页每 3 秒读取状态/IP；OLED 在网络状态变化时更新；密码显隐并隐藏日志密码。
- JSON 调试：OLED、蜂鸣器、ESP-NOW 快捷示例。移除公开目录中的 T=9999。
- Wiki：中英文分类接口表、成对发送/接收示例、独立自动化页面、2.4 GHz 说明、总线设备及从 ID 术语。

## 验证

- PlatformIO 主固件编译成功。
- 18/18 Arduino 例程编译成功。
- test/run_robocore_tests.py：设备协议、参数校验、ID、模式和 IMU 主机测试通过。
- test/run_usb_cdc_tests.py：帧处理、最新命令优先、退出循环、溢出恢复和 200000 条命令测试通过。
- test/run_mission_tests.py：任务存储与执行校验测试通过。
- test/run_web_tests.cjs：双语、扫描、表单、确认框、桌面及手机布局通过。
- test/web_missions.cjs：指令目录、任务保存、读回、分页、上传中断、覆盖取消和运行通过。
- test/web_connectivity.cjs：模拟 STA 状态/IP、密码显隐/日志隐藏、ID 命令和按钮位置通过。
- 中英文 MkDocs 构建通过。
- 下载 ZIP 内各文件 SHA-256 及 Wiki 分发副本一致性校验通过。

未连接、擦除或烧录实际硬件。STA 路由器兼容性、实机 OLED 更新和设备 EEPROM 改 ID 仍需实机验证；软件模拟不代表硬件验证。
