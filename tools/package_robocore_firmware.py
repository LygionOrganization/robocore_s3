"""Package one successful build for Flash Download Tool, without flashing."""
from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parents[1]
build = root / '.pio/build/esp32-s3-devkitc-1-n8r8'
files = {'bootloader.bin': '0x0', 'partitions.bin': '0x8000', 'firmware.bin': '0x10000'}
manifest = {'board': 'ESP32-S3 8MB flash', 'hardware_validated': False, 'files': {}}
for name, address in files.items():
    data = (build / name).read_bytes()
    manifest['files'][name] = {'offset': address, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
target = root / '.pio/robocore-firmware.zip'
with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
    for name in files:
        archive.write(build / name, name)
    archive.writestr('manifest.json', json.dumps(manifest, indent=2))
    archive.write(root / 'platformio.ini', 'platformio.ini')
    archive.write(root / 'tools/pio_python_path.py', 'tools/pio_python_path.py')
    archive.writestr('README.txt', '''RoboCore S3 firmware
Restore the RoboCore S3 web console and JSON control firmware.
Use the existing ESP32-S3 Flash Download Tool with these three files from this archive:
bootloader.bin  0x0
partitions.bin  0x8000
firmware.bin    0x10000
Flash size: 8MB. Do not write firmware.bin at 0x0.
Disconnect actuators/motor power before recovery. Erasing flash removes saved settings and missions.
See the RoboCore S3 wiki firmware recovery tutorial for boot mode and tool instructions.
''')
print(target)
