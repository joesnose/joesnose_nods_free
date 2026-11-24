# NODS LME v0.71 — screen keep-awake

Separate 6.60 LME build for PSP Go, based on the clean v0.70 source and its v0.69 hardware-tested input routes. The input guards, GAME trampolines, VSH route, controller mappings, reconnect and HOME handling are retained.

## Install

1. Replace `ef0:/seplugins/joesnose_nods_free_LME.prx` with this build, adjusting the device/path if your installation differs.
2. Enable only the LME variant in both `seplugins/vsh.txt` and `seplugins/game.txt`:

   ```text
   ef0:/seplugins/joesnose_nods_free_LME.prx 1
   ```

3. Keep `pspgo_hid_controls.txt` and `pspgo_hid_target.dat`. Do not delete pairing or overwrite mappings.
4. Fully shut down and restart the PSP. A VSH restart or sleep/resume does not replace the resident plugin.

Use the standard ARK build instead on ARK; do not load both copies.

## Keep-awake behaviour

The existing worker refreshes display and automatic-suspend timers every 500 ms while a controller input session is live, including neutral reports and trusted session keepalive. Clearing/disconnecting input or allowing it to become stale stops the refresh. The firmware timeout settings and manual power switch are not locked or changed.

The change adds a live-session query and a worker power tick using the normal kernel power API. No power work runs on the HID report stack or with input interrupts disabled. This clean build retains diagnostic logging removal; pairing and target persistence still use their existing state files.

## Build and validation

Run `make -B release` in this directory with PSPSDK, or build the repository root for the standard ARK variant. Host and compiled-code checks are recorded in the repository-root `validation/` directory. The LME hook/guard/trampoline sources match the clean v0.70 base byte-for-byte. Bluetooth screen-timeout behaviour and manual sleep/wake still require PSP Go confirmation.
