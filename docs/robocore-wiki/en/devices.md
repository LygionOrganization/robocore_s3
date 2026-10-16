# Controlling bus devices

Scan first, select an ID, then choose the correct device component. Discovery does not identify models. Connect one target at a time when changing factory IDs.

## TTL Node (A)

PWM channels are 1/2, with 0–1000 representing 0–100% duty. Start at zero. RGB channel 0 controls both LED groups; 1/2 selects one. The UI follows the SDK's quantized color mapping: R=0–7 and G/B=0–3, not 8-bit color values.

Read S.BUS to obtain 16 raw channels and a status byte. Interpret the status as well as values when the receiver has no valid signal. ID changes check for a responding destination ID; scan again afterward.

## TTL Stepper Driver (A)

For position control: set mode 0, enable output deliberately, send position/speed/acceleration/current, then read feedback. For continuous rotation: set mode 1, enable output, send signed speed. Zero speed stops rotation; disabling output removes drive.

**Modes 0 and 1 can switch freely within one power session, without restarting. Only phase configuration changes require a restart.** This UI does not modify phase. A mode write disables output; re-enable when ready.

Parameters are register units, not direct amperes or degrees. Consult the [memory map](../ttl-stepper-driver-a/memory-map.md). Position is 0–65534 and its physical distance depends on microstepping and resolution.

For follower mode, keep a unique primary ID, choose mode 2, and set `leader_id` to the leader's primary ID. Follow the [EDS wiring instructions](../ttl-stepper-driver-a/operating-modes.md). Do not duplicate primary IDs.

Setting the current position changes the coordinate reference; it is not a move command.

## TTL Encoder E02

Read raw position and speed. Angle is `position / 4096 * 360 degrees`. Place the mechanism at its reference before calibration. Calibration is persistent, while the accumulated multi-turn count is not retained across power loss. Avoid repeated calibration writes in a high-rate loop.

## Servos

Choose SCS, HLS or STS/SMS correctly. Read first, then use a small low-speed move within the mechanism's limits. SCS and STS position ranges differ. JSON tools includes synchronized-command examples and mission scripts.
