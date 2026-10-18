# 对外发布 / Public releases

## Release contents

- `README.md` / `README.zh-CN.md`: customer entry points.
- `AGENTS.md`: AI development map and constraints.
- `src`, `include`, `lib`, `web`: buildable firmware and editable UI.
- `arduino_example`, `python_example`: independent examples and host clients.
- `docs`, `test`, `tools`: API, verification and reproducible release tools.
- `download_tool/RoboCore_S3_Download_Tool`: current Windows flashing distribution.

本地 `.pio`、编辑器设置、Python/Node 缓存、下载工具运行日志和内部工作记录不属于
对外内容。已移出的历史工作材料可保留在本机 `.pio/internal_archive`，不要打包整个工作目录。
源码包脚本使用明确目录及文件类型清单，并检查 Markdown 本地链接。

## Build the release

1. Follow `ROBOCORE_VALIDATION.md`; update both language references when changing APIs.
2. Set the firmware version in `src/main.cpp`. Run `python tools/embed_web.py`.
3. Run `python tools/package_download_tool.py`. This builds firmware, packages only
   named vendor files plus fresh images, and verifies SHA-256 hashes. It does not flash.
   The existing current download-tool folder supplies the Espressif executable and
   configuration templates; retain vendor licenses and release notes.
4. Run `python tools/package_source.py`. This checks local Markdown links and creates
   `.pio/RoboCore_S3_Source.zip` with a SHA-256 sidecar. The source archive includes
   the current flashing distribution from its manifest, so rebuilding it is supported.
5. Distribute the source ZIP and `.pio/RoboCore_S3_Download_Tool.zip` as separate
   artifacts as needed. `.pio/robocore-firmware.zip` contains the matching three images.
6. Verify the actual board as described in the acceptance checklist. Record results
   for this build; package creation alone does not set `hardware_validated` to true.

The source package excludes Git history and local archives. The firmware manifest
contains source hashes; source archives without `.git` use a null commit identity.
Never mix bootloader, partition table and firmware from different builds.

## 1.3.8 API cleanup

IMU responses retain measurement units, `sample_count`, `sample_age_ms`, presence,
address and errors. Internal investigation counters and sampling-mode strings have
been removed; see the bilingual API reference for the exact fields. The tested
sampling, shared-bus locking, freshness checks and congestion protections are unchanged.
