# NODS 0.65 LME input test (base 0.64)

Experimental PSP Go / DS4 / 6.60 LME build, separate from the working v0.64 ARK release. It targets an identified XMB input-hook layout mismatch, not an established Bluetooth transport fault.

## Why this change

The inspected LME VshControl source redirects sceVshBridge +0x25C directly to its own controller-read handler. NODS v0.64 accepts the stock call site and ARK import-stub detours, so it rejects that LME layout. HOME uses a separate worker/native toggle and can still work while ordinary input installation fails. This matches the symptom in XMB, but does not prove why buttons might fail inside games.

The test accepts the direct CALL only when its target belongs to the named resident VshControl kernel-text module, all other bridge instructions match, the original import stub is valid, and the module/text snapshot stays unchanged up to installation. It saves LME's current callee as the original handler and calls it before merging remote input, preserving LME's existing controller/menu chain. With LME's VSH menu disabled, the stock-call path remains accepted. ARK's existing path, GAME syscall input path, pairing, HID reports, control mapping and native HOME handling are unchanged.

The PRX/module are named joesnose_nods_free_LME, version 0.65 for this test. No retail firmware or game binaries are included. Exact installed LME revision/hardware verification is pending.

## Install

1. Copy seplugins/joesnose_nods_free_LME.prx to ef0:/seplugins/ (or your existing plugin directory).
2. Disable the ordinary NODS entry in both VSH and GAME lists. Enable ONLY this version. For standard LME plugin lists, add the following line to BOTH seplugins/vsh.txt and seplugins/game.txt, preserving the actual device/directory you use:

   ef0:/seplugins/joesnose_nods_free_LME.prx 1

3. Keep your existing pspgo_hid_target.dat and pspgo_hid_controls.txt. Do not overwrite custom mappings. Fully power off and restart.
4. Connect the same DS4. First check XMB directions/Cross/Circle and the analogue stick; then launch a game and test its controls and HOME. Also check whether LME's SELECT/VSH menu still opens normally. Report which contexts work. If only games fail, this XMB-specific change will need a separate GAME investigation.

If the plugin fails, disable this test entry using the PSP's physical controls and restore the v0.64 entry. No re-pairing should be required. This build retains v0.64's disabled logging; existing old logs are not new evidence.

## Validation

Host tests/run.sh passed under ASan/UBSan. Tests cover the LME direct call, LME stock fallback, exact original-callee retention, malformed context/module bounds/stub rejection and pre-install mutation, alongside existing ARK and GAME paths, reports, mappings and HOME tests. PSP build passes with warnings as errors. Compiled HID report path stack checks pass (400-byte conservative known budget within the 1024-byte report thread); no installer is called from the report callback. Compiled DS4 and HOME checks also pass. These do not establish hardware compatibility.

Source ZIP includes the buildable project, tests and validation tools. Run make -B release with PSPSDK or pspdev/pspdev:latest. Matching ELF, patch against v0.64 and SHA256SUMS are provided beside the installation/source ZIPs. No current release files are replaced.
