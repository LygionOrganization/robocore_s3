"""Refresh grouped API tables while preserving the reviewed guide introduction."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
commands=json.loads((ROOT/'docs/command-catalog.json').read_text(encoding='utf-8'))
for lang,file in [('zh','ROBOCORE_API.md'),('en','ROBOCORE_API_EN.md')]:
    p=ROOT/'docs'/file
    text=p.read_text(encoding='utf-8').split('<!-- COMMAND_TABLES -->')[0]+'<!-- COMMAND_TABLES -->\n'
    for group in dict.fromkeys(c['group'][lang] for c in commands):
        text+='\n## '+group+'\n\n'+('| T | 功能 | 发送 JSON | 参数说明 |\n' if lang=='zh' else '| T | Function | Send JSON | Parameters |\n')+'| --- | --- | --- | --- |\n'
        for c in commands:
            if c['group'][lang]!=group: continue
            text+=f"| {c['T']} | {c['title'][lang]} | `"+json.dumps(c['sample'],ensure_ascii=False,separators=(',',':'))+'` | '+c['note'][lang].replace('|','\\|')+' |\n'
    p.write_text(text,encoding='utf-8')
    (ROOT/'docs/robocore-wiki'/lang/'json-api.md').write_text(text,encoding='utf-8')
print('Updated grouped bilingual API tables')
