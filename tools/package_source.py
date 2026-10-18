"""Create a public source archive, excluding local state and internal archives."""
from pathlib import Path
import hashlib
import json
import re
import zipfile
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
FOLDERS = ('src', 'include', 'lib', 'web', 'arduino_example', 'python_example',
           'docs', 'test', 'tools', 'README')
ROOT_FILES = ('README.md', 'README.zh-CN.md', 'AGENTS.md', 'LICENSE',
              'platformio.ini', '.gitignore')
SUFFIXES = {'.md', '.h', '.hpp', '.cpp', '.c', '.ino', '.py', '.js', '.cjs',
            '.html', '.css', '.json', '.txt', '.properties', '.png', '.jpg',
            '.svg', '.ps1', '.bat', '.gitignore'}
SKIP = {'.git', '.pio', '.vscode', '__pycache__', 'node_modules', 'logs', 'dl_temp'}


def main():
    paths = {Path(name) for name in ROOT_FILES}
    for folder in FOLDERS:
        for p in (ROOT / folder).rglob('*'):
            rel = p.relative_to(ROOT)
            if p.is_file() and not SKIP.intersection(rel.parts):
                if p.suffix in SUFFIXES or p.name in {'README', 'LICENSE', '.gitignore'}:
                    paths.add(rel)
                else:
                    raise ValueError(f'Review unknown source file type before publishing: {rel}')
    # The current release's explicit manifest is the only input for binary assets.
    tool = Path('download_tool/RoboCore_S3_Download_Tool')
    manifest = json.loads((ROOT / tool / 'manifest.json').read_text(encoding='utf-8'))
    for name, entry in manifest['files'].items():
        rel = tool / name
        full = (ROOT / rel).resolve()
        if not full.is_relative_to((ROOT / tool).resolve()) or SKIP.intersection(rel.parts):
            raise ValueError(f'Invalid distribution path: {name}')
        if hashlib.sha256(full.read_bytes()).hexdigest() != entry['sha256']:
            raise ValueError(f'Rebuild download tool first: {name}')
        paths.add(rel)
    paths.update(tool / name for name in ('manifest.json', 'SHA256SUMS.txt'))
    for name, expected in manifest['source_files'].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise ValueError(f'Firmware does not match source; rebuild download tool: {name}')
    # Check the customer entry-point documents (wiki links follow site routing).
    for name in (*ROOT_FILES[:3], 'docs/RELEASING.md', 'docs/ROBOCORE_VALIDATION.md'):
        p = ROOT / name
        for link in re.findall(r'\]\(([^)]+)\)', p.read_text(encoding='utf-8')):
            link = unquote(link.split('#')[0])
            if not link or ':' in link or link.startswith('/'):
                continue
            target = (p.parent / link).resolve()
            if not target.is_relative_to(ROOT) or not target.exists():
                raise ValueError(f'Broken local link in {name}: {link}')
            if target.is_file() and target.relative_to(ROOT) not in paths:
                raise ValueError(f'Linked file excluded from release: {link}')
    archive = ROOT / '.pio/RoboCore_S3_Source.zip'
    archive.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for rel in sorted(paths):
            z.write(ROOT / rel, 'RoboCore_S3/' + rel.as_posix())
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert not any(SKIP.intersection(Path(n).parts) for n in z.namelist())
        assert not any('ROBOCORE_PROGRESS' in n for n in z.namelist())
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix('.zip.sha256').write_text(f'{checksum}  {archive.name}\n', encoding='ascii')
    print(f'Validated {len(paths)} files: {archive}')


if __name__ == '__main__':
    main()
