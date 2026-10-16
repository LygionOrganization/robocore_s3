# 电脑/上位机控制

RoboCore S3 可以作为机器人的“小脑板”，与 PC 或单板计算机（SBC）组成大小脑架构：上位机负责视觉、规划和应用逻辑，RoboCore S3 负责向总线设备发送控制指令并读取反馈。上位机可通过原生 USB CDC、HTTP 或 WebSocket 发送人类可读的 JSON 指令。

Python 只是示例语言。用户可以将 [JSON 指令表](json-api.md) 与项目需求交给 AI 工具，使用 C++、C#、JavaScript、Rust 等语言开发通信程序。保留字段类型、UTF-8 编码、USB 换行分帧和 `rid` 匹配规则；先读取状态，再加入运动。HTTP 202 仅表示接收，待执行命令可能被更新目标覆盖，不应将网络通信视为硬实时控制。

## Python 示例：HTTP、WebSocket 与 USB CDC

本章控制的是**完整出厂固件**，无需给主控重新烧录。下载[例程包](../../../assets/files/robocore-s3/examples.zip)，进入其中 `python_example/robocore`。

```powershell
python -m pip install requests websocket-client pyserial
```

## HTTP

电脑连接主控热点或同一路由器，在 PowerShell 执行：

```powershell
python client.py http http://192.168.4.1 '{"T":700,"first":0,"last":20}'
```

脚本提交后会按 rid 轮询 `/api/result`，不会把 HTTP 202 当作执行成功。如果接入路由器，请换成实际 IP。

## WebSocket

```powershell
python client.py ws ws://192.168.4.1/ws '{"T":732,"id":1}'
```

保持连接接收 JSON，忽略遥测或不匹配 rid 的结果。该示例读取 E02，不让电机运动。

## USB CDC

连接主控原生 USB 接口，关闭其他占用端口的软件，并替换 COM15：

```powershell
python client.py usb COM15 '{"T":740}'
```

脚本自动追加换行、持续读取回复并忽略非 JSON 日志。Linux 可使用 `/dev/ttyACM0` 等实际枚举路径。若返回 `imu_not_present`，请检查板载传感器和 I²C 接线。

## 在自己的程序中调用

```python
from client import request

result = request('http', 'http://192.168.4.1', {'T': 732, 'id': 1})
if result.get('ok'):
    print(result['pos'], result['speed'])
else:
    print(result.get('error'))
```

每次等待一条配置指令的结果。超时不代表一定没执行，先读状态再决定是否重试；高频控制时丢掉等待中的旧目标是固件的设计。完整字段见 [JSON API](json-api.md)。
