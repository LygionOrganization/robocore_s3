"""Build the offline console catalog and Wiki command index from reviewed metadata."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
config = (ROOT/'src/Config.h').read_text(encoding='utf-8')
samples = {}
for match in re.finditer(r'^// (\{"T":\d+.*\})', config, re.M):
    try:
        sample = json.loads(match[1]); samples.setdefault(sample['T'], sample)
    except ValueError:
        pass

# Every entry is an implemented command, not merely a constant in Config.h.
rows = '''0|停止任务循环|Stop mission loop
10|设置主控总线波特率|Set controller bus baud rate
11|STS/SMS 位置控制|STS/SMS position
12|STS/SMS 设置中位|STS/SMS calibrate midpoint
13|STS/SMS 修改 ID|STS/SMS change ID
14|STS/SMS 扭矩开关|STS/SMS torque enable
15|STS/SMS 状态反馈|STS/SMS feedback
16|STS/SMS 同步控制|STS/SMS synchronized targets
17|STS/SMS 简化同步控制|STS/SMS simple synchronized targets
21|HLS 位置控制|HLS position
22|HLS 设置中位|HLS calibrate midpoint
23|HLS 修改 ID|HLS change ID
24|HLS 扭矩开关|HLS torque enable
25|HLS 状态反馈|HLS feedback
26|HLS 同步控制|HLS synchronized targets
27|HLS 简化同步控制|HLS simple synchronized targets
31|SCS 位置控制|SCS position
33|SCS 修改 ID|SCS change ID
34|SCS 扭矩开关|SCS torque enable
35|SCS 状态反馈|SCS feedback
36|SCS 同步控制|SCS synchronized targets
37|SCS 简化同步控制|SCS simple synchronized targets
51|等待毫秒数|Wait milliseconds
100|设置主控总线波特率|Set controller bus baud rate
101|选择通用舵机类型|Select generic servo type
102|设置舵机坐标换算|Set servo coordinate conversion
103|通用舵机反馈|Generic servo feedback
104|通用舵机 Ping|Generic servo ping
105|通用舵机修改 ID|Generic servo change ID
106|通用舵机设置中位|Generic servo calibrate midpoint
107|通用舵机移动至中位|Generic servo move to midpoint
108|通用舵机扭矩开关|Generic servo torque enable
109|SCS 步数控制|SCS step position
112|SCS 角度控制|SCS degree target
115|SCS 弧度控制|SCS radian target
118|执行预置舵机动作|Trigger deferred servo moves
119|SCS 扭矩限制|SCS torque limit
130|读取机械臂零位|Read arm zero positions
131|设置机械臂零位|Set arm zero positions
132|读取机械臂位置|Read arm positions
133|以当前位置标定机械臂零位|Calibrate arm zeros at current pose
134|机械臂关节角度|Arm joint angles
135|机械臂关节弧度|Arm joint radians
136|机械臂 XYZG 目标|Arm XYZG target
137|机械臂 RBZG 目标|Arm RBZG target
138|机械臂平滑 XYZG 运动|Arm smooth XYZG move
139|机械臂平滑 RBZG 运动|Arm smooth RBZG move
140|机械臂最大关节速度|Arm maximum joint speed
141|读取机械臂反馈弧度|Read arm feedback radians
142|读取机械臂目标弧度|Read arm target radians
143|读取 XYZG 解算状态|Read XYZG IK state
144|读取 RBZG 解算状态|Read RBZG IK state
145|机械臂周期反馈|Arm periodic feedback
146|机械臂连续运动|Arm continuous motion
147|机械臂手动标定模式|Arm manual calibration mode
180|四路轮毂电机速度|Four hub motor speeds
200|兼容 LED 单灯颜色|Legacy LED single color
201|兼容 LED 全部颜色|Legacy LED all colors
202|OLED 单行文字|OLED single line
203|OLED 刷新|OLED refresh
204|OLED 四行文字|OLED four lines
205|OLED 清屏|OLED clear
206|外接模块蜂鸣器|External module buzzer
208|兼容 Node 修改 ID|Legacy Node change ID
209|兼容 Node PWM|Legacy Node PWM
210|兼容 Node 单灯颜色|Legacy Node single LED
211|兼容 Node 刷新|Legacy Node flush
212|兼容 Node 全灯颜色|Legacy Node all LEDs
300|列出任务|List missions
301|创建任务|Create mission
302|读取任务|Read mission
303|追加任务步骤|Append mission step
304|插入任务步骤|Insert mission step
305|替换任务步骤|Replace mission step
306|删除任务步骤|Delete mission step
307|执行一个任务步骤|Run one mission step
308|运行任务|Run mission
309|删除任务|Delete mission
310|保存已验证的草稿|Commit verified draft
399|清空文件系统|Erase filesystem
400|设置 Wi-Fi|Configure Wi-Fi
401|读取 Wi-Fi 配置|Read Wi-Fi configuration
402|读取热点 IP|Read AP IP
403|读取路由器分配的 IP|Read station IP
410|初始化 ESP-NOW|Initialize ESP-NOW
411|设置 ESP-NOW 接收模式|Set ESP-NOW receive mode
412|读取 MAC 地址|Read MAC address
413|发送 ESP-NOW JSON|Send ESP-NOW JSON
414|添加 ESP-NOW 对端|Add ESP-NOW peer
415|ESP-NOW 模拟 MAC 开关|ESP-NOW synthetic MAC option
501|发送 CAN 帧|Send CAN frame
600|重启主控|Restart controller
601|清空 NVS|Erase NVS
602|清空文件系统|Erase filesystem
603|读取消息输出设置|Read message output settings
604|设置消息输出|Set message outputs
605|UART0 转发模式|UART0 forwarding mode
700|扫描总线 ID|Scan bus IDs
710|TTL Node 修改 ID|TTL Node change ID
711|TTL Node PWM|TTL Node PWM
712|TTL Node RGB|TTL Node RGB
713|TTL Node 读取 S.BUS|TTL Node read S.BUS
720|步进驱动修改 ID|Stepper change ID
721|步进驱动模式|Stepper mode
722|步进驱动位置基准|Stepper position reference
723|步进驱动反馈|Stepper feedback
724|步进驱动位置目标|Stepper position target
725|步进驱动连续速度|Stepper continuous speed
726|步进驱动输出开关|Stepper output enable
730|编码器位置基准|Encoder position reference
732|编码器位置与速度|Encoder position and speed
740|读取板载惯性传感器|Read onboard IMU
extra = [
 {'T':700,'first':0,'last':20}, {'T':710,'id':1,'new_id':2},
 {'T':711,'id':1,'ch':1,'pwm':0}, {'T':712,'id':1,'ch':0,'r':1,'g':0,'b':0},
 {'T':713,'id':1}, {'T':720,'id':1,'new_id':2}, {'T':721,'id':1,'mode':0},
 {'T':722,'id':1,'pos':0}, {'T':723,'id':1},
 {'T':724,'id':1,'pos':3200,'speed':100,'acc':10,'current':150},
 {'T':725,'id':1,'speed':0,'acc':10,'current':150}, {'T':726,'id':1,'enable':0},
 {'T':730,'id':1,'pos':0}, {'T':732,'id':1}, {'T':740}]
samples.update({s['T']:s for s in extra})
# Keep examples at a modest nonzero speed; zero has model-specific semantics.
for n in (11,21,31,109,112,115):
    samples[n]['spd']=100
for n in (16,26,36):
    samples[n]['spd']=[100,100]
for n in range(301,310):
    if 'name' in samples[n]: samples[n]['name']='demo'
for n in (303,304,305): samples[n]['json']='{"T":51,"delay":500}'
samples[304]['step']=1; samples[305]['step']=1; samples[306]['step']=1; samples[307]['step']=1
samples[112]['spd']=10
samples[115]['spd']=0.2
samples[413]={'T':413,'mac':'FF:FF:FF:FF:FF:FF','data':'{"T":205}'}
notes = {
0:('请求退出任务循环、清空待执行指令；不会关闭电机输出。','Requests loop exit and clears pending commands; does not disable motors.'),
51:('delay：等待时间，毫秒；任务中使用可控制步骤节奏。','delay: milliseconds to wait; useful between mission steps.'),
101:('type=0 SCS、1 STS/SMS、2 HLS；优先使用明确区分型号的指令。','type=0 SCS, 1 STS/SMS, 2 HLS; prefer the model-specific commands.'),
102:('steps 为舵机每圈计数，angle 为行程角度；这是舵机坐标换算，与外接编码器无关。','steps is servo counts per turn; angle is travel in degrees. This changes servo conversion, not an external encoder.'),
119:('兼容高级接口；请使用对应型号驱动库配置扭矩限制。','Legacy advanced interface; use the model-specific driver library for torque-limit configuration.'),
180:('A/B/C/D 为匹配的四路轮毂电机速度，寄存器值 × 0.06 = rpm；不是通用步进控制。','A/B/C/D control four compatible hub motors; register value × 0.06 = rpm. Not a generic stepper command.'),
206:('freq：Hz；duration：毫秒。蜂鸣器位于外接 UI Module (A)，GPIO21。','freq: Hz; duration: milliseconds. Buzzer is on the external UI Module (A), GPIO21.'),
300:('offset 默认为 0；每页 16 个名称，next=-1 表示结束；包含未完成草稿。','offset defaults to 0; 16 names per page, next=-1 ends pagination. Includes unfinished drafts.'),
301:('name：1–32 位英文字母、数字、下划线或连字符；intro 最多 96 UTF-8 字节；不能覆盖已有任务。','name: 1–32 ASCII letters, digits, underscores or hyphens. intro: at most 96 UTF-8 bytes. Existing missions are not overwritten.'),
302:('offset 为从 0 开始的步骤偏移；每页最多 8 步，返回 intro/steps/total/next。','offset is a zero-based step offset; up to 8 steps per page, returns intro/steps/total/next.'),
303:('json 为转义后的 JSON 字符串；最多 64 步、文件最多 16 KB；步骤不能含 rid 或 T=300…399。','json is an escaped JSON string. Maximum 64 steps and 16 KB per file; no rid or T=300…399 inside a step.'),
304:('step 从 1 开始，插入到指定步骤之前；json 为转义后的 JSON 字符串。','step is 1-based; insert before that step. json is an escaped JSON string.'),
305:('step 从 1 开始，替换现有步骤；json 为转义后的 JSON 字符串。','step is 1-based; replaces an existing step. json is an escaped JSON string.'),
306:('step 从 1 开始，删除指定步骤。','step is 1-based; deletes the specified step.'),
307:('step 从 1 开始；执行已保存的单步，可能产生运动。','step is 1-based; executes one saved step and may cause motion.'),
308:('interval 为每步后额外等待的毫秒数，0–60000；loop 为 1–10000 或 -1 无限循环。用 T=0 退出循环。','interval: extra delay after each step, 0–60000 ms. loop: 1–10000 or -1 for infinite. T=0 exits the loop.'),
309:('删除已保存任务，无法撤销。','Deletes a saved mission; cannot be undone.'),
310:('source 必须以 draft_ 开头；name 为目标名称。覆盖已有目标必须显式传 overwrite:true。','source must start with draft_; name is the destination. Overwriting requires explicit overwrite:true.'),
400:('mode=0 关闭 Wi-Fi，1 开启 AP+STA；channel 为信道，ap_* 为热点参数，sta_* 为路由器参数。更改后可能断线。','mode=0 disables Wi-Fi; 1 enables AP+STA. channel selects channel; ap_* configures hotspot, sta_* router credentials. Connection may drop.'),
410:('longrange：0 普通模式，1 长距离模式；双方需支持并使用兼容模式、同一信道。','longrange: 0 normal, 1 long range. Both peers need compatible mode and the same channel.'),
411:('mode：0 关闭接收，1 接收可信 MAC，2 接收广播。','mode: 0 receive off, 1 trusted MAC reception, 2 broadcast reception.'),
413:('mac 为冒号分隔的 MAC；data 为转义的 JSON 字符串。使用 250 字节补零帧，不能任意发送大对象。','mac is colon-separated; data is an escaped JSON string. Uses a zero-padded 250-byte frame, not arbitrary large objects.'),
414:('mac 为对端 MAC 地址；先确认双方 Wi-Fi 信道相同。','mac is the peer MAC; both devices must use the same Wi-Fi channel.'),
415:('fake 为兼容应用的模拟 MAC 开关；常规通信保持 0。','fake is a synthetic MAC option for legacy applications; keep 0 for normal communication.'),
501:('CAN 档；id 为标识符，ext=0 标准帧/1 扩展帧，data 最多 8 个 0–255 字节；默认 500 kbps。','CAN switch position; id is identifier, ext=0 standard/1 extended, data at most eight bytes 0–255; default 500 kbps.'),
600:('立即重启，连接中断；可能无法收到执行回复。','Restarts immediately and disconnects; an execution reply may not arrive.'),
601:('清空 NVS 设置；与任务文件系统不同。此操作不可撤销。','Erases NVS settings, separate from mission storage. Irreversible.'),
604:('echo/uart/usb 为 0 或 1；关闭 usb 后 USB 不再输出结果。','echo/uart/usb are 0 or 1; disabling usb suppresses USB result output.'),
605:('sf=1 转发 UART0 字节到舵机总线（默认）；sf=0 将 UART0 输入按 JSON 解析。','sf=1 forwards UART0 bytes to the servo bus (default); sf=0 parses UART0 input as JSON.'),
9999:('会以广播 ID 移动全部兼容电机，不用于日常控制。','Moves all compatible motors using broadcast ID; not for routine control.')}
reviewed=json.loads((ROOT/'docs/command-catalog.json').read_text(encoding='utf-8'))
reviewed={c['T']:c for c in reviewed}
catalog=[]
for row in rows.splitlines():
    n,zh,en=row.split('|'); n=int(n)
    if n==9999: continue
    group = ('任务脚本','Missions') if 300<=n<=310 else ('总线设备','Bus devices') if 700<=n<740 else ('板载惯性传感器','Onboard IMU') if n==740 else ('舵机','Servos') if 10<=n<=37 or 100<=n<=119 else ('机械臂预设','Arm presets') if 130<=n<=147 else ('外接模块','External modules') if 200<=n<=212 else ('通信与系统','Communication and system')
    note=notes.get(n,('按示例字段设置参数；执行前确认目标设备和供电。','Set fields as shown; verify target hardware and power before execution.'))
    if 130<=n<=147: note=('仅用于匹配固件几何参数、舵机 ID 和零位的机械臂；不是任意机械臂通用接口。数组顺序为四个关节/坐标分量。','Only for arms matching firmware geometry, servo IDs and zeros; not a universal arm API. Arrays contain four ordered joints/coordinates.')
    if n==17: note=('速度与加速度固定为0，0的含义按舵机型号；需要限速时用T=16。','Speed and acceleration are fixed at0; zero semantics depend on the servo. Use T=16 for explicit speed limits.')
    if n==27: note=('固定速度60、加速度50；tor 数组指定各轴电流限制。需要完整参数时用T=26。','Fixed speed60 and acceleration50; tor sets each axis current limit. Use T=26 for full parameters.')
    if n==37: note=('速度固定为0，可能代表最高速度；需要限速时用T=36。','Speed is fixed at0, which may mean maximum speed. Use T=36 for explicit speed limits.')
    if n in (399,602): note=('清空全部任务及文件系统内配置；不自动重启，无法撤销。','Erases all missions and filesystem configuration; does not restart automatically. Irreversible.')
    if n in (202,203,204,205): note=('需要外接 UI Module (A) 的 SSD1306 OLED；line 为 1–4，update=1 立即刷新。默认字体使用 ASCII。','Requires the external UI Module (A) SSD1306 OLED; line is 1–4, update=1 refreshes immediately. Default font uses ASCII.')
    if n in (208,209,210,211,212): note=('保留的旧 Node 接口；新程序建议使用 T=710…713 的参数校验和结构化反馈。','Retained legacy Node interface; prefer T=710…713 for validated fields and structured feedback.')
    catalog.append(dict(T=n,title=dict(zh=zh,en=en),group=dict(zh=group[0],en=group[1]),note=dict(zh=note[0],en=note[1]),sample=samples[n]))
catalog=[reviewed.get(c['T'],c) for c in catalog]
(ROOT/'docs/command-catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(f'Built {len(catalog)} command entries')
