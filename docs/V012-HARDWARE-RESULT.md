# Native profile v0.12 hardware result

Original analysis,2026-11-09. Raw user log remains private.

The log confirms the repaired mode selection ran. Registration used the
address-only NO_PIN route and returned slot2; its saved profile was0402.
For both user connection attempts, the fresh profile query succeeded, the
selected native mode was1, and the radio event carried flags0402. Both events
reported state2 and the connection helper returned802F0140. No successful HID
initialization or usable controls were observed.

Thus the old0202/0402 mismatch is corrected but was insufficient. These events
do not contain a standard HCI authentication/page/timeout reason, so they do
not establish which radio procedure failed.

The next implementation uses a separate stock UI registration branch that
preserves the genuine discovery record and supplies the native UTF16 PIN.
This is distinct from forcing the later DS3 identity check. The latter occurs
after successful service establishment and enables incompatible DS3-specific
feature/report commands; it cannot fix this earlier connection failure.

The native PIN path is supported by examined firmware callers and a
[first-hand DS4 legacy pairing trace](https://github.com/felis/USB_Host_Shield_2.0/issues/114).
That trace establishes behavior for an original DS4 revision, not all revisions
or clones, and does not itself prove PSP compatibility.
