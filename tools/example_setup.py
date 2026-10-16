"""Shared standalone-sketch console and dependency instructions."""
import re

def prepare(name, source):
    source = re.sub(r'^// BEGIN EXAMPLE SETUP\n.*?// END EXAMPLE SETUP\n', '', source, flags=re.S)
    lines = ['// BEGIN EXAMPLE SETUP',
             '// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).']
    if name == 'USB_CDC_JSON':
        lines += ['// Connect Type-C USB; select USB-OTG (TinyUSB), USB CDC On Boot = Enabled.']
    else:
        source = re.sub(r'\bSerial\b', 'Serial0', source)
        lines += ['// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.',
                  '// Keep the upload cable connected to Type-C UART for feedback and commands.',
                  '// Recommended Arduino setting: USB CDC On Boot = Disabled.',
                  '// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.']
    deps = []
    if '#include <SCServo.h>' in source:
        deps += ['FTServo 2.0.0 by ftservo / FEETECH (provides SCServo.h).']
    if '#include <ArduinoJson.h>' in source:
        deps += ['ArduinoJson 7.4.3 by Benoit Blanchon.']
    if '#include <AsyncTCP.h>' in source:
        deps += ['AsyncTCP 3.5.0 and ESPAsyncWebServer 3.12.1 by ESP32Async.']
    if '#include <Adafruit_SSD1306.h>' in source:
        deps += ['Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, and Adafruit BusIO.']
    if '#include <Adafruit_NeoPixel.h>' in source:
        deps += ['Adafruit NeoPixel 1.15.5 by Adafruit.']
    for dep in deps:
        lines += ['// Library Manager: install ' + dep]
    bundled = []
    if '#include <lygion_devs.h>' in source: bundled.append('lygion_devs')
    if '#include <OptionalQMI8658.h>' in source: bundled.append('RoboCoreSupport')
    for lib in bundled:
        lines += [f'// Bundled library: copy the entire lib/{lib} folder from this project',
                  f'// to your Arduino sketchbook libraries/{lib} folder, then restart the IDE.']
    if not deps and not bundled:
        lines += ['// No external libraries required; all headers are included with the ESP32 board package.']
    else:
        lines += ['// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.',
                  '// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.']
    return '\n'.join(lines + ['// END EXAMPLE SETUP']) + '\n' + source
