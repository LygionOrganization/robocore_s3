# HLS 总线舵机

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

一只 HLS 舵机，ID=1、总线波特率 1000000。断电后按舵机物理协议接 5264-3P TTL 或 XH2.54-4P RS485，主控 RX5/TX4。XT30 输入电压必须符合该舵机额定值。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/HLS_Servo/HLS_Servo.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- FTServo 2.0.0 (ftservo / FEETECH)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

先发送 `r` 读取位置、速度。核对 `ID` 和 `CENTER=2047`，保证中位及中位 + 50 计数均在机械行程内。发送 `m` 会使能并移动到 **CENTER + 50 的绝对位置**，不是在当前位置增加 50；`c` 移动到 CENTER；`x` 关闭扭矩。关闭扭矩后负载可能自由下落，请先支撑。 HLS 的 WritePosEx 最后一个参数 150 是电流限制寄存器值，请按设备规格设置。

## 排查与下一步

读数为负数或无响应时，检查 ID、波特率、动力电源和协议型号。不要用另一个型号的行程参数测试。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：HLS 总线舵机
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install FTServo 2.0.0 by ftservo / FEETECH (provides SCServo.h).
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <SCServo.h>

// Supply the servo at its rated voltage. Bus power equals board input power.
// STS and SMS share the SDK class; verify your model's range before moving.
HLSCL servo;
constexpr uint8_t ID = 1;
constexpr int CENTER = 2047;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4); // 舵机/TTL 总线：RX5、TX4
  servo.pSerial = &Serial1;
  Serial0.println("Send m to move 50 counts, c to center, r to read, or x to disable.");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  if (!Serial0.available()) { delay(1); return; }
  char command = Serial0.read();
  if (command == 'm' || command == 'c') {
    int target = command == 'm' ? CENTER + 50 : CENTER;
    servo.EnableTorque(ID, 1);
    servo.WritePosEx(ID, target, 100, 10, 150);
  } else if (command == 'x') {
    servo.EnableTorque(ID, 0);
  } else if (command == 'r') {
    Serial0.printf("Position=%d, speed=%d\n", servo.ReadPos(ID), servo.ReadSpeed(ID));
  }
}
```
