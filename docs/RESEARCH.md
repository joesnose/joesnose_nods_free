# v0.6 finding and scope

The v0.5 hardware trace recorded successful native no-PIN registration returning a positive slot ID, followed by a successful native profile query for that ID with HID capability set. This establishes a native registration record, not a confirmed active connection or usable input.

In Sony's official PSP Go 6.61 Bluetooth menu, generic HID objects identify as device type 2. The initial-registration action builder allows its HID connection helper only for type 1 after the non-audio/non-modem checks. It returns type 2 without adding an action. The outer registration flow treats an empty action list as unsupported, explaining a popup even after the successful native result. Separately, the existing-device action builder explicitly sends both type 1 and type 2 to the same HID helper.

The HID helper invokes sceBSMan_F188A405 for connection requests and sceBSMan_68327A27 for disconnection. Its native connection wrapper and argument forwarding were checked against the user's private driver capture. v0.6 traces the existing connection request without issuing additional requests itself.

The correction extends the initial type-1 decision to type 2 only if the actual profile record contains HID bit 0x2. Other types and type 2 without HID retain the reject/return path; native type 1 retains its old accepted path. It reuses the stock allocator, helper table, connection code and return flow. No native authentication/result/profile-capability value is modified. A valid record alone must not be described as a working DS4 connection.

The patch site and its context were checked in Sony's 6.61 menu. Runtime guards validate the relevant register loads, comparison, delay slot, allocator target and relocated helper-table address. A 56-byte user-mode stub performs the decision, changing only temporary v0, retaining s1/s2/ra and the allocator argument set by the original delay slot. Its return jumps rejoin the original accept/return paths. The temporary allocations are bounded to four unique UI module addresses and retained until full restart to avoid freeing referenced code.

Offline tests interpret every emitted routing instruction across 120 type/flag combinations and verify preserved state, targets and rejects. Kernel hook guards and relocation tests also pass. This does not establish cache/loader behaviour on hardware, pairing completeness or DS4 report compatibility. The next hardware result must establish menu behaviour, connection status and real inputs.

Analysis sources/tools: Sony's official PSP Go 6.61 updater (MD5 fd0f7d0798b4f6e6d32ef95836740527), John-K/pspdecrypt, Ghidra 11.4.2 with kotcrab/ghidra-allegrex, PSP objdump, and the privately supplied driver snapshots. Official/private firmware and logs are not included in this release. New plugin code implements an interoperability repair and does not redistribute Sony helper code.

- https://github.com/PSP-Arkfive/ARK-5
- https://github.com/pspdev/pspsdk
- https://github.com/John-K/pspdecrypt
- https://github.com/kotcrab/ghidra-allegrex
- https://github.com/PSP-Archive/btfree
- https://github.com/Hirador/btfree-ark
