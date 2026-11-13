# Plugin Source Code

The PSP plugin implementation is stored here.

- `main.c`: startup, guarded native hooks, pairing, saved targets and transition recovery.
- `zeemote_hooks.c`: HID descriptor handling, report dispatch and owned input contexts.
- `zeemote_hid.c`: generic controller descriptor parsing and report decoding.
- `zeemote_input.c`: PSP button and analogue input integration.
- `ds4_basic.c`: supported Sony basic-report decoding.
- `sony_controls.c`: configurable DS4 and DualSense controls.
- `completion_bridge.S`: native callback argument bridges.
- Header files: controller policies, firmware guards and helper interfaces.

Build from the repository root using the [Makefile](../Makefile). See [Building from Source](../README.md#building-from-source) for requirements and instructions.
