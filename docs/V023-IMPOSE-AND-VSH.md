# Impose ABI and VSH reconnect audit, 2026-11-10

User confirms v0.22 game controls work. HOME/quit selection does not, and return
to VSH requires a controller power cycle.

Read-only official6.61 impose_05g ELF import-table analysis identifies module
sceImpose_Driver and sceCtrl_driver/2BA616AF import. ARK-5's published660 mapping
identifies this as ordinary sceCtrlPeekBufferPositive. Examined callsites pass
a stack sample pointer as a0 and count1 as a1; callers read Buttons at sample+4.
No three-argument Buffer2 function is involved. This supports hooking that
specific import while retaining the ordinary16-byte sample ABI. Runtime import
location is resolved by NID, not a hardcoded official offset.

The installed runtime stub must be an aligned, in-module J to the previously
validated current Ctrl peek export with NOP delay slot. All checks occur before
one J write, with IRQ-time code recheck and cache maintenance. The original
export remains untouched and is called by the existing peek wrapper. A hook
installed by another plugin into the import is refused rather than replaced.
The hook is GAME-only, bounded and installed from the owned worker after valid
input. Existing public syscall and VSH bridge routes remain.

The v0.22 native reconnect tick explicitly excluded VSH. The same single saved
registration query/connection operation now runs in VSH as well as GAME. No
extra command format or pairing step is introduced. Each boot gets one attempt
after2seconds, only when HID hooks exist and validated input is absent. Reboot
between game and VSH is still real; seamless radio continuity is not claimed.

Thirteen sanitizer programs cover policy and import/merge behavior. Physical
HOME navigation and automatic return reconnection are unverified for v0.23.
Original factual findings only; no firmware bytes/disassembly/private logs
are packaged. Primary mapping source:
https://github.com/PSP-Arkfive/ARK-5/blob/main/SystemControl/src/nid_660_data.c
