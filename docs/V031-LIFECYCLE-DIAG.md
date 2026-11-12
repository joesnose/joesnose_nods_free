# Transition diagnosis after v0.30 failure, 2026-11-10

Latest log: v0.30 installed stop-aware drain, valid GameStick input, later SLC
state3 disconnect, no logged drain-stop notice and no new GAME banner. User
still hangs. Absence of a deferred stop log is not proof the native flag never
became set; it did not establish a successful transition or a resolved cause.

v0.42 is diagnostic-only, based on v0.28 rather than stacking failed fixes.
Original context-removal dispatch and keyboard/mouse term table entries are
wrapped after nine code/pointer guards. Each observer records bounded numeric
begin/end metadata, calls the original once with unchanged args, records its
exact return and returns that result. Adapter-owned gamepad term still does
its original ownership cleanup, with markers around it. No filesystem calls
run there. Thread snapshots/formatting/file output run on the existing owned
worker. Native report callbacks are unchanged by these diagnostics.

Thread collection starts after first input, every2seconds for6minutes max.
Only known native component names are selected. Thread status/wait IDs and
read-only semaphore/event info use standard kernel APIs; no controller payload
or native state mutation. Logging may itself stop during reboot/FS teardown,
so missing end markers require correlation, not a claim of a crash location.

Sixteen sanitizer programs and compiled checks pass. Context observer frame16,
term observers24; their direct calls are only IRQ helpers and exactly one original call on each compiled return path. Profiler frame864 runs on8192-byte worker. Report path56-byte
callback remains independently checked. Native indirect stacks not fully
proven. Hardware diagnosis depends on the next runtime log; no fix claimed.
