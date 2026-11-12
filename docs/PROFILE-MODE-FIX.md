# Evidence for native profile selection

Original analysis,2026-11-09; raw captures remain private.

Earlier supplied logs show profile0402 for the selected slot, followed by
HID_CONNECT mode0 and a failed SLC event with flags0202. No mode1 connection
attempt was recorded in those examined logs.

The examined official Bluetooth menu +146C4 identifies type1 as PS3 controller
and type2 as Bluetooth HID. Its +14E0C path exposes the action for type1, while
+15514 calls the HID Connect API with mode0. Our earlier menu admission patch
allowed type2 through that action without adapting the mode.

In the examined BSMan, +A730/+A770 maps mode0 to0202 and nonzero to0402. Connected
variant checks at+A9B0 distinguish0202,8002 and0402; the category mapping at+AA44
uses0402 for category4. HID initialization at+A128 branches on0400. The repair
therefore selects the known saved profile rather than assigning undocumented
security meanings to bits or blindly trying another mode after failure.

The production adapter validates the current record and selects mode1 only for
exact0402. It makes one connection call and retains the actual result. Tests
cover all65536flags plus query/record/slot errors. Hardware success remains
unproven; authentication and reports may require further changes.
