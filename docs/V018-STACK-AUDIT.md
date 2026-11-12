# v0.18 crash audit: report-thread stack exhaustion

Original offline analysis, 2026-11-10. No hardware actions or guard weakening.

## Concrete defect

The new controller installer runs synchronously from the HID report callback on a native thread with a 1024-byte stack. Its compiled stack frame grew from 56 bytes in v0.17 to 400 bytes in v0.18. The live call chain exceeds the entire native stack before resolving exports or reaching any controller-entry write.

| Live function | Evidence | Stack frame |
| --- | --- | --- |
| Native HID report thread | Captured sceHIDSvc offset+0x680, address0x8822FE80 | 48 bytes |
| Native report dispatcher | Captured sceHIDSvc offset+0xE2C, address0x8823062C | 80 bytes |
| Plugin receive_report | Compiled v0.17 and v0.18 ELF symbols | 640 bytes |
| Plugin zee_input_install v0.17 | Compiled v0.17 ELF symbol | 56 bytes |
| Plugin zee_input_install v0.18 | Compiled v0.18 ELF symbol | 400 bytes |

The native parent creates the report thread at sceHIDSvc+0x55C with stack size0x400 and entry+0x680. The dispatcher invokes the callback without releasing its frame, on both numbered and unnumbered report branches. These facts are confirmed in the retained captured instruction listing.

The v0.18 lower bound is48+80+640+400=1168 bytes, already144 bytes beyond the whole1024-byte allocation. Additional thread startup and resolver/native-call frames are not included. v0.17's corresponding lower bound is824 bytes; that is not a generous stack margin, even though prior hardware runs survived.

Therefore an installation guard returning failure does not make this path memory-safe: the installer allocated its frame before evaluating the guard. Repeating failed installation on every decoded report repeatedly executes the oversized call chain.

## Separate reason for guard rejection

The examined ARK-5 source, commit62f9936d65560c44973d2cec83d3e707d3dbe00f, SystemControl/src/controller.c, already hijacks all four ordinary controller entry functions in patchController(). A stock-wrapper guard can legitimately reject those entries. This is separate from firmware-version differences and must not be fixed by simply removing the guard or replacing an unknown existing hook.

The supplied first v0.18 log section explicitly reports controller installed0/error4/guard0. That session demonstrates rejection before controller-entry writes, while the oversized report-thread call remains present. Later log sections do not establish whether controller writes ran before the crash; there is no successful-install record in those sections.

## Required architectural correction

Remove installation, export lookup, planning, and controller-patch operations from the HID report callback. Perform any verified installation from an owned worker with an adequate explicit stack, or before enabling the report callbacks. A report callback should consume only already-prepared state and must not retry installation after every report.

Separately reduce its640-byte layout-copy frame, using a bounded lifetime-safe publication scheme. Do not substitute an unprotected pointer to mutable descriptor storage.

ARK coexistence needs its own validated integration or chaining design. This audit does not authorize modifying its existing detours and does not claim a working replacement input path.

Desktop ASan tests did not model the PSP thread's1024-byte allocation. Future verification must include compiled PSP frame accounting for the full live native/plugin call chain, not only host memory-safety tests.
