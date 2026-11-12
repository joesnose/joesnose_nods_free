# Clean generic input and GameStick pairing observation, 2026-11-10

The user confirmed Zeemote works with USB on v0.23, then clarified that NES30
also freezes USB on v0.23 using a registration left from the experiments. This
does not establish a post-v0.23 code regression independent of the device and
saved registration. The user asked to set NES30/wait work aside and resume a
generic test focused on the original white GameStick.

v0.27 starts from v0.23, adds only the reviewed generic descriptor/ownership
expansion and a passive numeric pairing observer. Stock pairing choice for
unfamiliar controllers and Zeemote-only automatic reconnect remain. No generic
CoD-forced PIN path, preferred-slot file or native setup-wait patch is copied.
This is not a claimed repair of GameStick pairing.

The native management receive call accepts six EABI args. The examined caller
sets selectors at SP+60,count23,outputSP,capacity84,timeoutNULL,callback1 (the
last in the original JAL delay slot). The observer changes only this JAL after
nine context-word guards, calls the original native receive, preserves its
result, and records bounded event metadata afterward. Forwarding tests cover
all args/results and unchanged packet bytes. PSP compiled argument/register
and stack evidence is checked separately. No event payload name/address/key
or PIN data is copied into the log. The observer is not on the tiny HID report
thread and does not do filesystem I/O there.

Generic input limits/mapping are documented in README. Native type3 ownership
still requires valid descriptor, free ownership slot and zero existing native
resources. Native DS3 paths remain separate. All15 sanitizer programs passed.
PSP cross-build and compiled frame evidence accompany this package. Hardware
GameStick registration/input and shutdown remain unverified.

Own factual notes/code only; no native firmware, disassembly or private logs.
