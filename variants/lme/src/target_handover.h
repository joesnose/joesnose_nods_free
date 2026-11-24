/* Prepare an explicitly selected Sony target before native HID startup. */
#ifndef SONY_TARGET_HANDOVER_H
#define SONY_TARGET_HANDOVER_H
#include "ds4_local.h"
/* IRQ-owned operation. Return0 for invalid/non-Sony,1 unchanged,2 changed.
 * Only SLC events change linked bits; preference persistence never erases a
 * genuine link or invents one. No file work or native queries here. */
static inline int sony_target_prepare(Ds4LocalScope *scope,ZeePreferred *preferred,unsigned slot,const uint8_t *record){
 ZeePreferred next;
 if(!record||!native_sony_wireless_name(record)||!zee_preferred_make(record,slot,&next))return 0;
 int changed=!zee_preferred_valid(preferred)||memcmp(preferred,&next,sizeof(next));
 *preferred=next;ds4_scope_observe(scope,slot,record,0,preferred);return changed?2:1;
}
#endif
