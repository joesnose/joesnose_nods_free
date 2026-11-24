#ifndef ZEEMOTE_INPUT_H
#define ZEEMOTE_INPUT_H
#include "zeemote_hid.h"
typedef struct {unsigned overlay_installed,installed,active,vsh_installed,vsh_reads,reads[4],merged_samples,install_error,guard_index,seen_buttons,motion_seen,last_buttons,lx,ly,min_lx,max_lx,min_ly,max_ly;} ZeeInputStats;
void zee_input_stats(ZeeInputStats *stats);
/* Five-second aggregate, never payloads. Age starts at host report receipt,
 * not physical button press, and cannot measure over-the-air queue delay. */
typedef struct {unsigned reports,gaps,gap_max_us,samples,age_max_us,stale;uint64_t gap_total_us,age_total_us;} ZeeInputTiming;
void zee_input_timing(ZeeInputTiming *out);
/* Heavy bounded installation: call only from the owned 8192-byte worker. */
int zee_input_install(void);
int zee_input_install_overlay(void);
void zee_input_update(const ZeeState *state);
void zee_input_update_kind(const ZeeState *state,unsigned ds4);
/* Only the dedicated GAME worker consumes these edges; no menu/API work
 * or blocking waits on the native HID report stack. */
void zee_input_home_enable(int enabled);
int zee_input_home_take(void);
void zee_input_clear(void);
/* Refreshes an existing state only; called by the trusted live-session monitor. */
void zee_input_keepalive(void);
/* Worker-only power policy: live input or trusted session keepalive, never stale state. */
int zee_input_session_live(void);
#endif
