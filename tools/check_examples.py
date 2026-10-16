"""Compile every Arduino sketch with the project's installed PlatformIO toolchain.

This checks API compatibility without flashing any connected board.
"""
import argparse
import configparser
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--only', help='One example folder name')
args = parser.parse_args()
project = root / '.pio' / 'example_check'
(project / 'src').mkdir(parents=True, exist_ok=True)
config = configparser.ConfigParser()
config.read(root / 'platformio.ini', encoding='utf-8')
env = next(s for s in config.sections() if s.startswith('env:'))
config[env].pop('lib_deps', None)
config[env]['extra_scripts'] = 'pre:' + str(root / 'tools/pio_python_path.py')
config[env]['lib_extra_dirs'] = '\n' + str(root / 'lib') + '\n' + str(root / '.pio/libdeps' / env[4:])
config[env]['lib_ldf_mode'] = 'deep'
with (project / 'platformio.ini').open('w', encoding='utf-8') as f:
    config.write(f)
pio = Path(os.environ['USERPROFILE']) / '.platformio/penv/Scripts/pio.exe'
if not pio.exists():
    pio = Path(shutil.which('pio') or 'pio')
failed = []
sketches = sorted((root / 'arduino_example').glob('*/*.ino'))
if args.only:
    sketches = [p for p in sketches if p.parent.name == args.only]
    if not sketches:
        parser.error('Example not found')
for sketch in sketches:
    (project / 'src/main.cpp').write_text(sketch.read_text(encoding='utf-8'), encoding='utf-8')
    log = project / (sketch.stem + '.log')
    with log.open('w', encoding='utf-8') as output:
        result = subprocess.run([str(pio), 'run', '-d', str(project)], stdout=output, stderr=subprocess.STDOUT)
    print(('PASS ' if result.returncode == 0 else 'FAIL ') + sketch.stem, flush=True)
    if result.returncode:
        failed.append(sketch.stem)
        print(log.read_text(encoding='utf-8', errors='replace')[-6000:], flush=True)
print(f'{len(sketches)-len(failed)}/{len(sketches)} examples compiled.', flush=True)
sys.exit(bool(failed))

