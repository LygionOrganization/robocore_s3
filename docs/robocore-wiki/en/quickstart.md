# Quick start without coding

Connect the included external antenna before wireless use; see [package contents and antenna mounting](hardware.md).

Prepare a RoboCore S3, a data-capable USB cable, a phone or computer, and one bus device with its correctly rated external supply. Start with one device to avoid duplicate factory IDs.

## Connect

Wire with power removed. Follow the board labels for supply polarity, GND and TTL. USB communication does not mean the external actuator has motor power. Powered bus connectors carry the board input voltage.

## Join the network

The factory AP is `Robot`, password `12345678`. Stay connected when your phone reports “no Internet”; prevent automatic switching to cellular data. Enter `http://192.168.4.1` directly in the browser address bar.

If network settings were changed, use the current SSID and address. The OLED or router client list can help locate the board.

## Scan bus IDs

Open **Bus devices**, scan 0–253, and match the controller baud rate to the device (examples use 1Mbps). Results show IDs only. Click an ID to fill it into the control components; choose the device type yourself.

No result: check motor power, signal wiring, ground and baud rate. A scan cannot reliably distinguish devices with identical IDs. Configure those individually.

## Read before moving

- E02: open **Encoder**, read position, then turn the shaft by hand.
- TTL Node: open **TTL Node** and read S.BUS channels.
- Stepper: open **Stepper driver**, read position/speed, then follow the mode/enable sequence in [Bus devices](devices.md).

Network changes are under **Settings**. Applying settings may disconnect the current session; reconnect to the new AP or router-assigned address. Internet access is unnecessary.

Next: [Web console](web-console.md), [Bus devices](devices.md), [Troubleshooting](faq.md).


## Wi-Fi band and STA connection

RoboCore S3 uses 2.4 GHz Wi-Fi. The device joining its hotspot must support 2.4 GHz; most phones, tablets and laptops do, usually without extra setup. For STA mode, enable 2.4 GHz on the router; 5 GHz / 6 GHz-only networks are not supported.

Apply the router SSID and password in Settings. The page shows connecting, connected or unsuccessful, and the STA IP after connection. The OLED also updates when User Interface Module (A) is connected. Use the eye button to reveal passwords. Failed connections retain the settings; check the network and apply again. AP or channel changes can disconnect the browser; reconnect to the board hotspot to check status.
