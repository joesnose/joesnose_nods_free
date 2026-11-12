# SteelSeries Free: primary-source interoperability findings

Date: 2026-11-10. Research only; no device commands or publication.

## Pairing and identity

The [official SteelSeries user guide](https://downloads.steelseriescdn.com/drivers/userguides/steelseries-free-controller-user-guide_en_win-xp-win-vista-win-7_3264-bit.pdf)
identifies the advertised name as `Zeemote: SteelSeries Free`. It distinguishes
default HID Gamepad mode from Arcade/iCade keyboard mode. Mode toggling uses
A+B for three seconds while powered off; a single blink denotes Gamepad and
a double blink Arcade. The manufacturer explicitly specifies PIN `0000` when
requested in its Android, Windows, and Mac Gamepad pairing instructions.

The same name appears in both mode instructions. Therefore name matching alone
does not establish Gamepad mode. This is stronger PIN evidence than the prior
DS4 inference: it is the actual target manufacturer's prescribed pairing code.
It supports trying the existing native Sony PIN/HID registration path with
properly constructed arguments; it does not establish that native HID input
handling already supports the controller.

The original author of [zeemouse](https://github.com/bitbank2/zeemouse/blob/master/main.c)
uses the uppercase spelling `Zeemote: SteelSeries FREE`. An exact bounded
case-insensitive ASCII match accommodates these two evidenced spellings.
Do not broaden the matcher to arbitrary names containing `Free` or `SteelSeries`.
Require the actual HID profile in the discovered device record as an additional
gate; do not fabricate DS3 identity or profile flags.

## Real HID implementation precedent

MissionControl's [SteelSeries controller declaration](https://github.com/ndeadly/MissionControl/blob/71bbe2d4e97d618775b5a3e2f91bfeaf52d8b75e/mc_mitm/source/controllers/steelseries_controller.hpp)
includes Free hardware identity **VID 0x1038, PID 0x1412**. Chromium's original
[Linux mapping implementation](https://github.com/chromium/chromium/blob/main/device/gamepad/gamepad_standard_mappings_linux.cc)
independently associates the Zeemote SteelSeries FREE with its SteelSeries
product-1412 mapping. SDP/device identification on the actual connected unit
should corroborate this before using it to authorize a report decoder.

MissionControl's [parser](https://github.com/ndeadly/MissionControl/blob/71bbe2d4e97d618775b5a3e2f91bfeaf52d8b75e/mc_mitm/source/controllers/steelseries_controller.cpp)
supports multiple SteelSeries families. Its short report-0x01 layout is a
candidate for Free: report ID, dpad byte, signed 8-bit left X/Y and right X/Y,
then two button bytes. The mapped first button byte uses A/B at bits 0/1,
X/Y at bits 3/4, and L1/R1 at bits 6/7; the second includes start/select at
bits 3/4. Its hat representation uses directions beginning at zero and neutral
0xF. This is an implementation precedent, not a captured Free descriptor for
the user's exact firmware. The family also has longer 0x01 and other report
variants. Do not copy the upstream parser's permissive fallback or unchecked
casts into a kernel plugin: validate identity, report ID and exact available
length, use explicit byte decoding, and release stale input.

The reviewed source is GPL-2.0. Original protocol facts can guide a separate
implementation; copying implementation requires honoring its license.

## Separate non-HID route

The original [bitbank2 esp32_gamepad driver](https://github.com/bitbank2/esp32_gamepad/blob/master/src/esp32_gamepad.cpp)
connects the Free using RFCOMM channel 1 with ESP_SPP_SEC_NONE and decodes
Zeemote messages for buttons and sticks. This demonstrates that another
transport exists, not that these message bytes are HID reports. It cannot be
used as the byte layout for Sony's HID receive path. Switching to RFCOMM would
also require a verified usable native PSP serial-profile path; none was
established in this research.

## Missing evidence and implementation consequence

No verified exact Free Class-of-Device value, complete SDP record, or raw HID
descriptor was found in this pass. Do not invent a CoD such as 0x2508 or
interpret Sony's profile flags 0x0402 as a Bluetooth CoD. A pinned-name-plus-HID
matcher is supported for the registration experiment; a future decoder needs
actual device identity and report/descriptor confirmation.

Registration, ACL/authentication, HID channel connection, and valid usable
reports remain separate success gates. A successful registration slot does
not prove PIN authentication or input delivery. No DS3 spoofing or radio mode
change follows from these findings.
