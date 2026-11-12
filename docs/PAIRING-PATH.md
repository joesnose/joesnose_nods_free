# Firmware review and the cable-pairing trial

Analysis used the user's 6.61 loaded-code snapshots and the locally extracted official PSP Go 6.61 update. No firmware binaries, decompiler output or user records are included here.

## Findings

1. `usbps3controller.prx` is the USB peripheral used by the PS3-assisted menu flow. Its feature-report handlers expose DS3-style address reports. GET F2 returns the Go's Bluetooth address in the controller-address position. SET F5 accepts a six-byte peer address and signals the waiting menu caller. This explains why DS3-oriented PC utilities can operate on the Go as well as on the controller.
2. In the Bluetooth menu, the returned peer address is passed through a six-byte address adapter to native sceBSMan no-PIN registration, NID 2D649923. This is the function that returned slot 2 in the existing test. The PC/PS3 procedure is therefore not evidence of a separate missing PSP registration API in the plugin.
3. The native HID Connect export selects flags 0202 for mode zero, submits the request to the Bluetooth module through the USB BSM command transport and waits for a service-link event. The v0.7 hardware trace recorded state 2 with flags 0202, not state 0. The native consumer sets the failure event bit and Connect returns 802F0140. No exact HCI/authentication reason was captured.
4. The HID service contains explicit Sony DS3 VID/PID handling as well as a generic HID descriptor path. Those checks are in device initialization/report handling; patching them is not established to fix the already-observed service-link failure. They must not simply be relabelled as the proven cause.
5. The command transport sends the request into the Bluetooth module. The host PRXs inspected do not implement the full Bluetooth authentication exchange locally. A registration record and HID profile bit are not proof of a shared link key.

## Trial implemented

The Go already knows the DS4's address. The untested cable-side step is to make the DS4 know the Go as its host, then try PS-button reconnection rather than another Go-initiated request.

The Windows helper implements the zero-key report-13 method used by BlueZ's sixaxis plugin: the Go host address in DS4 little-endian order, followed by 16 zero bytes. It reads the saved host from report 12 afterward. It does not copy or install a matching authentication key into the Go, so connection can still fail. This is an explicitly labelled experiment, not proof of a workaround or support for games.

The PRX's new host query uses sceBSMan NID 96425848. It is guarded against the expected relocated read-query context and exports only the local Bluetooth address to a separate text file. No arbitrary BCCMD writes or controller-identity spoofing are attempted.

## Sources checked

- The supplied MotioninJoy PSPGo page's Pair button calls BTPair for both endpoints. Its UI code establishes the two calls, but not the driver's exact packet or authentication semantics.
- https://github.com/bluez/bluez/blob/master/plugins/sixaxis.c : DS4 report 12 parsing and zero-key report 13 cable pairing.
- https://github.com/dualshock-tools/ds4-tools/blob/master/ds4-tool.py : separate own-address and host-address/link-key reports.

## Hardware evidence needed

A verified USB host-address read-back followed by an actual Bluetooth connection/input result. Opening Manage Bluetooth Devices flushes v0.8's buffered known HID service-link fields. `during_request=0` means outside the traced native Connect call; it does not independently prove an incoming connection or its authentication outcome. Failure after a verified target write would leave link-key/lower-stack investigation necessary.
