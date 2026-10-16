# PC / host control

RoboCore S3 can serve as the robot's low-level controller alongside a PC or single-board computer (SBC). The host handles perception, planning and applications; RoboCore S3 handles bus-device commands and feedback. Connect through native USB CDC, HTTP or WebSocket using human-readable JSON.

Python is one example implementation, not a requirement. Use the [JSON command table](json-api.md) with AI tools to develop a client in C++, C#, JavaScript, Rust or another language. Preserve field types, UTF-8 encoding, USB newline framing and `rid` response matching. Start with a read-only command before adding motion. HTTP 202 confirms acceptance only, and pending commands can be replaced by newer ones.

## Python examples: HTTP, WebSocket and USB CDC

These examples control the **complete factory firmware**; do not flash a unit sketch first. Download the [example package](../../../assets/files/robocore-s3/examples.zip), then open `python_example/robocore`.

```powershell
python -m pip install requests websocket-client pyserial
python client.py http http://192.168.4.1 '{"T":700,"first":0,"last":20}'
python client.py ws ws://192.168.4.1/ws '{"T":732,"id":1}'
python client.py usb COM15 '{"T":740}'
```

For HTTP/WS, join the board AP or the same router network. Replace the IP if using a router. HTTP submits then polls `/api/result` for the matching rid. HTTP 202 alone is not execution success.

For USB, use the native USB connector, substitute the actual port, and close other serial monitors. Linux may enumerate `/dev/ttyACM0`. The script appends a newline, continuously reads replies and ignores non-JSON log lines. If `imu_not_present` is returned, check the onboard sensor and I²C wiring.

```python
from client import request
result = request('http', 'http://192.168.4.1', {'T': 732, 'id': 1})
if result.get('ok'):
    print(result['pos'], result['speed'])
else:
    print(result.get('error'))
```

Wait for each configuration result. A timeout does not prove a write failed; read state before retrying. High-rate pending targets are intentionally overwritten. See the [JSON API](json-api.md) for ranges and responses.
