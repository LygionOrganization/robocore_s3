"""Prefer executables belonging to PlatformIO's Python environment.

pioarduino 55.03.30 searches Scripts/Scripts on Windows and falls back to
bare esptool. Without this path, a .py editor association may intercept it.
"""
from pathlib import Path
Import('env')
python_dir = Path(env.subst('$PYTHONEXE').strip('"')).parent
env.PrependENVPath('PATH', str(python_dir / 'Scripts'))
env.PrependENVPath('PATH', str(python_dir))
