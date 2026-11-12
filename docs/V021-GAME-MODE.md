# PSP game-mode implementation evidence

v0.20 is confirmed by the user to control XMB. Game launch loses connection.
No GAME-context device log has yet been supplied; the precise radio behavior
across loadexec is unknown.

Confirmed source defects: v0.20 installation unconditionally required
sceVshBridge_Driver, and distributed instructions enabled only VSH. ARK-5
SystemControl/src/plugin.c loads a fresh set of plugins per boot, filtered by
SystemControl/src/configparser.c. The game runlevel matches retail PSP games
and PSP homebrew, separately from POPS. ARK's PLUGINS.TXT supports vsh/game
entries and reads ef0:/seplugins/PLUGINS.TXT on PSP Go.

v0.21 uses sceKernelInitKeyConfig (SDK alias of sceKernelApplicationType).
Application type 0x100 keeps the v0.20 VSH path. Type 0x200 installs the four
public Ctrl syscall-table redirects after validating current Ctrl export
bounds/aliases and checking module bounds again with interrupts suspended.
It never reads or patches VSH text in GAME. It retains current Ctrl entries,
including ARK detours, as original call targets. The callback/worker split is
unchanged. Other application types fail with installation error7.

Tests reject every VSH memory read/write in GAME and cover all four merges,
once-only original calls, original errors, missing bridge, alias mismatch,
POPS refusal and unchanged VSH behavior. No fake native connection or
speculative reconnection command is introduced. A new boot banner distinguishes
plugin not loaded in GAME from game loaded but HID session absent.

PSP cross-build and all12 sanitizer programs passed. See V021-STACK-CHECK.json
for the compiled callback budget; indirect native calls still require review.
Neither tests nor a packaged config prove radio reconnection or physical game
controls. Keep the confirmed-working v0.20 VSH build as rollback.
