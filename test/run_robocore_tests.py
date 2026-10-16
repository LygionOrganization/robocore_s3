"""Compile the production bus/IMU modules against deterministic host fakes."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / '.pio/robocore_host_tests'
out.mkdir(parents=True, exist_ok=True)
vs = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio'
vcvars = next(vs.glob('*/BuildTools/VC/Auxiliary/Build/vcvars64.bat'))
includes = [root / p for p in ('test/host_stubs', 'src', 'include', 'lib/RoboCoreSupport/src')]
includes.append(next((root / '.pio/libdeps').glob('*/ArduinoJson/src')))
command = f'call "{vcvars}" >nul\ncl /nologo /std:c++17 /EHsc /W4 '
command += ' '.join(f'/I"{p}"' for p in includes)
command += f' "{root / "test/robocore_host_test.cpp"}" "{root / "src/BusDevices.cpp"}" /Fe:tests.exe\n'
command += 'if errorlevel 1 exit /b 1\ntests.exe\n'
batch = out / 'run.cmd'
batch.write_text(command, encoding='utf-8')
subprocess.run(['cmd.exe', '/c', str(batch)], cwd=out, check=True)
