# TTL Node：PWM、RGB 与 S.BUS

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

TTL Node (A)，默认 ID=1，1000000 baud，连接 5264-3P 总线。测试 S.BUS 时连接匹配的接收机。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/TTL_Node/TTL_Node.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

本例只需例程包自带的 **lygion_devs**；不用在库管理器搜索同名第三方库。打开“文件 → 首选项”查看 Sketchbook location（项目文件夹位置），把包内 `lib/lygion_devs` 整个目录复制为 `<Sketchbook>/libraries/lygion_devs`，确认其下直接包含 `library.properties` 和 `src`，然后重启 IDE。不要多套一层目录。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

`p` 将 PWM 通道 1 设为 100/1000（10%），`x` 归零；`l` 设置两组状态灯；`s` 读取 16 个通道和 S.BUS 状态字节。`i` 把 ID 1 改为 2，只接目标设备且确认 ID 2 空闲。成功后本次运行自动使用 ID 2；重启程序前也要将变量 id 改为 2。

## 排查与下一步

S.BUS 寄存器回复不等于无线接收正常，还要查看状态字节和通道变化。RGB 使用 SDK 量化色值，不是通用 0–255 色值。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：TTL Node：PWM、RGB 与 S.BUS
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
NodeClass node;
uint8_t id = 1;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4); // 舵机/TTL 总线：RX5、TX4
  node.pSerial = &Serial1;
  Serial0.println("p=10% PWM, x=PWM off, l=LED, s=S.BUS, i=change ID 1 to 2.");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  switch (Serial0.read()) {
    case 'p': node.pwmCtl(id, 1, 100); break;
    case 'x': node.pwmCtl(id, 1, 0); break;
    case 'l': node.ledAllCtrl(id, 2, 1, 0, 0); break;
    case 's':
      if (node.sbusFlush(id) == 33 && node.getLastError() == 0) {
        Serial0.printf("S.BUS status=%u\n", node.sbusStatus());
        for (int ch = 1; ch <= 16; ++ch) Serial0.printf("CH%d=%u\n", ch, node.sbusGetch(ch));
      } else Serial0.println("No valid S.BUS register reply.");
      break;
    case 'i':
      // Connect only the intended device. Do not reuse an occupied ID.
      if (id == 1 && node.Ping(2) != 2 && node.Ping(1) == 1) {
        node.unLockEprom(1);
        node.writeByte(1, LY_NODE_ID, 2);
        if (node.Ping(2) == 2) { node.LockEprom(2); id = 2; }
        else { node.LockEprom(1); Serial0.println("ID change not verified."); }
      }
      break;
  }
  delay(1);
}
```
