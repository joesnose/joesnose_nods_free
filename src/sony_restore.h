/* Warm-reboot registration recovery; no firmware data or device identities. */
#ifndef SONY_RESTORE_H
#define SONY_RESTORE_H
#include "target_handover.h"
static inline int sony_restore_keep_scope(int result,int ready,unsigned selected,unsigned linked,unsigned slot){
 return (uint32_t)result==0x802f0203u&&!ready&&!!(selected&linked&ds4_slot_bit(slot));
}
typedef struct {unsigned slot,flags;int event;unsigned backend_slot,backend_mode;uint8_t address[6];} SonyRestoreState;
static inline int sony_restore_live(const SonyRestoreState *s,unsigned slot){
 return ds4_slot_bit(slot)&&s&&s->slot==slot&&(s->flags&255)==2&&!(s->flags&~0x8602u)&&s->event>0&&s->backend_slot==slot&&(s->backend_mode==2||s->backend_mode==3);
}
/* Pre-query hint only, never connection authorization. Compare the existing
 * registration fingerprint against the two supported Sony names and current
 * native cached address. Do not query a generic preferred controller merely
 * to discover afterward that it is not Sony. No synthetic record leaves here. */
static inline int sony_restore_target_hint(const ZeePreferred *preferred,const SonyRestoreState *state){
 if(!zee_preferred_valid(preferred)||!sony_restore_live(state,preferred->slot))return 0;
 static const char *const names[]={"Wireless Controller","DualSense Wireless Controller"};
 uint8_t record[80]={80};record[4]=(uint8_t)preferred->slot;record[72]=2;record[73]=6;
 memcpy(record+74,state->address,6);
 for(unsigned n=0;n<2;n++){
  memset(record+6,0,66);
  for(unsigned i=0;names[n][i];i++)record[6+i*2]=(uint8_t)names[n][i];
  if(zee_preferred_matches(preferred,record,preferred->slot))return 1;
 }
 return 0;
}
/* Native registration reply is 80 bytes; exported record is a separate ABI.
 * Never reuse an uninitialized output or accept mismatched slot/name/address. */
static inline int sony_restore_record(const uint8_t raw[80],unsigned slot,uint8_t record[80]){
 if(!raw||!record||!ds4_slot_bit(slot)||((unsigned)raw[0]|((unsigned)raw[1]<<8))!=slot)return 0;
 uint8_t p[80]={80};p[4]=(uint8_t)slot;memcpy(p+6,raw+2,66);memcpy(p+72,raw+68,2);
 p[74]=raw[79];p[75]=raw[78];p[76]=raw[76];p[77]=raw[72];p[78]=raw[75];p[79]=raw[74];
 if(!generic_saved_hid(p,slot)||!native_sony_wireless_name(p))return 0;
 memcpy(record,p,80);return 1;
}
typedef int (*SonyRestorePublicRead)(int,void *);
typedef int (*SonyRestoreStartupRead)(unsigned,void *,int);
static inline int sony_restore_read_record(int ready,unsigned slot,SonyRestorePublicRead public_read,SonyRestoreStartupRead startup_read,uint8_t raw[80],uint8_t record[80],int *status){
 if(!raw||!record||!status||!ds4_slot_bit(slot))return 0;
 if(ready){
  if(!public_read){*status=-1;return 0;}
  record[0]=80;*status=public_read((int)slot,record);return *status>=0;
 }
 if(!startup_read){*status=-1;return 0;}
 *status=startup_read(slot,raw,0);return *status>=0&&sony_restore_record(raw,slot,record);
}
static inline int sony_restore_accept(const ZeePreferred *preferred,const uint8_t record[80],const SonyRestoreState *before,const SonyRestoreState *after){
 return zee_preferred_valid(preferred)&&sony_restore_live(before,preferred->slot)&&sony_restore_live(after,preferred->slot)&&before->flags==after->flags&&before->event==after->event&&before->backend_mode==after->backend_mode&&!memcmp(before->address,after->address,6)&&!memcmp(after->address,record+74,6)&&native_sony_wireless_name(record)&&zee_preferred_matches(preferred,record,preferred->slot);
}
#endif
