# Official 6.61 VSH controller read ABI

Original static audit, 2026-11-10. Official firmware/decompiler material remains private.

The official vshbridge sceVshBridge export C6395C03 (vshCtrlReadBufferPositive) resolves to module offset +0200. It accepts a buffer and count, validates the caller buffer extent using count multiplied by 0x10, then forwards those two arguments through import stub +5608.

That stub imports sceCtrl_driver NID BE30CED0. ARK's published 6.60 NID mapping associates it with sceCtrlReadBufferPositive, including original NID 1F803938. The captured controller module's manifest resolves BE30CED0 to controller offset +1B20, the ordinary read-positive entry.

Therefore entry detours on the four ordinary controller read/peek functions cover the examined official VSH bridge read path. Its sample ABI is 16 bytes. Patching only the public syscall table does not establish coverage of a kernel driver's resolved direct import.

The manifest's controller driver NID 55497589 at +1A50 is mapped by ARK as sceCtrlSuspend; it must not be labelled an extended read routine.

The separate Read/Peek Buffer Positive2/Negative2 family at +1B60/+1B8C/+1BB8/+1BE4 uses three arguments: selector, buffer and count. Those wrappers shuffle them into common helper +1E70 as buffer, count, selector and operation values 4..7. The ordinary family supplies selector zero and operations 0..3. Consequently, a two-argument wrapper cannot safely be applied to that separate family. No larger sample stride is inferred merely from the suffix 2.

## Independent input layout comparison

[MissionControl's SteelSeries declarations](https://github.com/ndeadly/MissionControl/blob/master/mc_mitm/source/controllers/steelseries_controller.hpp) identify SteelSeries Free as 1038:1412 and describe report 1 with a direction byte, signed 8-bit stick axes and two button bytes. Its button positions agree with the actual 81-byte descriptor and existing descriptor-derived adapter. No report-offset correction was established by this comparison. GPL implementation code is not copied into the adapter.

[AOSP's device layout](https://android.googlesource.com/platform/frameworks/base/+/8fdaa6ea70b28a964b2471d904e94d0d2fb7233a/data/keyboards/Vendor_1038_Product_1412.kl) independently identifies the button/axis semantics used by the adapter. Neither this static comparison nor successful report decoding proves that physical button state was non-neutral in the supplied log. Aggregate seen-button masks and axis extrema can distinguish incoming activity from neutral reports without logging complete reports.

Published NID mapping source: [ARK-5 nid_660_data.c](https://github.com/PSP-Arkfive/ARK-5/blob/main/SystemControl/src/nid_660_data.c).
