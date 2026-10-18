"""Install the prepared RoboCore series and targeted corrections into the wiki.

Only writes the named docs/assets and mkdocs navigation; never deletes content.
"""
from pathlib import Path
import shutil
import argparse

root = Path(__file__).resolve().parents[1]
stage = root / '.pio/wiki_staging'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('wiki', type=Path, help='Path to the lygion-wiki checkout')
wiki = parser.parse_args().wiki.resolve()
assert (wiki / 'mkdocs.yml').is_file()
for source in (stage / 'docs').rglob('*'):
    if not source.is_file(): continue
    target = (wiki / source.relative_to(stage)).resolve()
    assert target.is_relative_to(wiki)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)

titles_zh = [('quickstart','快速开始'),('hardware','供电与接口'),('web-console','网页控制台'),
             ('devices','总线设备控制'),('json-api','JSON 接口'),('automation','网页自动化脚本'),('python','电脑/上位机控制'),
             ('arduino','Arduino 单元例程'),('firmware','固件恢复'),('ai-development','AI 辅助开发'),('faq','故障排查')]
titles_en = [('quickstart','Quick Start'),('hardware','Power and Wiring'),('web-console','Web Console'),
             ('devices','Bus Devices'),('json-api','JSON API'),('automation','Web Automation'),('python','PC / Host Control'),
             ('arduino','Arduino Examples'),('firmware','Firmware Recovery'),('ai-development','AI Development'),('faq','Troubleshooting')]
config = wiki / 'mkdocs.yml'
text = config.read_text(encoding='utf-8')
asset_page = 'assets/files/robocore-s3/AGENTS.md'
if asset_page not in text:
    assert 'not_in_nav: |' in text
    text = text.replace('not_in_nav: |', 'not_in_nav: |\n  ' + asset_page)
    config.write_text(text, encoding='utf-8')
if 'reference/bus-devices/robocore-s3/index.md' not in text:
    marker = '                    - Robot Driver with ESP32S3 Lite:'
    assert text.count(marker) == 2, 'Expected Chinese and English navigation blocks'
    for titles in (titles_zh, titles_en):
        block = '                    - RoboCore S3:\n                        - reference/bus-devices/robocore-s3/index.md\n'
        block += ''.join(f'                        - {label}: reference/bus-devices/robocore-s3/{slug}.md\n' for slug,label in titles)
        # Replace successive original blocks, not the marker inserted in a previous pass.
        index = text.find(marker, 0 if titles is titles_zh else text.find('        - locale: en'))
        assert index >= 0
        text = text[:index] + block + text[index:]
    config.write_text(text, encoding='utf-8')

for locale in ('','en/'):
    folder = wiki / 'docs' / locale / 'reference/bus-devices'
    p = folder / 'ttl-stepper-driver-a/operating-modes.md'
    text = p.read_text(encoding='utf-8')
    if locale:
        text = text.replace('Disconnect the EDS interface before changing an operating mode. Power-cycle the driver after changing the mode.',
            'Follow the EDS wiring procedure when configuring follower operation. Modes 0 (position) and 1 (continuous speed) can be switched freely during a single power session without restarting. A restart is required only after changing phase configuration.')
        text = text.replace('!!! warning "Disconnect EDS before changing modes"','!!! note "Mode switching and phase changes are different"')
    else:
        text = text.replace('!!! warning "EDS接口 与 断电重启"\n    更改运行模式前，不要连接 EDS 接口，避免损害从动设备。\n    \n    更改模式后，需要断电重启后才可以正常使用。',
            '!!! note "运行模式与相位配置不同"\n    模式 0（位置）与模式 1（连续转动）可以在单次上电期间自由切换，无需重启。只有更改相位配置时才需要重启。配置从动同步时请按 EDS 接线流程操作。')
    p.write_text(text, encoding='utf-8')
    # The checked-in C++ SDK exposes SetMode, not WheelMode.
    p = folder / 'ttl-stepper-driver-a/cpp-arduino.md'
    text = p.read_text(encoding='utf-8').replace('ttlsd.WheelMode(1);',
        'ttlsd.unLockEprom(1);\nttlsd.SetMode(1, 1); // Continuous-speed mode; no restart required.\nttlsd.LockEprom(1);\nttlsd.EnableTorque(1, 1);')
    p.write_text(text, encoding='utf-8')
    p = folder / 'index.md'; text = p.read_text(encoding='utf-8')
    if 'robocore-s3/index.md' not in text:
        row = ('| Controller | RoboCore S3 | Compact 5–48V controller, onboard IMU and Web device tools | [Open guide](robocore-s3/index.md) |\n' if locale else
               '| 控制板 | RoboCore S3 | 紧凑型 5–48V 主控、板载 IMU、网页总线设备控制 | [查看文档](robocore-s3/index.md) |\n')
        lines = text.splitlines(keepends=True)
        index = next(i for i,line in enumerate(lines) if '| Robot Driver with ESP32S3 Lite |' in line)
        lines.insert(index,row); p.write_text(''.join(lines),encoding='utf-8')
print('Installed RoboCore documentation and corrected stepper mode instructions:', wiki)
