# NODS LME 0.69 — experimental GAME entry-hook comparison

Base: v0.68 (XMB controls and GAME HOME confirmed by the user; ordinary Daxter input did not reach any of four syscall merge hooks). This test changes GAME interception, not Bluetooth pairing/report decoding or the working XMB path. Hardware results are pending. Keep v0.66/v0.68 to restore the previous behaviour.

## Change

In GAME only, the installer checks all eight instructions in each of Sony's four ordinary two-argument read/peek wrappers, their text bounds, non-overlap and shared internal callee. Only after every guard and a second under-interrupt-guard comparison passes does it redirect their first two instructions to NODS. Each merge wrapper calls a resident trampoline with the original stack adjustment/argument setup followed by the untouched original body. GAME does not use the previous syscall-table redirects. No CP0 syscall-table scanner or repeated patch attempts are added. No game executable or import stub is modified. XMB retains the guarded LME VshControl route from v0.68, and HOME retains its separate worker.

This should intercept callers that reach those standard exported entry points, whether through syscall or direct kernel imports; it does not cover all possible extended controller APIs. It deliberately refuses existing entry detours or unexpected firmware instructions rather than trying to chain unknown code. Installed hooks/trampolines remain resident until reboot.

## Install and test

Replace the enabled joesnose_nods_free_LME.prx with this package's version. Keep it enabled in both vsh.txt and game.txt, disable all other NODS copies, preserve registration/mappings/target data, and fully power off/restart. Keep the fullscreen plugin disabled for this comparison.

Check XMB directions and buttons first, then launch Daxter. Use DS4 buttons and both sticks for about 20 seconds; check HOME and cancellation back to the game. Report whether ordinary game controls now work. Then upload:

- ef0:/seplugins/NODS_LME_GAME.txt
- ef0:/seplugins/NODS_LME_VSH.txt

First line must contain NODS_LME_DIAG_069. GAME_ENTRY installed=1 and GAME_HOOK current matching expected with delay=0 verify the four jumps at each worker snapshot. ALL_READS and merged show which paths actually run. STATE error=8 means the new entry guard refused the layout; guard indexes 1..7 identify the planner's rejection group. Existing errors 5/6 identify module or instruction changes during installation. If a later snapshot shows a changed entry, this test reports it without overwriting another patch.

Logs are overwritten on each corresponding module session. Copy them before another reboot/game launch. If either file is missing, say which and whether the game booted. Missing logs do not prove the plugin was never loaded. This is not a pairing or fullscreen HOME-crash fix.

## Validation

PSP build passes with warnings treated as errors. Host ASan/UBSan suite passes, including XMB/legacy paths, all 32 instruction mutation refusals, complete-guard-before-write behaviour, pre-install mutation refusal, expected eight entry writes and retained trampolines. A small instruction interpreter executes all four trampolines through the original wrappers, checking once-only internal calls, argument/mode setup, stack/RA restoration and original success/error returns. Compiled HID/DS4/HOME checks pass; Bluetooth report callback budget remains 400 bytes with no direct installer. Only the worker performs diagnostics/file I/O. Exact LME firmware/hardware behaviour is pending.

Source ZIP, matching ELF, patch against original v0.64 and SHA256SUMS are included. Build with make -B release using PSPSDK. Do not load both this and another NODS binary.
