"""Host regression tests; requires MSVC Build Tools and installed ArduinoJson."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / '.pio' / 'usb_cdc_host_tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'src/main.cpp').read_text(encoding='utf-8')

def function(signature):
    start = source.index(signature + ' {')
    end = source.index('\n}', start) + 2
    return source[start:end]

(out / 'usb_cdc_production.inc').write_text(
    function('void addJsonCmd2Queue(const char* jsonString)') + '\n' +
    function('void usbCdcEvent(void*, esp_event_base_t base, int32_t eventId, void*)'),
    encoding='utf-8')
vs = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio'
vcvars = next(vs.glob('*/BuildTools/VC/Auxiliary/Build/vcvars64.bat'))
arduinojson = next((root / '.pio/libdeps').glob('*/ArduinoJson/src'))
command = (
    f'call "{vcvars}" >nul\n'
    f'cl /nologo /std:c++17 /EHsc /W4 /D_CRT_SECURE_NO_WARNINGS '
    f'/I"{root / "include"}" /I"{arduinojson}" /I"{out}" '
    f'"{root / "test/usb_cdc_host_test.cpp"}" /Fe:"{out / "tests.exe"}" '
    f'/Fo:"{out / "tests.obj"}"\n'
    'if errorlevel 1 exit /b 1\n'
    f'"{out / "tests.exe"}"\n'
)
batch = out / 'run.cmd'
batch.write_text(command, encoding='utf-8')
subprocess.run(['cmd.exe', '/c', str(batch)], cwd=root, check=True)
