# RoboCore S3 JSON API

Send one JSON object per WebSocket frame (`/ws`) or HTTP POST (`/api/cmd`), or one UTF-8 line ending in newline over USB CDC. Maximum 255 bytes excluding newline. HTTP 202 acknowledges receipt only. Read `/api/result` or WebSocket replies and match the integer `rid`. Only the latest pending command is retained; wait for a result before sending another configuration command. Do not automatically retry timed-out writes.

ESP-NOW uses a zero-padded 250-byte frame and does not automatically return execution results. Configure matching channels and peer MAC addresses first.


See [Web automation scripts](automation.md) for editing, saving and running missions.

## Common fields and operating sequence

JSON field names are case-sensitive. Examples are complete objects: change IDs, positions and speeds for your hardware before sending. Supply all shown fields unless explicitly optional. Legacy servo/arm interfaces retain their original protocol and lack the new bus API’s comprehensive validation; check mechanical travel yourself. Sync commands validate array lengths, unique IDs, numeric types and positions (SCS0–1023, other sync families0–4095).

| Field | Meaning and units |
| --- | --- |
| `id / old_id / new_id` | Current, old and new device IDs. Use an individual ID. Some legacy protocols accept broadcast254; never broadcast reads or ID changes |
| `baud` | Controller UART bus bitrate; must match devices. Prefer T=10. Startup configuration applies again after reboot |
| `pos` | Servo position counts; commonly0–1023 for SCS,0–4095 for STS/SMS/HLS. Model-specific limits take precedence |
| `spd / acc` | Servo speed/acceleration register values. Zero is model-specific and does not necessarily mean stop; use nonzero HLS speed |
| `time` | SCS movement time in milliseconds; priority versus speed follows the device protocol. time=0,spd=0 is not a stop command |
| `state` | Servo torque:0 off,1 enabled. Support loads before disabling |
| `cl / tor` | HLS single-axis current limit / per-axis synchronized limit array; register values, not directly amperes |
| `idn` | Number of synchronized devices; every array must have exactly this many elements with count1–16; the full command must also fit255 bytes |
| `id[] / pos[] / spd[] / acc[] / tor[]` | Matching indices describe one axis. Simplified sync commands omit speed/acceleration and offer fewer controls |
| `mid / ang / rad` | SCS reference count / target degrees / target radians, using T=102 conversion settings; their spd fields are degrees/s and radians/s respectively |
| `move` | T=109/112/115:1 starts movement,0 defers until T=118 |
| `steps / angle` | T=102: at least512 counts,90–360 degrees of travel; used for servo coordinate conversion |
| `set` | T=200 takes `[LED index,r,g,b]`; T=201 takes `[r,g,b]`. For compatible legacy LED controllers, not the T=712 color convention |
| `line / text / update` | OLED line1–4, ASCII text, refresh0/1. T=203 refreshes; T=204 updates l1…l4 together |
| `num` | Legacy Node LED count/refresh scope, per device protocol. Prefer T=712 for new Node applications |
| `freq / duration` | Buzzer Hz and milliseconds; duration=0 does not mean a continuous tone |
| `ang[] / rad[] / pos[]` | Four arm joint angles, radians or zero counts; order: base, shoulder, elbow, gripper |
| `xyzg` | Arm `[x,y,z,g]`, position in mm and gripper angle in degrees; workspace depends on matching geometry |
| `rbzg` | Arm `[r,b,z,g]`, r/z in mm, base b and gripper g in degrees |
| `spd / br` (arm) | Smooth-motion parameter and base interpolation ratio; not raw servo speed units. Use matching arm configuration |
| `flag / hz` | T=145 periodic arm feedback0/1 and frequency; unrelated to T=740 onboard IMU readings |
| `flag / axis / delta` | T=146 continuous-motion enable0/1, preset coordinate axis and increment; only for the matching arm application |

Scan with T=700, read model-specific feedback, then try low-speed movement within limits. TTL and bus-device RS485 share IO4/IO5 baud and data; they are not separate UARTs. External CAN/RS485 uses IO13/IO14 and requires both the correct selector position and matching program.

### Synchronized servo example

Give two STS/SMS servos different IDs and verify travel limits. Read each with T=15 first, then send:

```json
{"T":16,"id":[1,2],"idn":2,"pos":[2047,2100],"spd":[100,100],"acc":[10,10]}
```

To measure motion of the board itself, send `{"T":740}`. It reads the onboard QMI8658A, not an external TTL IMU.

## Requests and replies

Replies below are illustrative. Read HTTP replies through GET /api/result or receive WebSocket messages.

| Operation | Send | Example reply |
| --- | --- | --- |
| Scan | `{"T":700,"first":0,"last":20,"rid":1}` | `{"T":700,"rid":1,"ok":true,"ids":[1,5],"first":0,"last":20}` |
| Encoder | `{"T":732,"id":1,"rid":2}` | `{"T":732,"rid":2,"id":1,"ok":true,"bus_error":0,"device_status":0,"pos":2048,"speed":0}` |
| IMU unavailable | `{"T":740,"rid":3}` | `{"T":740,"rid":3,"ok":false,"present":false,"error":"imu_not_present"}` |

`ok:false` indicates invalid parameters or device communication failure. `dispatched:true` means handler dispatch only, not verified motion. ID commands T=13/23/33/710/720 verify the new ID; encoders share T=720 with steppers.

## Read-only endpoints

| Endpoint | Content |
| --- | --- |
| GET /api/info | Firmware, baud rate, IMU, mission state, sta_state, sta_ip, ap_ip, sta_ssid; no passwords |
| GET /api/result | Latest result only; match rid |

<!-- COMMAND_TABLES -->

## Communication and system

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 0 | Stop mission loop | `{"T":0}` | Requests loop exit and clears pending commands; does not disable motors. |
| 51 | Wait milliseconds | `{"T":51,"delay":1000}` | delay: milliseconds to wait; useful between mission steps. |
| 180 | Four hub motor speeds | `{"T":180,"A":0,"B":0,"C":0,"D":0}` | A/B/C/D control four compatible hub motors; register value × 0.06 = rpm. Not a generic stepper command. |
| 399 | Erase filesystem | `{"T":399}` | Erases all missions and filesystem configuration; does not restart automatically. Irreversible. |
| 400 | Configure Wi-Fi | `{"T":400,"mode":1,"ap_ssid":"LYgion","ap_password":"12345678","channel":1,"sta_ssid":"ssid","sta_password":"password"}` | mode=0 disables Wi-Fi, 1 enables AP+STA. STA connects asynchronously; failed connections retain settings. GET /api/info reports state and IP. Use a 2.4 GHz router network. |
| 401 | Read Wi-Fi configuration | `{"T":401}` | Set fields as shown; verify target hardware and power before execution. |
| 402 | Read AP IP | `{"T":402}` | Set fields as shown; verify target hardware and power before execution. |
| 403 | Read station IP | `{"T":403}` | Set fields as shown; verify target hardware and power before execution. |
| 410 | Initialize ESP-NOW | `{"T":410,"longrange":0}` | longrange: 0 normal, 1 long range. Both peers need compatible mode and the same channel. |
| 411 | Set ESP-NOW receive mode | `{"T":411,"mode":1}` | mode: 0 receive off, 1 trusted MAC reception, 2 broadcast reception. |
| 412 | Read MAC address | `{"T":412}` | Set fields as shown; verify target hardware and power before execution. |
| 413 | Send ESP-NOW JSON | `{"T":413,"mac":"FF:FF:FF:FF:FF:FF","data":"{\"T\":205}"}` | mac is colon-separated; data is an escaped JSON string. Uses a zero-padded 250-byte frame, not arbitrary large objects. |
| 414 | Add ESP-NOW peer | `{"T":414,"mac":"FF:FF:FF:FF:FF:FF"}` | mac is the peer MAC; both devices must use the same Wi-Fi channel. |
| 415 | ESP-NOW synthetic MAC option | `{"T":415,"fake":0}` | fake is a synthetic MAC option for legacy applications; keep 0 for normal communication. |
| 501 | Send CAN frame | `{"T":501,"id":291,"ext":0,"data":[17,34,51]}` | CAN switch position; id is identifier, ext=0 standard/1 extended, data at most eight bytes 0–255; default 500 kbps. |
| 600 | Restart controller | `{"T":600}` | Restarts immediately and disconnects; an execution reply may not arrive. |
| 601 | Erase NVS | `{"T":601}` | Erases NVS settings, separate from mission storage. Irreversible. |
| 602 | Erase filesystem | `{"T":602}` | Erases all missions and filesystem configuration; does not restart automatically. Irreversible. |
| 603 | Read message output settings | `{"T":603}` | Set fields as shown; verify target hardware and power before execution. |
| 604 | Set message outputs | `{"T":604,"echo":1,"uart":0,"usb":1}` | echo/uart/usb are 0 or 1; disabling usb suppresses USB result output. |
| 605 | UART0 forwarding mode | `{"T":605,"sf":1}` | sf=1 forwards UART0 bytes to the servo bus (default); sf=0 parses UART0 input as JSON. |

## Servos

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 10 | Set controller bus baud rate | `{"T":10,"baud":1000000}` | Set fields as shown; verify target hardware and power before execution. |
| 11 | STS/SMS position | `{"T":11,"id":1,"pos":2047,"spd":100,"acc":0}` | Set fields as shown; verify target hardware and power before execution. |
| 12 | STS/SMS calibrate midpoint | `{"T":12,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 13 | STS/SMS change ID | `{"T":13,"old_id":1,"new_id":2}` | Distinct integer old_id / new_id in 0–253. Connect one device at a time. Rejects occupied IDs and verifies the new ID after writing. |
| 14 | STS/SMS torque enable | `{"T":14,"id":1,"state":1}` | Set fields as shown; verify target hardware and power before execution. |
| 15 | STS/SMS feedback | `{"T":15,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 16 | STS/SMS synchronized targets | `{"T":16,"id":[1,2],"idn":2,"pos":[2047,2047],"spd":[100,100],"acc":[0,0]}` | Set fields as shown; verify target hardware and power before execution. |
| 17 | STS/SMS simple synchronized targets | `{"T":17,"id":[1,2],"idn":2,"pos":[2047,2047]}` | Speed and acceleration are fixed at0; zero semantics depend on the servo. Use T=16 for explicit speed limits. |
| 21 | HLS position | `{"T":21,"id":1,"pos":2047,"spd":100,"acc":0,"cl":500}` | Set fields as shown; verify target hardware and power before execution. |
| 22 | HLS calibrate midpoint | `{"T":22,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 23 | HLS change ID | `{"T":23,"old_id":1,"new_id":2}` | Distinct integer old_id / new_id in 0–253. Connect one device at a time. Rejects occupied IDs and verifies the new ID after writing. |
| 24 | HLS torque enable | `{"T":24,"id":1,"state":1}` | Set fields as shown; verify target hardware and power before execution. |
| 25 | HLS feedback | `{"T":25,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 26 | HLS synchronized targets | `{"T":26,"id":[1,2],"idn":2,"pos":[2047,2047],"spd":[100,100],"acc":[0,0],"tor":[500,500]}` | Set fields as shown; verify target hardware and power before execution. |
| 27 | HLS simple synchronized targets | `{"T":27,"id":[1,2],"idn":2,"pos":[2047,2047],"tor":[500,500]}` | Fixed speed60 and acceleration50; tor sets each axis current limit. Use T=26 for full parameters. |
| 31 | SCS position | `{"T":31,"id":1,"pos":511,"time":0,"spd":100}` | Set fields as shown; verify target hardware and power before execution. |
| 33 | SCS change ID | `{"T":33,"old_id":1,"new_id":2}` | Distinct integer old_id / new_id in 0–253. Connect one device at a time. Rejects occupied IDs and verifies the new ID after writing. |
| 34 | SCS torque enable | `{"T":34,"id":1,"state":1}` | Set fields as shown; verify target hardware and power before execution. |
| 35 | SCS feedback | `{"T":35,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 36 | SCS synchronized targets | `{"T":36,"id":[1,2],"idn":2,"pos":[511,511],"spd":[100,100]}` | Set fields as shown; verify target hardware and power before execution. |
| 37 | SCS simple synchronized targets | `{"T":37,"id":[1,2],"idn":2,"pos":[511,511]}` | Speed is fixed at0, which may mean maximum speed. Use T=36 for explicit speed limits. |
| 100 | Set controller bus baud rate | `{"T":100,"baud":1000000}` | Set fields as shown; verify target hardware and power before execution. |
| 101 | Select generic servo type | `{"T":101,"type":1}` | type=0 SCS, 1 STS/SMS, 2 HLS; prefer the model-specific commands. |
| 102 | Set servo coordinate conversion | `{"T":102,"steps":4096,"angle":360}` | steps is servo counts per turn; angle is travel in degrees. This changes servo conversion, not an external encoder. |
| 103 | Generic servo feedback | `{"T":103,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 104 | Generic servo ping | `{"T":104,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 105 | Generic servo change ID | `{"T":105,"old_id":1,"new_id":2}` | Set fields as shown; verify target hardware and power before execution. |
| 106 | Generic servo calibrate midpoint | `{"T":106,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 107 | Generic servo move to midpoint | `{"T":107,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 108 | Generic servo torque enable | `{"T":108,"id":1,"state":1}` | Set fields as shown; verify target hardware and power before execution. |
| 109 | SCS step position | `{"T":109,"id":1,"pos":512,"time":0,"spd":100,"move":1}` | Set fields as shown; verify target hardware and power before execution. |
| 112 | SCS degree target | `{"T":112,"id":1,"mid":512,"ang":10,"spd":10,"move":1}` | Set fields as shown; verify target hardware and power before execution. |
| 115 | SCS radian target | `{"T":115,"id":1,"mid":512,"rad":0.785,"spd":0.2,"move":1}` | Set fields as shown; verify target hardware and power before execution. |
| 118 | Trigger deferred servo moves | `{"T":118}` | Set fields as shown; verify target hardware and power before execution. |
| 119 | SCS torque limit | `{"T":119,"id":1,"limit":500}` | Legacy advanced interface; use the model-specific driver library for torque-limit configuration. |

## Arm presets

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 130 | Read arm zero positions | `{"T":130}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 131 | Set arm zero positions | `{"T":131,"pos":[512,512,512,512]}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 132 | Read arm positions | `{"T":132}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 133 | Calibrate arm zeros at current pose | `{"T":133}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 134 | Arm joint angles | `{"T":134,"ang":[0,0,0,0]}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 135 | Arm joint radians | `{"T":135,"rad":[0,0,0,0]}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 136 | Arm XYZG target | `{"T":136,"xyzg":[260.5,0,122.38,50]}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 137 | Arm RBZG target | `{"T":137,"rbzg":[260.5,0,122.38,50]}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 138 | Arm smooth XYZG move | `{"T":138,"xyzg":[236.5,0,122.38,0],"spd":0.4}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 139 | Arm smooth RBZG move | `{"T":139,"rbzg":[236.5,0,122.38,0],"spd":0.4,"br":200}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 140 | Arm maximum joint speed | `{"T":140,"spd":1.2}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 141 | Read arm feedback radians | `{"T":141}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 142 | Read arm target radians | `{"T":142}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 143 | Read XYZG IK state | `{"T":143}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 144 | Read RBZG IK state | `{"T":144}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 145 | Arm periodic feedback | `{"T":145,"flag":0,"hz":10}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 146 | Arm continuous motion | `{"T":146,"flag":0,"axis":0,"delta":0.01}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |
| 147 | Arm manual calibration mode | `{"T":147}` | Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates. |

## External modules

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 200 | Legacy LED single color | `{"T":200,"id":40,"set":[1,255,0,0]}` | Set fields as shown; verify target hardware and power before execution. |
| 201 | Legacy LED all colors | `{"T":201,"id":40,"set":[9,0,0]}` | Set fields as shown; verify target hardware and power before execution. |
| 202 | OLED single line | `{"T":202,"line":1,"text":"Hello, world!","update":1}` | Requires the external UI Module (A) SSD1306 OLED; line is 1–4, update=1 refreshes immediately. Default font uses ASCII. |
| 203 | OLED refresh | `{"T":203}` | Requires the external UI Module (A) SSD1306 OLED; line is 1–4, update=1 refreshes immediately. Default font uses ASCII. |
| 204 | OLED four lines | `{"T":204,"l1":"Hello, world!","l2":"Hello, world!","l3":"Hello, world!","l4":"Hello, world!"}` | Requires the external UI Module (A) SSD1306 OLED; line is 1–4, update=1 refreshes immediately. Default font uses ASCII. |
| 205 | OLED clear | `{"T":205}` | Requires the external UI Module (A) SSD1306 OLED; line is 1–4, update=1 refreshes immediately. Default font uses ASCII. |
| 206 | External module buzzer | `{"T":206,"freq":1000,"duration":1000}` | freq: Hz; duration: milliseconds. Buzzer is on the external UI Module (A), GPIO21. |
| 208 | Legacy Node change ID | `{"T":208,"old":0,"new":1}` | Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback. |
| 209 | Legacy Node PWM | `{"T":209,"id":0,"ch":0,"pwm":1000}` | Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback. |
| 210 | Legacy Node single LED | `{"T":210,"id":0,"ch":0,"r":7,"g":7,"b":7}` | Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback. |
| 211 | Legacy Node flush | `{"T":211,"id":0,"num":2}` | Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback. |
| 212 | Legacy Node all LEDs | `{"T":212,"id":0,"num":2,"r":7,"g":7,"b":7}` | Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback. |

## Missions

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 300 | List missions | `{"T":300}` | offset defaults to 0; 16 names per page, next=-1 ends pagination. Includes unfinished drafts. |
| 301 | Create mission | `{"T":301,"name":"demo","intro":"introduction for this mission file."}` | name: 1–32 ASCII letters, digits, underscores or hyphens. intro: at most 96 UTF-8 bytes. Existing missions are not overwritten. |
| 302 | Read mission | `{"T":302,"name":"demo"}` | offset is a zero-based step offset; up to 8 steps per page, returns intro/steps/total/next. |
| 303 | Append mission step | `{"T":303,"name":"demo","json":"{\"T\":51,\"delay\":500}"}` | json is an escaped JSON string. Maximum 64 steps and 16 KB per file; no rid or T=300…399 inside a step. |
| 304 | Insert mission step | `{"T":304,"name":"demo","step":1,"json":"{\"T\":51,\"delay\":500}"}` | step is 1-based; insert before that step. json is an escaped JSON string. |
| 305 | Replace mission step | `{"T":305,"name":"demo","step":1,"json":"{\"T\":51,\"delay\":500}"}` | step is 1-based; replaces an existing step. json is an escaped JSON string. |
| 306 | Delete mission step | `{"T":306,"name":"demo","step":1}` | step is 1-based; deletes the specified step. |
| 307 | Run one mission step | `{"T":307,"name":"demo","step":1}` | step is 1-based; executes one saved step and may cause motion. |
| 308 | Run mission | `{"T":308,"name":"demo","interval":1000,"loop":1}` | interval: extra delay after each step, 0–60000 ms. loop: 1–10000 or -1 for infinite. T=0 exits the loop. |
| 309 | Delete mission | `{"T":309,"name":"demo"}` | Deletes a saved mission; cannot be undone. |
| 310 | Commit verified draft | `{"T":310,"source":"draft_upload","name":"demo","overwrite":false}` | source must start with draft_; name is the destination. Overwriting requires explicit overwrite:true. |

## Bus devices

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 700 | Scan bus IDs | `{"T":700,"first":0,"last":20}` | Set fields as shown; verify target hardware and power before execution. |
| 710 | TTL Node change ID | `{"T":710,"id":1,"new_id":2}` | Set fields as shown; verify target hardware and power before execution. |
| 711 | TTL Node PWM | `{"T":711,"id":1,"ch":1,"pwm":0}` | Set fields as shown; verify target hardware and power before execution. |
| 712 | TTL Node RGB | `{"T":712,"id":1,"ch":0,"r":1,"g":0,"b":0}` | Set fields as shown; verify target hardware and power before execution. |
| 713 | TTL Node read S.BUS | `{"T":713,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 720 | Stepper / encoder change ID | `{"T":720,"id":1,"new_id":2}` | Distinct id/new_id integers in 0–253; connect devices individually. Verifies the new ID. Shared by encoders and steppers. |
| 721 | Stepper mode | `{"T":721,"id":1,"mode":0}` | mode=0 position, 1 velocity, 2 follower. Mode 2 requires leader_id (secondary ID, 0–253, distinct from own primary ID) matching the leader. Mode writes disable output; switching 0/1 needs no restart. |
| 722 | Stepper position reference | `{"T":722,"id":1,"pos":0}` | Set fields as shown; verify target hardware and power before execution. |
| 723 | Stepper feedback | `{"T":723,"id":1}` | Set fields as shown; verify target hardware and power before execution. |
| 724 | Stepper position target | `{"T":724,"id":1,"pos":3200,"speed":100,"acc":10,"current":150}` | pos=0–65534, speed=1–32767, acc=0–254, current=0–1000. Register units; current is not amperes. Set position mode and enable first. |
| 725 | Stepper continuous speed | `{"T":725,"id":1,"speed":0,"acc":10,"current":150}` | speed=-32767–32767, 0 stops; acc=0–254, current=0–1000. Set velocity mode and enable first. |
| 726 | Stepper output enable | `{"T":726,"id":1,"enable":0}` | enable=0 disables output, 1 enables it. |
| 730 | Encoder position reference | `{"T":730,"id":1,"pos":0}` | pos=0–65534; saves current position reference without motion. |
| 732 | Encoder position and speed | `{"T":732,"id":1}` | Returns pos/speed; 4096 counts/turn, multi-turn position 0–65534. Accumulated turns do not survive power loss. |

## Onboard IMU

| T | Function | Send JSON | Parameters |
| --- | --- | --- | --- |
| 740 | Read onboard IMU | `{"T":740}` | Set fields as shown; verify target hardware and power before execution. |
