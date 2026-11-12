# GameStick report3 dispatch gap, 2026-11-10

The newest user log confirms v0.27 no-PIN registration to slot3, flags0402,
selected mode1, SLC state0, original native connection result0. Captured HID
identity0f0d:1011, descriptor232bytes, selected gamepad report3/payload9,
parser status0, owned/authorized1, reports0. This is beyond pairing and native
HID setup; no raw input packet has yet reached the adapter.

The descriptor has separate keyboard1, mouse2 and gamepad3 applications.
Native HIDSvc's generic gamepad collection scanner only skips items; it does
not fill the node report ID. Keyboard/mouse handlers do. Native dispatch with
a numbered first node reads each incoming first byte and compares it against
the linked nodes' second byte, strips the ID and calls the matched handler.
A zero-ID generic node in that list cannot receive report3.

The fix assigns validated layout.report_id to the owned numbered generic
node's second byte during initialization. This is within the existing IRQ/
ownership/type/resource guard; no native allocation, keyboard/mouse node,
DS3/type4, dispatcher or report buffer is altered. Known Free1038:1412 keeps
its previously confirmed native route. Route before/after fields are logged
at init. The first successful callback will still validate framing and report
length before publishing PSP state. Original lifetime/termination is retained.

The static232-byte descriptor is included as protocol metadata, with a
synthetic report that selects3 past1/2 and decodes the expected Cross bit after
ID stripping/restoration. Existing full/stripped report tests remain. Fifteen
sanitizer programs pass; physical GameStick input remains unverified.
This changes no pairing, registration, auto reconnect, native wait or USB
shutdown behavior. No private logs, addresses, keys, raw buttons or native
firmware/disassembly are included.
