# GAME reconnect evidence, 2026-11-10

User log: v0.21 GAME banner context0x200, all four native modules indicated,
HID hooks installed, then no GAME service-link event, descriptor or report.
Later VSH instance initializes and merges valid Free reports again. This rules
out a missing GAME instance for this attempt; it does not prove a radio-level
reason or establish an attempted GAME connection.

Saved profile export CBD232D1 returns an80-byte object: size0, slot4,
UTF16 name6..71, flags72..73, address74..79. Captured reference code checks
size0x50 and populates those fields. v0.22 only reads that object and immediately
clears local copies. Exactly one saved-name/slot/HID match is required.

The native F188A405 export is already guarded/hooked by the working VSH code.
v0.22 invokes its existing wrapper/trampoline once from the owned8192-byte
GAME worker, two seconds after worker startup, only when HIDSvc hooks exist
and validated input is inactive. It does not invoke connection from the tiny
native report thread. Mode selection matches the existing native profile
policy. Menu wrapper logging preserves original outcome. No retry loop or
new native command format is introduced.

Thirteen sanitizer programs pass, including a new pure reconnect policy test.
Physical GAME reconnection remains unverified. Native success can mean request
accepted rather than usable controls; descriptor/init/report/read evidence
must confirm success. No Sony code, logs or address data is packaged here.
