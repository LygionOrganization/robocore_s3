# Develop with an AI assistant

Download the [AI development guide archive](../../../assets/files/robocore-s3/ai-guide.zip), extract it, and place `AGENTS.md` in your project root. The complete firmware source also includes this file at its root. It describes hardware limits, APIs, dependencies and verification. Lygion resources are maintained under [LygionOrganization](https://github.com/LygionOrganization/).

Tell the assistant your board, device models, IDs, baud rate, supply, wiring, desired behavior and stop conditions. Say whether you are controlling factory firmware from Python or replacing it with Arduino code.

Example prompt:

> Read AGENTS.md and the actual repository APIs first. I use RoboCore S3 over USB CDC with an E02 encoder at ID 1. Implement read-only position/speed polling in Python with timeout/disconnection handling. Do not change calibration.

Start with reads before adding motion. Verify functions and parameters against the included SDK, and keep GPIO assignments consistent with your wiring. When modifying the complete firmware, preserve sensor error handling, communication timeouts and protection against high-rate input; see `AGENTS.md`. Use `TTLSDClass::SetMode()` for stepper modes; modes 0 and 1 can be switched without restarting.

Ask the assistant to list checks it actually ran. Compilation and simulations do not prove hardware behavior. Run `pio run`, the USB host tests and example compilation checks as appropriate. Save working code with your wiring and parameter notes for future use.

Confirm actual wiring and target hardware before executing motion, flashing or erasing commands.
