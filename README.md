# joesnose_nods_free

[Downloads](#downloads) · [Installation](#installation) · [Controller Configuration](#controller-configuration) · [Building](#building-from-source)

## Overview

A Bluetooth HID controller plugin for PSP Go. Built with AI.

Connects to PSPGo, mappable controls through text file stored in plugins folder.

It make come with quirks and absolutely no guarentee. sometimes connection attempts fail, DualSense and DS4 sometimes have a few seconds lag at startup but soon catch up........happy gaming.

## Controller Support

Controllers tested in this project include DualShock 4, DualSense and 8BitDo NES30 Pro. Other older Bluetooth Classic controllers may work and need further testing. Bluetooth Low Energy controllers are outside this implementation.

## Requirements

- A PSPGO, standard build targets ARK; a separate 6.60 LME build is included.
- nods free plugin
- A compatible controller. I know it works with Dualshock4, Dualsense, 8bitdo nes30 pro, Steel series zeemote and the controller for the original Gamestick. It may work with other but these all required individual per controller tweaking.

## Downloads

[ARK v0.65 ZIP](https://github.com/joesnose/joesnose_nods_free/raw/refs/heads/main/downloads/v0.65/joesnose_nods_free-v0.65.zip) · [LME v0.71 ZIP](https://github.com/joesnose/joesnose_nods_free/raw/refs/heads/main/downloads/v0.65/joesnose_nods_free_LME-v0.71.zip) · [Both sources](https://github.com/joesnose/joesnose_nods_free/raw/refs/heads/main/downloads/v0.65/joesnose_nods_free-v0.65-source.zip) · [Release notes](https://github.com/joesnose/joesnose_nods_free/releases/tag/v0.65) · [Checksums](downloads/v0.65/SHA256SUMS.txt)

Choose the ZIP matching your firmware. ARK uses `joesnose_nods_free.prx`; 6.60 LME uses `joesnose_nods_free_LME.prx`. **Enable only one NODS variant.** Replace that PRX, preserve your controls/target files and fully shut down/restart the PSP. The separate LME source and instructions are in [variants/lme](variants/lme/README.md). Historical v0.64 downloads remain in `downloads/v0.64`.

## Keep-awake update

The resident worker refreshes display and automatic-suspend timers every 500 ms while a live controller session exists, including neutral input. Disconnects, cleared input and stale sessions stop the refresh. It does not lock the power switch or change the configured timeout values. Pairing, mappings, reconnect and HOME routing are retained; the LME input guards match its clean v0.70 base.

Power calls run in the worker, outside the HID callback and outside the input snapshot's interrupt-disabled section. Both PRXs were built with compiler warnings treated as errors. Host and compiled-code checks passed; physical PSP Go screen timeout and sleep/wake checks remain necessary. See [validation/result.json](validation/result.json).

## Installation

1. Copy `seplugins/joesnose_nods_free.prx` from the installation ZIP to `ef0:/seplugins/`.
2. In your existing ARK plugin list, replace the old `pspgo_hci_transport.prx` filename in both VSH and GAME entries:

   ```text
   vsh, ef0:/seplugins/joesnose_nods_free.prx, on
   game, ef0:/seplugins/joesnose_nods_free.prx, on
   ```

3. Disable or remove the old plugin entries so only this PRX loads.
4. Fully power off and restart the PSP.

The same entries are provided in [ARK-plugin-entries.txt](ARK-plugin-entries.txt). No EBOOT or re-pairing is needed for this update.

## Controller Configuration

Keep these existing files and filenames:

- `ef0:/seplugins/pspgo_hid_controls.txt` — custom DS4 and DualSense controls, with separate `[vsh]` and `[game]` sections.
- `ef0:/seplugins/pspgo_hid_target.dat` — saved controller target.

The included [controls file](pspgo_hid_controls.txt) is a template. Preserve your customized file instead of overwriting it. Fully restart the PSP after editing control bindings.

## Changes in v0.64

- Renamed the PRX and internal module to `joesnose_nods_free`.
- Removed file logging and its formatter. No log file is created or updated; an old log can be deleted.
- Retained the controller worker for keepalive, reconnect, target persistence and Sony link setup.
- Retained v0.63 controller mappings, the NES30 regression fix, Sony recovery isolation, pairing, configurable controls and reboot reader quiescing.

The controller implementation files other than `main.c` match v0.63. The unused file-log helper was removed.

## Building from Source

Install PSPSDK and the ARK systemctrl development library, then run this command from the repository root:

```sh
make -B release
```

The output is `joesnose_nods_free.prx`.

## Testing and Validation

Run the host tests from the repository root:

```sh
sh tests/run.sh
```

Compiled validation scripts in `tools/` accept `joesnose_nods_free.elf` as their argument. For example:

```sh
python tools/check_diag_compile.py joesnose_nods_free.elf
```

The forced PSP SDK build passes with strict warnings. All 34 remaining ASan/UBSan host programs and 16 compiled checks pass. The obsolete file-logger test was removed with its helper. The logging check verifies that the formatter and log paths are absent, the logging sink is inactive, and the only compiled file write is saved-target persistence.

Source review covered native patch guards and bounds, Sony/generic isolation, shutdown quiescing, callback stack constraints, control configuration and worker lifetime. No additional defect was identified. v0.64 is based on the user-confirmed working v0.63 build; the renamed logging-free binary has not independently been tested on hardware.

## Repository Structure

| Location | Contents |
| --- | --- |
| `src/` | Plugin source code and controller handling |
| `tests/` | Host tests and controller fixtures |
| `tools/` | Compiled-code and stack validation scripts |
| `docs/` | Historical research, implementation notes and validation records |
| `notices/` | Third-party notices |
| `downloads/v0.64/` | Exact release files and checksums |
| `Makefile` | PSP build configuration |
| `ARK-plugin-entries.txt` | Installation entry reference |
| `pspgo_hid_controls.txt` | Controller configuration template |

## License

The project uses the [MIT License](LICENSE). Third-party notices are in [notices/](notices/).

No Sony firmware, private logs, keys or device identities are included.
