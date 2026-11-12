# ARK-5 VSH import compatibility, 2026-11-10

The v0.19 hardware log demonstrates authorized valid reports and a refused VSH
ABI guard (installed=0,error=4,guard=4); it does not capture which guard operand
mismatched. ARK-5 source provides a concrete mismatch omitted by the v0.19
fixture: VSHControl/src/vshpatch.c patch_sceCtrlReadBufferPositive calls
sctrlHookImportByNID on sceVshBridge_Driver/sceCtrl_driver/BE30CED0 with
_sceCtrlReadBufferPositive. Its module name is VshCtrl. The handler accepts
(SceCtrlData*,int), calls the current Ctrl export, then applies VSH menu logic.
SystemControl/src/hooknids.c writes a direct J/NOP for kernel imports.

v0.20 accepts the stock Ctrl target or an aligned direct J/NOP target inside
resident VshCtrl kernel text. It changes only the guarded VSH caller's JAL,
keeps its original import stub and invokes that stub exactly once before
merging input. All 34 caller instructions, export offset, Ctrl alias, module
bounds, same-region jump, IRQ-time context/stub and VshCtrl module text bounds
remain checked. Unknown targets and non-NOP delay slots fail before any write.
No added native HID callback work, Ctrl entry patch or ARK import patch.

Tests include the actual ARK topology (existing import jumps into VshCtrl,
not directly to Ctrl), preserved stub bytes, no-VshCtrl rejection and malformed
stub rejection. Host-native calls use a mock; no physical PSP control success
is claimed. The 0.18 stack correction is retained.

Primary source: https://github.com/PSP-Arkfive/ARK-5/tree/main/VSHControl
