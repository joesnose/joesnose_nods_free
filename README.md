# joesnose_nods_free

## Overview

A Bluetooth Classic HID controller plugin for PSP Go with ARK. The current v0.64 release removes file logging and uses the filename `joesnose_nods_free.prx`.

## Controller Support

Controllers tested in this project include DualShock 4, DualSense and 8BitDo NES30 Pro. Other older Bluetooth Classic controllers may work and need further testing. Bluetooth Low Energy controllers are outside this implementation.

## Requirements

- A PSP Go running ARK, with 6.61 as the user's tested firmware. Existing runtime guards also recognize the supported 6.60 environment.
- Access to the PSP's internal storage to install the plugin and update its ARK entries.

## Downloads

[Download v0.64](https://github.com/joesnose/joesnose_nods_free/releases/tag/v0.64) · [PRX](https://raw.githubusercontent.com/joesnose/joesnose_nods_free/f31b3a6c6b1cfc6c66c07edb157bc76eafd50da9/downloads/v0.64/joesnose_nods_free.prx) · [Installation ZIP](https://raw.githubusercontent.com/joesnose/joesnose_nods_free/f31b3a6c6b1cfc6c66c07edb157bc76eafd50da9/downloads/v0.64/joesnose_nods_free-v0.64.zip) · [Source ZIP](https://raw.githubusercontent.com/joesnose/joesnose_nods_free/f31b3a6c6b1cfc6c66c07edb157bc76eafd50da9/downloads/v0.64/joesnose_nods_free-v0.64-source.zip)

Exact release files and checksums are also available in [downloads/v0.64](downloads/v0.64). The buildable project is at the repository root.

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
