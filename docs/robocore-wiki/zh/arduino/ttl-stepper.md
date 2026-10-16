# TTL 步进驱动

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

TTL Stepper Driver (A)、匹配步进电机及额定电源。默认 ID=1、1000000 baud，TTL 总线连接 IO4/IO5。先确认驱动器相位、电流配置和机械空间。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/TTL_Stepper/TTL_Stepper.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

本例只需例程包自带的 **lygion_devs**；不用在库管理器搜索同名第三方库。打开“文件 → 首选项”查看 Sketchbook location（项目文件夹位置），把包内 `lib/lygion_devs` 整个目录复制为 `<Sketchbook>/libraries/lygion_devs`，确认其下直接包含 `library.properties` 和 `src`，然后重启 IDE。不要多套一层目录。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

先 `r` 读反馈。位置模式：`0` → `e` → `p`，目标为绝对位置 3200。连续模式：`1` → `e` → `v`，`s` 将连续速度归零；`x` 失能。模式修改会关闭输出，模式 0/1 切换无需重启。`z` 将当前位置坐标设为 0，不产生运动。`2` 为从动模式，示例从 ID=2，需按设备教程连接 EDS 并保持主 ID 唯一。

## 排查与下一步

CURRENT=150、速度100、加速度10 都是寄存器值，不能按安培或普通步/秒理解。只改相位配置才需要重启。模式不对时不要发送对应运动指令。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：TTL 步进驱动
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
TTLSDClass stepper;
constexpr uint8_t ID = 1;
// These are raw register values, not amperes or steps per second.
constexpr uint16_t CURRENT = 150;
void setMode(uint8_t mode) {
  stepper.unLockEprom(ID); // Also disables output.
  stepper.SetMode(ID, mode);
  if (mode == 2) stepper.writeByte(ID, 7, 2); // Leader primary ID = 2.
  stepper.LockEprom(ID);
  // Modes 0 and 1 can switch without restarting. Phase changes require restart.
  Serial0.println("Mode written. Use e to enable output when ready.");
}
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4); // 舵机/TTL 总线：RX5、TX4
  stepper.pSerial = &Serial1;
  Serial0.println("0=position, 1=speed, 2=follower, e=enable, x=disable");
  Serial0.println("p=move to 3200, v=rotate, s=stop, z=set current position to 0, r=read");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  switch (Serial0.read()) {
    case '0': setMode(0); break;
    case '1': setMode(1); break;
    case '2': setMode(2); break; // Connect EDS according to the product guide.
    case 'e': stepper.EnableTorque(ID, 1); break;
    case 'x': stepper.EnableTorque(ID, 0); break;
    case 'p': stepper.WritePosEx(ID, 3200, 100, 10, CURRENT); break;
    case 'v': stepper.WriteSpe(ID, 100, 10, CURRENT); break;
    case 's': stepper.WriteSpe(ID, 0, 10, CURRENT); break;
    case 'z': stepper.CalibrationOfs(ID, 0); break;
    case 'r': {
      int position = stepper.ReadPos(ID);
      if (stepper.getLastError()) { Serial0.println("Position read failed"); break; }
      int speed = stepper.ReadSpeed(ID);
      if (stepper.getLastError()) { Serial0.println("Speed read failed"); break; }
      Serial0.printf("Position=%d, speed=%d\n", position, speed);
      break;
    }
  }
  delay(1);
}
```
