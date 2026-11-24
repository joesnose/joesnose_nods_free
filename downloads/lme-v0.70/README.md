# NODS LME v0.70 — clean build

Separate LME build for PSP Go, based on the v0.69 hardware-tested input fix. The user confirmed v0.69 works in XMB, Daxter and the in-game HOME menu on 6.60 LME. v0.70 removes diagnostic logging and version markers while retaining the verified input paths. This exact clean binary still needs device validation. The ordinary v0.64 ARK release is unchanged.

## Install

1. Copy seplugins/joesnose_nods_free_LME.prx from this ZIP to your existing plugin directory (normally ef0:/seplugins/).
2. Enable only this NODS version in both seplugins/vsh.txt and seplugins/game.txt. Standard LME entry, adjusting the device/path to match your installation:

   ef0:/seplugins/joesnose_nods_free_LME.prx 1

3. Disable ordinary NODS and other test copies. Keep pspgo_hid_target.dat and pspgo_hid_controls.txt; do not overwrite your mappings or delete pairing.
4. Fully shut down and boot the PSP; restarting VSH or entering sleep is not sufficient to replace this resident plugin. If switching from a resident test still causes problems, disable all NODS entries, cold boot once, then enable only the clean LME PRX and cold boot again.

The filename remains joesnose_nods_free_LME.prx, module version 0.70. Your existing correctly targeted LME entries need no filename change. No re-pairing is expected for an existing connection; this build does not separately fix or validate fresh registration on every LME revision/controller.

## What is retained

- The guarded LME VshControl call path for XMB input.
- GAME-only controller-entry hooks with exact four-wrapper instruction/bounds checks, original-call trampolines and no raw syscall-table scanner.
- DS4 input decoding, custom VSH/GAME mappings, dedicated native HOME worker, reconnect/persistence and existing controller support paths.

The compiler check confirms no diagnostic formatting or log paths remain. It does not create or update NODS_LME_VSH.txt / NODS_LME_GAME.txt; any existing files are old. The background worker remains because it handles reconnect, state persistence and input keepalive. Pairing/target data can still be read/written as required for normal operation.

## Device checks next

Keep the fullscreen plugin disabled while validating this clean build. Check a second game, analogue input, HOME then cancel, HOME then Quit Game, controller disconnect/reconnect and PSP sleep/wake. After those pass, test fullscreen separately so a remaining LME HOME/display issue can be isolated. This ZIP cannot perform those hardware checks for you.

If a problem returns, restore the diagnostic v0.69 PRX from tests/lme-v0.69 and cold boot before collecting fresh logs. Do not interpret old diagnostic files as v0.70 output.

## Validation and source

PSP build passed with warnings treated as errors. Full host ASan/UBSan suite passed, including LME/ARK XMB cases, GAME entry guards and no-write-on-failure checks, merge/mapping/HOME policies, and an interpreter checking all four original-wrapper trampolines for stack/RA/argument/result preservation. Compiled logging-removal, HID callback stack, DS4 and HOME checks passed. The report callback conservative known budget remains 400 bytes in its 1024-byte thread. Exact clean-build hardware results are pending.

All input/HOME policy, guard and trampoline source files match v0.69 byte-for-byte. Only main.c changes: diagnostic worker/formatting/record copies removed and version updated. Source ZIP, matching ELF, patch against the original v0.64 tree and SHA256SUMS are provided beside the installation ZIP. Build with make -B release using PSPSDK or pspdev/pspdev:latest. Historical tests and the ARK release are preserved.
