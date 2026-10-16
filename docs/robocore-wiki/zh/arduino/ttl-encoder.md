# TTL 编码器 E02

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

TTL Encoder E02，ID=1、1000000 baud，通过 TTL 接口连接 IO4/IO5；手动转动轴用于观察反馈。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/TTL_Encoder/TTL_Encoder.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

本例只需例程包自带的 **lygion_devs**；不用在库管理器搜索同名第三方库。打开“文件 → 首选项”查看 Sketchbook location（项目文件夹位置），把包内 `lib/lygion_devs` 整个目录复制为 `<Sketchbook>/libraries/lygion_devs`，确认其下直接包含 `library.properties` 和 `src`，然后重启 IDE。不要多套一层目录。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

发送 `r`，读取位置、角度及速度。每圈 4096 计数，示例角度=位置×360/4096，包含多圈累积。需要校准时发送 `z`，把当前位置设为 0；校准会保存，请勿在循环里高频调用。

## 排查与下一步

位置范围为 0–65534；基准校准保存，但多圈累积不在断电后保留。读取失败先查 ID、波特率和供电，不要把错误当角度。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：TTL 编码器 E02
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Bundled library: copy the entire lib/lygion_devs folder from this project
// to your Arduino sketchbook libraries/lygion_devs folder, then restart the IDE.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <lygion_devs.h>
// E02 uses the same position/speed register protocol as TTLSDClass.
TTLSDClass encoder;
constexpr uint8_t ID = 1;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4); // 舵机/TTL 总线：RX5、TX4
  encoder.pSerial = &Serial1;
  Serial0.println("r=read, z=save current position as 0 (persistent calibration).");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  char c = Serial0.read();
  if (c == 'z') {
    encoder.CalibrationOfs(ID, 0);
    Serial0.printf("Calibration bus error=%u\n", encoder.getLastError());
  } else if (c == 'r') {
    int pos = encoder.ReadPos(ID);
    if (encoder.getLastError()) { Serial0.println("Position read failed"); return; }
    int speed = encoder.ReadSpeed(ID);
    if (encoder.getLastError()) { Serial0.println("Speed read failed"); return; }
    Serial0.printf("Position=%d, angle=%.2f deg, speed=%d counts/s\n", pos, pos * 360.0 / 4096.0, speed);
  }
  delay(1);
}
```
