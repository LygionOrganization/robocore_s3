# User Interface Module (A)：按键、蜂鸣器和屏幕

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

断电后将 UI Module (A) 接到 PH1.25-10P。上/下/左/右/确认=IO10/11/12/9/2，低有效；蜂鸣器21；OLED SDA6/SCL7、0x3C、128×32。这些资源都在外接模块上。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/User_Interface_Module_A/User_Interface_Module_A.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- Adafruit SSD1306 2.5.17
- Adafruit GFX Library 1.12.6
- Adafruit BusIO

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

依次按五个按键，串口显示 Button: UP/DOWN/LEFT/RIGHT/OK，屏幕更新 Last，蜂鸣器响 80ms。去抖时间30ms；长按只触发一次，释放后再次按下才产生新事件。本例程不发送电机命令。

## 排查与下一步

OLED 不可用时，按键和蜂鸣器继续工作。若按键无响应，检查模块方向、共地和 GPIO；不要把 RESET/BOOT 当作模块五向按键。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：User Interface Module (A)：按键、蜂鸣器和屏幕
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, and Adafruit BusIO.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Connect User Interface Module (A) to the PH1.25-10P expansion port.
// OLED and onboard QMI8658A share SDA=6 / SCL=7; initialize Wire only once.
constexpr uint8_t buttonPins[] = {10, 11, 12, 9, 2};
const char *buttonNames[] = {"UP", "DOWN", "LEFT", "RIGHT", "OK"};
constexpr uint8_t buzzerPin = 21;
bool rawState[5], stableState[5];
uint32_t changedAt[5] = {};
uint32_t beepAt = 0;
bool beeping = false, screenReady = false;
Adafruit_SSD1306 screen(128, 32, &Wire, -1);

void showButton(const char *name) {
  Serial0.printf("Button: %s\n", name);
  if (!screenReady) return;
  screen.clearDisplay();
  screen.setCursor(0, 0);
  screen.println("RoboCore S3 / UI (A)");
  screen.println("Press a button");
  screen.print("Last: "); screen.println(name);
  screen.display();
}

// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  for (size_t i = 0; i < 5; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    rawState[i] = stableState[i] = digitalRead(buttonPins[i]);
  }
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
    screenReady = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (screenReady) {
    screen.setTextColor(SSD1306_WHITE);
    screen.setTextSize(1);
  } else Serial0.println("OLED unavailable; buttons and buzzer remain active.");
  showButton("None");
}

// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  const uint32_t now = millis();
  for (size_t i = 0; i < 5; ++i) {
    const bool state = digitalRead(buttonPins[i]);
    if (state != rawState[i]) { rawState[i] = state; changedAt[i] = now; }
    // Debounce both edges; emit one event per press, without blocking other keys.
    if (state != stableState[i] && uint32_t(now - changedAt[i]) >= 30) {
      stableState[i] = state;
      if (state == LOW) {
        showButton(buttonNames[i]);
        tone(buzzerPin, 1500);
        beepAt = now; beeping = true;
      }
    }
  }
  if (beeping && uint32_t(now - beepAt) >= 80) {
    noTone(buzzerPin); digitalWrite(buzzerPin, HIGH); beeping = false;
  }
  delay(1);
}
```
