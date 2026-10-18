# Web automation scripts

Edit, save and run missions in JSON tools. Stop loop is also in JSON tools. Enter one JSON command per line; see [JSON API](json-api.md) for parameters.

**JSON tools → Mission scripts** provides list, load, edit, verified save, run, delete and local import/export. Enter one JSON object per line. Saving does not execute; Run uses the version saved on the board.

Names contain1–32 ASCII letters, digits, underscores or hyphens. `boot` is reserved for system configuration and cannot be accessed through this API. `boot_user` loops at startup: review every action before saving. `up/down/left/right` can serve configured UI-module key missions; use custom names for ordinary tasks.

Each mission allows64 steps and16KB. Each step is at most255 UTF-8 bytes and cannot contain rid or T=300…399. The outer T=303 upload also needs a name, escaping and rid, so the practical uploadable step is shorter; the web editor checks this before sending. Avoid repeating persistent ID/reference writes in loops.

Create a mission without motor movement below. Send each object separately, waiting for its reply:

```json
{"T":301,"name":"demo","intro":"Read onboard IMU twice","rid":1}
{"T":303,"name":"demo","json":"{\"T\":740}","rid":2}
{"T":303,"name":"demo","json":"{\"T\":51,\"delay\":500}","rid":3}
{"T":303,"name":"demo","json":"{\"T\":740}","rid":4}
{"T":302,"name":"demo","offset":0,"rid":5}
{"T":308,"name":"demo","interval":0,"loop":1,"rid":6}
```

T=308 returns status=started, then publishes status=finished with the same rid when execution ends. `/api/info` exposes mission_running. Completion means scheduling ended, not proof that each device action succeeded; inspect feedback. loop=-1 repeats indefinitely. T=0 requests loop exit, not motor emergency stop. Stop a running mission before editing or sending other commands.

The editor uploads a draft_, appends with acknowledgements, reads it back, then uses T=310 to replace the target. Interrupted uploads do not directly replace the target with a partial script; load a draft from the list to recover it. Existing targets require confirmation and overwrite:true. On timeout, inspect the list/content before retrying, because the save may already have completed.
