"""Compile the production mission API against an in-memory filesystem."""
from pathlib import Path
import subprocess
import os
root = Path(__file__).resolve().parents[1]
out = root / '.pio/mission_tests'
out.mkdir(parents=True, exist_ok=True)
vs = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio'
vcvars = next(vs.glob('*/BuildTools/VC/Auxiliary/Build/vcvars64.bat'))
jsonlib = next((root / '.pio/libdeps').glob('*/ArduinoJson/src'))
batch = out / 'run.cmd'
batch.write_text(f'call "{vcvars}" >nul\ncl /nologo /std:c++17 /EHsc /W4 /I"{root / "include"}" /I"{jsonlib}" "{root / "test/mission_api_test.cpp"}" /Fe:tests.exe\nif errorlevel 1 exit /b 1\ntests.exe\n', encoding='utf-8')
subprocess.run(['cmd.exe', '/c', str(batch)], cwd=out, check=True)
