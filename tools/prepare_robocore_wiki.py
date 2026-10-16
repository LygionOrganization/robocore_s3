"""Prepare bilingual RoboCore S3 docs and allowlisted download assets.

Edit docs/robocore-wiki/{zh,en}/*.md. Private design documents are not inputs.
"""
from pathlib import Path
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
stage = root / '.pio/wiki_staging'
for locale, sub in [('zh', ''), ('en', 'en')]:
    target = stage / 'docs' / sub / 'reference/bus-devices/robocore-s3'
    target.mkdir(parents=True, exist_ok=True)
    for source in (root / 'docs/robocore-wiki' / locale).rglob('*.md'):
        destination = target / source.relative_to(root / "docs/robocore-wiki" / locale)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)

# The Chinese API table is shared with the firmware documentation.
api = (root/'docs/ROBOCORE_API.md').read_text(encoding='utf-8')
(stage/'docs/reference/bus-devices/robocore-s3/json-api.md').write_text(api,encoding='utf-8')
assets = stage/'docs/assets/files/robocore-s3'; assets.mkdir(parents=True,exist_ok=True)
shutil.copyfile(root/'.pio/robocore-firmware.zip', assets/'firmware.zip')
if (root/'.pio/RoboCore_S3_Download_Tool.zip').is_file():
    shutil.copyfile(root/'.pio/RoboCore_S3_Download_Tool.zip', assets/'download-tool.zip')
shutil.copyfile(root/'docs/ROBOCORE_AI_AGENTS.md', assets/'AGENTS.md')
with zipfile.ZipFile(assets/'examples.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for folder in ('arduino_example','lib/lygion_devs','lib/RoboCoreSupport','python_example/robocore'):
        for path in (root/folder).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts: archive.write(path,path.relative_to(root))
    archive.write(root/'docs/ROBOCORE_API.md','docs/ROBOCORE_API.md')
    archive.write(root/'tools/pio_python_path.py','tools/pio_python_path.py')
    archive.write(root/'docs/ROBOCORE_AI_AGENTS.md','AGENTS.md')
print('Prepared bilingual source and example package:',stage)

shutil.copyfile(root/'docs/ROBOCORE_API_EN.md', stage/'docs/en/reference/bus-devices/robocore-s3/json-api.md')
with zipfile.ZipFile(assets/'ai-guide.zip','w',zipfile.ZIP_DEFLATED) as archive:
    archive.write(root/'docs/ROBOCORE_AI_AGENTS.md','AGENTS.md')
for p in (stage/'docs').rglob('*.md'):
    text=p.read_text(encoding='utf-8').replace('files/robocore-s3/AGENTS.md','files/robocore-s3/ai-guide.zip')
    text=text.replace('[classic console](/legacy)', 'classic console')
    p.write_text(text,encoding='utf-8')
