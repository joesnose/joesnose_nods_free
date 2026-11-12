# Zeemote v0.16 hardware result and v0.17 correction

Original analysis,2026-11-10. Raw log remains private.

The supplied log records PIN registration, saved HID profile0402, native Connect
result0 and SLC state0. It identifies vendor1038/product1412 and an81-byte HID
descriptor. Descriptor parsing succeeds with reportID1, payload7bytes and21
fields. Native report callbacks receive thousands of packets. Thus connection,
device identification and report delivery have been demonstrated for this unit.

The adapter logs initialized0/authorized0 and reports0. Its condition depended
on the stock generic initializer returning success. A type3 report reaches the
hook but cannot activate the merge under that condition. The known descriptor
and synthetic reports decode correctly offline; the packet layout need not be
invented or changed for this repair.

v0.17 gives validated1038:1412 type3 nodes their own input-driver lifecycle before
the stock generic initializer. It tracks those nodes independently of descriptor
cache generations, skips native resource teardown for resources never allocated,
and keeps foreign/native nodes on their original handlers. Actual Bluetooth
success is not fabricated; it was already recorded by v0.16.

Tests use the81-byte static protocol descriptor (no address/key/PIN) with
synthetic control reports, callback lifecycle mocks and emitted MIPS UI routing.
These prove software behavior, not physical button operation. The existing
registration should be kept; this change concerns input initialization only.
