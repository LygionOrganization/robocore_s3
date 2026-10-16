# HTTP JSON 通信

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

带 Wi-Fi 的电脑，连接外部天线后再运行。此例只读写内存数值，不控制电机。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/HTTP_JSON/HTTP_JSON.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- ArduinoJson 7.4.3 (Benoit Blanchon)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

电脑连接热点 RoboCoreDemo，密码12345678。用 POST 向 http://192.168.4.1/api/cmd 发送 {"T":1,"value":42}，得到 {"ok":true,"value":42}；再发送 {"T":2} 读取。同一数值在重启后归零。使用下方 PowerShell 请求示例；浏览器地址栏的 GET 不是 POST。

## 排查与下一步

HTTP400 表示非法或超过255字节的输入。没有网页首页属于正常现象；此单元例程只提供 API，完整控制台需恢复完整固件。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：HTTP JSON 通信
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
WebServer server(80);
int value = 0;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  server.on("/api/cmd", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument in, out;
    if (body.length() > 255 || deserializeJson(in, body) || !in["T"].is<int>()) {
      server.send(400, "application/json", "{\"ok\":false}"); return;
    }
    // Teaching protocol: T=1 sets a RAM value, T=2 reads it. No motors move.
    int type = in["T"];
    bool ok = type == 2 || (type == 1 && in["value"].is<int>());
    if (ok && type == 1) value = in["value"];
    out["ok"] = ok; out["value"] = value;
    String response; serializeJson(out, response);
    server.send(ok ? 200 : 400, "application/json", response);
  });
  server.begin();
  Serial0.println("POST http://192.168.4.1/api/cmd with {\"T\":1,\"value\":42}");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() { server.handleClient(); delay(1); }
```

### PowerShell

```powershell
Invoke-RestMethod -Uri http://192.168.4.1/api/cmd -Method Post -ContentType application/json -Body '{"T":1,"value":42}'
Invoke-RestMethod -Uri http://192.168.4.1/api/cmd -Method Post -ContentType application/json -Body '{"T":2}'
```
