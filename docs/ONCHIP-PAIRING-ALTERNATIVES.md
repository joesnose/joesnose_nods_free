# Existing-stack DS4 pairing alternatives

Date: 2026-11-09. Source review only; no radio commands or publication.

## Concrete alternative worth implementing

The existing Sony PIN-registration path deserves an exact ABI audit and a
targeted DS4 legacy-PIN implementation before requiring standard HCI exposure.
This is supported by actual DS4 driver code and first-hand traces, rather than
only generic Bluetooth backwards-compatibility claims.

[USB Host Shield 2.0 PS4BT.h](https://github.com/felis/USB_Host_Shield_2.0/blob/master/PS4BT.h)
defaults its PIN to the four-character string `0000`, passing it to BTHID.
[BTD.cpp](https://github.com/felis/USB_Host_Shield_2.0/blob/master/BTD.cpp)
answers a PIN request with the configured PIN and negatively answers an unknown
link-key request. Its authentication-complete handler recognizes successful
pairing. The
[PS4BT example](https://github.com/felis/USB_Host_Shield_2.0/blob/master/examples/Bluetooth/PS4BT/PS4BT.ino)
initiates pairing with a discoverable controller.

The project's [issue 114](https://github.com/felis/USB_Host_Shield_2.0/issues/114)
contains a 2014 first-hand trace that reaches successful HID-device pairing
after responding with that PIN, identifying VID 054C/PID 05C4. Its subsequent
failure was later in HID handling. This demonstrates legacy PIN behavior for
at least one DS4 revision, not every genuine controller or clone. A later
maintainer comment explicitly warns that controller revisions may differ.

The examined Sony no-PIN registration path previously used in this project is
not equivalent to attempting this PIN exchange. Native PIN registration is
already present: loaded export NID 8AC2F7B8 resolves to 0x8823DC74 in the
captured image and calls the worker at 0x8823F958. The worker copies 32 bytes
from its second argument into a native request, then performs registration.
Consequently an implementation must determine the exact native string/record
ABI; passing a four-byte ASCII string would cause an out-of-bounds read.
The worker alone does not prove UTF-16 encoding. Its menu callers should settle
encoding, terminator, record initialization, and lifecycle before implementing
the call.

## Why the USB helper did not create a complete bond

[BlueZ's original sixaxis plugin](https://github.com/bluez/bluez/blob/master/plugins/sixaxis.c)
uses DS4 feature report 0x13 containing the six-byte host address followed by
sixteen key bytes. Its implementation writes zeros for the key and explicitly
notes the missing host-key loading mechanism. The controller must subsequently
complete actual Bluetooth pairing for this arrangement to create a usable
authenticated bond. Matching the saved host address alone proves neither that
process nor possession of a matching host-side key.

The original author of
[ds4-tools](https://github.com/dualshock-tools/ds4-tools/blob/master/ds4-tool.py)
implements `set_bt_link_info` as the host address and a caller-supplied 16-byte
key in report 0x13. Its existence is evidence of the controller-side primitive,
not a PSP host-side import primitive.

A recent [BlueZ issue 2678](https://github.com/bluez/bluez/issues/2678) supplies
first-hand experiments on a MOVONE clone: a real random key written through
0x13, followed by report 0x14/02, and the same key loaded into BlueZ produce
authentication/encryption. The issue is open and its clone-specific commit
requirement is not established for the user's unknown DS4 revision. It cannot
justify claiming a PSP fix by changing the USB helper alone.

## Required primitive for full cable bonding

A complete USB-provisioned bond requires the SAME controller-specific key in
both endpoints, plus a supported method for the Sony on-chip stack to associate
that key with the controller address and answer authentication requests. The
review did not locate a verified native PSP key-import API or its record ABI.
A raw HCI Write Stored Link Key command is not usable without an HCI command
route; storing bytes in an arbitrary PS key is not equivalent to updating the
on-chip stack's security database.

DS3 host-address-only provisioning must not be assumed sufficient for DS4.
BlueZ separately special-cases DS3 device records and cable authorization; its
DS4 implementation and USB report format make the distinction explicit.

## Recommended implementation direction

First audit and implement the already-existing native PIN registration with a
properly sized, correctly encoded `0000` argument and the genuine discovery
record. Preserve the native authentication result. Identify the target by its
actual discovered controller identity, not by device-name matching alone.
Success must subsequently mean an authenticated connection and valid HID input,
not just a registration slot. If that route fails on this revision, the missing
host-side key import/response primitive is the next concrete target for cable
bonding. Neither path requires more PS_Size diagnostics.
