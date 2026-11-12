#ifndef ZEEMOTE_HOOKS_H
#define ZEEMOTE_HOOKS_H
#ifndef ZEE_HOOK_HOST_TEST
#include <psploadcore.h>
#endif
typedef void (*ZeeLogger)(const char *,...);
int zeemote_input_install_module(SceModule *module,ZeeLogger logger);
void zeemote_input_log_tick(ZeeLogger logger);
typedef int (*ZeeDs4Authorize)(const void *);
void zeemote_ds4_set_authorizer(ZeeDs4Authorize);
int zeemote_ds4_install_read_trace(SceModule *,ZeeLogger);
int zeemote_input_ready(void);
int zeemote_gamestick_authorized(void);
int zeemote_sony_context_owned(const void *);
int zeemote_sony_quiesce_context(const void *);
#endif
