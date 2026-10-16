"""Build and package the Windows download tool from the current working tree.

No serial ports are opened. Only allowlisted vendor files and fresh build outputs
are packaged; previous logs, cached images and single-image updaters are excluded.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import configparser
import hashlib
import io
import json
import os
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ENV = 'esp32-s3-devkitc-1-n8r8'
VENDOR = ROOT / 'download_tool/RD_ESP32S3_LITE_Download_Tool'
DEST = ROOT / 'download_tool/RoboCore_S3_Download_Tool'
EXE = 'flash_download_tool_3.9.9_R2.exe'
OFFSETS = {'bootloader.bin': 0x0, 'partitions.bin': 0x8000, 'firmware.bin': 0x10000}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git(*args):
    return subprocess.check_output(
        ['git', '-c', f'safe.directory={ROOT.as_posix()}', *args],
        cwd=ROOT, text=True, encoding='utf-8').strip()


def main():
    managed_pio = Path.home() / '.platformio/penv' / ('Scripts/pio.exe' if os.name == 'nt' else 'bin/pio')
    pio = str(managed_pio) if managed_pio.is_file() else shutil.which('pio')
    if not pio:
        raise SystemExit('Install PlatformIO before building this package.')
    subprocess.run([sys.executable, 'tools/embed_web.py'], cwd=ROOT, check=True)
    subprocess.run([pio, 'run', '-e', ENV], cwd=ROOT, check=True)
    build = ROOT / '.pio/build' / ENV
    images = {name: (build / name).read_bytes() for name in OFFSETS}
    assert images['bootloader.bin'][0] == images['firmware.bin'][0] == 0xE9
    assert len(images['bootloader.bin']) <= 0x8000
    assert len(images['partitions.bin']) <= 0x1000
    # Check that the application fits the actual partition table from this build.
    entries = []
    for i in range(0, len(images['partitions.bin']), 32):
        block = images['partitions.bin'][i:i + 32]
        magic, kind, subtype, offset, size, label, flags = struct.unpack('<HBBII16sI', block)
        if magic != 0x50AA:
            break
        entries.append({'type': kind, 'subtype': subtype, 'offset': offset,
                        'size': size, 'label': label.rstrip(b'\0').decode()})
    app = next(p for p in entries if p['type'] == 0 and p['offset'] == 0x10000)
    assert len(images['firmware.bin']) <= app['size']
    assert all(p['offset'] + p['size'] <= 8 * 1024 * 1024 for p in entries)

    files = {f'bin/{name}': data for name, data in images.items()}
    files[EXE] = (VENDOR / EXE).read_bytes()
    files['release_note.txt'] = (VENDOR / 'release_note.txt').read_bytes()
    for name in ('utility.conf', 'security.conf'):
        files[f'configure/esp32s3/{name}'] = (VENDOR / 'configure/esp32s3' / name).read_bytes()
    config = configparser.ConfigParser()
    config.read(VENDOR / 'configure/esp32s3/multi_download.conf', encoding='utf-8')
    for i in range(14):
        selected = i < len(OFFSETS)
        name = list(OFFSETS)[i] if selected else ''
        config['DOWNLOAD PATH'].update({f'file_sel{i}': str(int(selected)),
            f'file_path{i}': 'bin\\' + name if selected else '',
            f'file_offset{i}': hex(OFFSETS[name]) if selected else '',
            f'file_flag{i}': 'False'})
    config['DOWNLOAD PATH']['default_path'] = './bin/'
    config['LOCK']['lock_setting_password'] = ''
    for key in config['DOWNLOAD']:
        if key.startswith('com_port'):
            config['DOWNLOAD'][key] = ''
        elif key.startswith('autostart'):
            config['DOWNLOAD'][key] = '0'
    for key in config['STATISTICS']:
        config['STATISTICS'][key] = '0'
    config['ESPTOOL_PARAM'].update({'verify': 'True', 'flash_size': 'keep',
                                  'flash_freq': 'keep', 'flash_mode': 'keep'})
    output = io.StringIO()
    config.write(output)
    files['configure/esp32s3/multi_download.conf'] = output.getvalue().encode()
    for name in ('README.md', 'README.zh-CN.md', 'Start_Download_Tool.bat', 'Verify_Package.ps1'):
        files[name] = (ROOT / 'tools/download_tool' / name).read_bytes()

    source_hashes = {}
    for folder in ('src', 'include', 'lib', 'web'):
        for path in sorted((ROOT / folder).rglob('*')):
            if path.is_file() and path.suffix in ('.h', '.cpp', '.c', '.ino', '.html', '.js', '.properties', '.json'):
                source_hashes[path.relative_to(ROOT).as_posix()] = digest(path.read_bytes())
    for name in ('platformio.ini', 'tools/embed_web.py', 'tools/pio_python_path.py'):
        source_hashes[name] = digest((ROOT / name).read_bytes())
    manifest = {
        'package': 'RoboCore_S3_Download_Tool',
        'built_at': datetime.now(timezone(timedelta(hours=8))).isoformat(timespec='seconds'),
        'supported_boards': ['RoboCore S3', 'Robot Driver with ESP32S3 Lite'],
        'environment': ENV, 'flash_size': '8MB', 'hardware_validated': False,
        'source_commit': git('rev-parse', 'HEAD'),
        'working_tree_dirty': bool(git('status', '--porcelain')),
        'source_tree_sha256': digest(json.dumps(source_hashes, sort_keys=True).encode()),
        'source_files': source_hashes,
        'images': {f'bin/{name}': hex(offset) for name, offset in OFFSETS.items()},
        'partitions': entries,
        'files': {name: {'bytes': len(data), 'sha256': digest(data)} for name, data in files.items()},
    }
    files['manifest.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    files['SHA256SUMS.txt'] = ''.join(f'{digest(data)}  {name}\n' for name, data in files.items()).encode()
    # Write exactly the named release files; never recurse through runtime data.
    for name, data in files.items():
        target = DEST / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.is_file() or target.read_bytes() != data:
            target.write_bytes(data)
    archive = ROOT / '.pio/RoboCore_S3_Download_Tool.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in files.items():
            z.writestr('RoboCore_S3_Download_Tool/' + name, data)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(files)
        for name, data in files.items():
            assert z.read('RoboCore_S3_Download_Tool/' + name) == data
    subprocess.run([sys.executable, 'tools/package_robocore_firmware.py'], cwd=ROOT, check=True)
    print(f'Validated package: {archive}', flush=True)


if __name__ == '__main__':
    main()
