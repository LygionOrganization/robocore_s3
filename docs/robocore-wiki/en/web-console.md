# Web console

The console opens in English. Use **中文** at the upper right for Chinese, or **English** to switch back. Your browser remembers the choice when local storage is available. Switching preserves inputs and sends no commands; JSON fields and raw device replies remain unchanged.

All pages are embedded in the firmware, with no internet or CDN requirement. Desktop navigation uses simple blue dots; phones use the horizontally scrollable top navigation.

| Page | Purpose |
| --- | --- |
| Dashboard | Baud, scan count and onboard IMU status |
| Bus devices | Discover IDs and fill control panels with the selected ID |
| TTL Node | ID, PWM, RGB and S.BUS |
| Stepper driver | Mode, output enable, position/speed, reference and ID |
| Encoder | E02 position, speed and reference calibration |
| Bus servos | SCS, HLS and STS/SMS position and feedback |
| Onboard IMU | Onboard QMI8658A acceleration, angular velocity and temperature |
| JSON tools | Commands, result log, complete command reference and mission scripts |
| Settings | Wi-Fi, controller baud and firmware information |

## JSON tools

Enter one JSON object and click Send command. Results match by rid; HTTP202 means accepted. `ok:false` indicates failure, while legacy `dispatched:true` only confirms dispatch, not successful physical motion. Timeouts do not trigger retries; inspect state first.

Search the command reference by category or keyword. Expand an entry for its purpose and example. Load command fills the single-command editor; Add to script appends to the mission editor. Neither sends a command. See the [JSON reference](json-api.md) for parameters. Device controls and Wi-Fi settings remain in their dedicated pages.

## Create and save a mission

1. Enter a name, description and one JSON object per line. Use `{"T":51,"delay":500}` to wait500ms between actions.
2. Click Verify and save to board. The editor uploads a draft with per-step acknowledgements, reads it back, then replaces the target. Overwriting requires confirmation.
3. Refresh the list and load the saved mission to inspect it. Recover interrupted uploads by loading a `draft_` entry.
4. Choose extra delay per step and repeat count, then Run saved mission. Unsaved editor changes do not run; execution requires confirmation.
5. Export file saves local editor contents. Import file only loads the editor; save again to write to the board. Deletion is irreversible, so export anything you wish to keep.

Names contain1–32 ASCII letters, digits, underscores or hyphens; up to64 steps. Steps cannot contain rid or invoke nested missions. `boot` is reserved; `boot_user` loops automatically at startup. Review every action and include delays before using it. Use custom names for ordinary missions.

## Stop loop

The button in JSON tools sends T=0 to exit an interruptible mission and clear pending commands; it is not motor emergency stop. Use zero speed or output disable to stop a continuous stepper. Stop missions before editing/debugging. Multiple hosts share the latest-command mailbox and may overwrite pending commands.

The old `/legacy` address redirects to JSON tools, where synchronized-command examples and scripts are now integrated.
