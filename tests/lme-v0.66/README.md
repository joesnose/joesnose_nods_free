# NODS LME diagnostic 0.66

This is a diagnostic comparison against the unsuccessful 0.65 LME input test. It retains that test's guarded XMB hook changes and adds worker-only snapshots. It is not a confirmed input, pairing or HOME fix. The v0.64 ARK release remains unchanged.

## Install and test

Replace joesnose_nods_free_LME.prx with this ZIP's version. Enable it in both LME vsh.txt and game.txt, using your actual plugin path. Disable ordinary v0.64 NODS and all other copies. Disable the fullscreen plugin for this comparison. Preserve pairing and pspgo_hid_target.dat; do not delete registration again. Fully power off and restart.

Connect the DS4. Spend about 15 seconds in XMB pressing directions and Cross/Circle. Launch a game with the PSP controls if necessary, wait 15 seconds and try DS4 buttons, sticks and HOME. Then copy BOTH diagnostic files before another restart/game launch overwrites them:

- ef0:/seplugins/NODS_LME_VSH.txt
- ef0:/seplugins/NODS_LME_GAME.txt

If a file is missing, report which one and send the relevant plugin-list entry. Missing logs can indicate loading/worker/file-path failure and do not alone prove the PRX was never loaded. Each file starts with NODS_LME_DIAG_066. The VSH file is reset when this module starts in VSH; GAME is reset on each new game session. Existing older logs are not modified.

## What it records

The existing 8 KiB worker writes snapshots once per second: firmware/context, module base/size and selected code words, input installation error/guard index, controller-read/merge counters, received HID report counts, decoded button/motion state, and HOME setup state. No PINs, link keys, Bluetooth addresses or raw device records are written. Report/registration callback log_line remains a no-op; no file I/O or new formatting is added to the Bluetooth report callback.

HOME state: 0 waiting/uninitialized (or not applicable in VSH), 1 missing export, 2 native toggle ABI mismatch, 3 missing status export, 4 status ABI mismatch, 5 ready, 6 module changed, 7 a native HOME toggle was dispatched. HOME_WORD records the guarded instruction words already examined by the HOME worker.

Input error meanings from the existing installer: 1 firmware, 2 missing module, 3 export bounds/alias, 4 VSH ABI, 5 module changed, 6 context changed, 7 unsupported application context. Together with module/code snapshots, this should identify why the safe hook refuses installation rather than bypassing its checks.

## Validation

Host tests/run.sh passes under ASan/UBSan. PSP strict-warning build passes. Compiled HID/DS4/HOME checks pass; the report callback conservative budget remains 400 bytes in a 1024-byte thread, with no direct installer call. Diagnostic formatting and filesystem writes run only from the existing worker. Exact LME hardware results are pending. The release-only check_diag_compile.py intentionally does not apply: this variant has diagnostics enabled.

Complete buildable source, matching ELF, changes against v0.64 and checksums are included separately. Build using make -B release with PSPSDK. Do not publish this experimental build as a working LME release until hardware results identify and verify the failure path.
