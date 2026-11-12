Historical v0.16 validation. v0.17 changes the validated target to an adapter-owned lifecycle; see V016-HARDWARE-RESULT.md and the current README.

# Zeemote v0.16 adapter: validation and boundaries

Original implementation and analysis,2026-11-10.

The registration object retains its PIN methods but uses a private HID-category
getter. The previous audio-category mismatch could cause native slot creation
to be followed by menu rejection/deletion. The new matcher targets the exact
manufacturer name, case-insensitively, and preserves the original discovery ID.
No CoD value, radio identity or service capability is fabricated.

The inspected native HIDSvc report table directs both generic type3 and DS3
type4 to a handler that only decodes type4. The adapter changes only type3
callbacks and one descriptor-parser callsite, after checking their firmware
context. Original parser/initializer/report/terminator returns and resource
lifetimes are preserved. Type4 DS3 function pointers are checked and untouched.
Native initializer arguments supply PID/VID before the generic native code
reuses an identity field; gating therefore uses those arguments, not that field.

Input is authorized only for1038:1412, a supported descriptor, a matching live
context/node generation and a valid report. Fields come from the descriptor;
there are no assumed Free report offsets. Supported application usages are
joystick/gamepad, with bounded absolute axes/buttons/hat and report IDs. Maximum
descriptor512bytes and report64bytes. Unsupported layouts produce no controls.
The report tests use explicitly synthetic descriptors, not a captured Free.

The PSP input merge preserves local controls. Termination, malformed authorized
reports, descriptor/session replacement and module disappearance invalidate
remote input. The500ms monitor refreshes existing state only while the native
session remains authorized. It is not an RF heartbeat. A1500ms deadman clears
state if the monitor stops, without requiring periodic unchanged HID reports.

No filesystem logging occurs in native descriptor/input callbacks. The logger
thread prints counters and a bounded descriptor only for the identified target,
plus report-framing metadata. Raw keyboard keystrokes, PINs, Bluetooth addresses
and link keys are not intentionally logged. No actual user descriptor or report
is bundled in this source package.

The11 passing test programs validate emitted MIPS and modeled native interfaces,
parser bounds and malformed cases, exact installer guards, identity/framing,
module and node lifetime races, and syscall merging. PSP cross-build succeeds.
These checks do not demonstrate radio connection, authentication or physical
input. The previous failure occurred before HID initialization. Proper PIN/HID
registration is an evidence-backed candidate, not a promised radio fix.
