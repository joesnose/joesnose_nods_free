# PSP Go native HID route and authentication ABI notes

Date: 2026-11-09. Original static analysis of privately retained PSP-side captures. No hardware commands were executed for this work. No radio executable has been identified. Offsets describe the captured modules, not universal patch addresses.

## Result

Sony's native CDC backend already carries framed HID profile data. Retaining the radio's on-chip stack is therefore an alternative architecture to exposing HCI. It is not yet a demonstrated DS4 connection route: the observed service-link failure occurs before native HID startup. No arbitrary L2CAP PSM open API or link-key import was established in the inspected functions.

## Service connection and earliest failure

BSMan base: 0x88239200.

| Offset | Observation |
| --- | --- |
| +A730 | HID connect export F188A405 / driver 27755521 chooses profile flags 0x0202 for argument zero or 0x0402 otherwise. |
| +2728 | Outgoing service helper sends request 0x68, expects response 0x168, and waits for completion when the immediate response indicates pending. |
| +2044 | Event 0x263 handler consumes slot, state and profile flags. Its state-2 branch sets failure event bit 4 for an existing pending connection. |
| +2728 | A pending connection receiving that failure bit returns 0x802F0140. This is not a decoded HCI reason. |
| +22C0 | State-0 success dispatch selects a profile callback, including the HID callback. |
| +5FF8 | HID callback queues worker operation 0x10. |
| +1BF8 | Worker dispatches operation 0x10 to HID startup. |
| +A128 | HID startup chooses CDC channel 2 or 3, queries device identity, initializes HIDSvc and begins asynchronous 64-byte input transfers. |

The HID startup's additional identity-specific calls are conditional on Sony DS3 identity 054C:0268. They occur after successful service establishment. Altering those calls cannot by itself explain or fix the earlier observed state-2 failure.

Incoming service request event 0x266 has a separate host decision path. It checks native profile capability and service state before sending request 0x6A with an allow/reject field. That decision does not expose the radio's actual authentication exchange.

## Native data interface and corrected framing

CDC base: 0x88226700.

| Offset / export | Observed operation |
| --- | --- |
| +1398 / EC0493A4 | Synchronous read on a channel index in 0..4. |
| +1514 / A5E108BB | Asynchronous channel read. |
| +1640 / F8C6AFBD | Synchronous channel write. |

These are profile-channel operations, not established arbitrary L2CAP or HCI primitives.

BSMan receive completion +9EE0 queues transport fragments. Fragments with a first-byte high nibble of 0xA0 terminate the assembled message; preceding nonterminal fragments are put ahead of that terminal fragment. BSMan reader +981C removes one marker byte from each fragment, copies the remaining bytes into the caller's capacity, and returns when it reaches the terminal fragment.

Consequently, the CDC marker byte must not automatically be treated as a raw HIDP 0xA1 header. A future adapter must reproduce Sony fragment framing and report boundaries before applying the DS4 report decoder.

HIDSvc base: 0x8822F800. Initialization +076C contains DS3-specific handling and a generic descriptor path, with a 512-byte descriptor limit. Its report reader +0E2C requests only 64 bytes from the backend. Full DS4 reports exceed that capacity, so the existing report path cannot simply be assumed adequate. Fragmentation already exists below that buffer; USB packet size alone does not establish truncation. The assembled-report capacity and generic parsing need explicit adaptation.

BSMan +AC38 and +AC7C construct native report transactions through requests 0x25 and 0x26, respectively, with asynchronous result handling. These are useful future ABI leads, not demonstrated DS4 feature-report support.

## Configuration and pairing ABI

### Requests 0x60 / 0x61

Set wrapper +3CDC and get wrapper +3FC4 operate on a 12-byte caller object whose first word is its size. The native set payload is six bytes: four scalar bytes and one packed bitfield halfword. The caller's bytes 8, 9, 10 and 11 contribute widths 1, 2, 3 and 1 bits, respectively. The getter reverses that packing. Other fields have nonzero/range validation.

No field names or security semantics are assigned here. This is not an address-plus-16-byte-link-key record and must not be presented as a link-key import API. Changing individual bits as guessed authentication options is not justified.

### Request 0x62

Wrapper +5BA0 constructs a 34-byte native payload by zero-initializing storage, copying 32 bytes from its caller, and retaining a final zero halfword. Registration helper +6758 sends the same form before request 0x65, caches the caller's 32 bytes, and marks registration pending.

The asynchronous event 0x269 handler uses that cached buffer as up to 16 zero-terminated 16-bit character units, sets a character count and acceptance field, then responds through request 0x6E. This caller/consumer relationship supports interpreting the observed 0x62 use as PIN/passkey text setup. A generic standalone link-key-import capability has not been found. Payload length alone would be inadequate evidence either way.

### Request 0x6E

Helper +3194 sends a 40-byte payload and expects response 0x16E. The caller at +4EDC accepts a size-tagged 40-byte caller object and constructs native fields consisting of slot, acceptance-like value, character count, and copied 16-bit character data. The internal event 0x269 responder uses the same layout with a zero-filled text buffer and at most 16 units from cached registration text.

This is evidence of a PIN/passkey response path, not proof that the radio accepts imported link keys, SSP confirmation parameters or a specific DS4 legacy PIN. Do not invent a PIN from these layouts. No secrets are included in these notes.

## Flags 0x0202 / 0x0402

Both share low profile bit 0x0002, which selects the HID profile callback. Their high bits are retained in the slot's service flags. HID startup checks bit 0x0400 to select a different internal channel/setup path; otherwise it additionally checks 0x0200 and 0x8000. This establishes a functional difference in the PSP-side backend configuration.

It does not establish labels such as encrypted/plain, authenticated/unauthenticated, master/slave or legacy/SSP. No flag flip was tested, and changing the value cannot yet be described as a pairing fix.

## Next useful work

The native architecture permits a future DS4-specific framed-data adapter without replacing the complete host stack, provided radio-side service establishment can be repaired. The missing authentication evidence is below the observed service-link result. Useful next work is to correlate native configuration and service flags with documented radio management/security capabilities, while retaining a GET-only boundary until a supported command and recovery path are established.

Existing hardware observations remain: no successful RAM write, HCI attachment or DS4 pairing has been demonstrated. This document contains original findings and factual offsets; firmware, decompiler output and user logs are retained privately.

## Independent pairing-route audit

The existing Linux helper's report 0x13 writes the DS4 target address plus sixteen zero key bytes. This matches the examined BlueZ reference implementation, whose comment explains that it cannot reload a supplied key into the kernel there. It does not establish a matching bond in the PSP. Conversely, zero key bytes alone are insufficient evidence for the observed failure's cause. Rewriting the same target is not a useful authentication experiment and can clear controller-side key material again.

The examined PSP usbps3controller module implements USB-peripheral DS3-style feature requests F2/F5. Its address exchange is not evidence of a DS4 host-link-key provisioning path.

The official Bluetooth UI has two native registration routes even for devices selected through discovery:

- A discovery record is 84 bytes: size word at 0, identifier at 4, UTF-16 name at 6..71, class-of-device bytes at 72..74, address bytes at 75..80. Remaining bytes are not assigned here.
- UI selection at +13E84 checks three predicates on those class bytes: byte 1 bit 1; byte 0 bit 5; byte 2 bit 6.
- If all three are false, the base registration object has execution method +13934, which forwards record address bytes through +138E8 into the address-only native registration export (request 0x7E).
- Other branches create an object retaining the genuine 84-byte record and a PIN buffer. Constructors +139E0/+13A84 initialize four UTF-16 zero digits. The execution method +13B28 passes the record and PIN through +138C8 into general registration (requests 0x62/0x65).

A hypothetical class value 0x002508 makes all three predicates false and therefore takes the address-only route. This is an offline routing result; it does not establish the actual discovery class in the user's particular attempt. Name/class matching alone cannot establish a controller's DS4 identity. No PIN-path patch or cached-PIN override was implemented from this audit.

The general registration worker uses the record's size and identifier, while the UI retains its remaining genuine discovery metadata. An address-only hook receives only six address bytes; it cannot safely invent the discovery identifier needed by general registration. Consequently, redirecting that hook blindly is not an acceptable implementation.
