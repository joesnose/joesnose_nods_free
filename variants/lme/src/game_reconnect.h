#ifndef ZEE_GAME_RECONNECT_H
#define ZEE_GAME_RECONNECT_H
#include <stdint.h>
#include <string.h>
#include "profile_policy.h"
typedef struct {unsigned queries,matches,slot,flags,attempted,preferred,deferred;int result;} ZeeReconnect;
static inline int generic_saved_hid(const uint8_t p[80],unsigned slot){
 if(!p||slot<1||slot>8||p[0]!=80||p[1]||p[2]||p[3]||
    ((unsigned)p[4]|((unsigned)p[5]<<8))!=slot)return 0;
 unsigned flags=((unsigned)p[72]|((unsigned)p[73]<<8))&~0x0020u;
 if(flags!=0x0202&&flags!=0x0402&&flags!=0x0602)return 0;
 /* Accept any nonempty bounded UTF16 name, without a vendor/name whitelist. */
 if(!p[6]&&!p[7])return 0;
 for(unsigned i=1;i<33;i++)if(!p[6+2*i]&&!p[7+2*i])return 1;
 return 0;
}
/* Fingerprint is registration matching, not authentication. Native Bluetooth
 * still authenticates the controller. Flags are excluded because they evolve. */
typedef struct {uint32_t magic,version,slot,lo,hi,check;} ZeePreferred;
_Static_assert(sizeof(ZeePreferred)==24,"fixed target record");
static inline uint32_t zee_preferred_check(const ZeePreferred *p){
 uint32_t words[5]={p->magic,p->version,p->slot,p->lo,p->hi},h=2166136261u;
 for(unsigned i=0;i<5;i++)for(unsigned j=0;j<4;j++){h^=(words[i]>>(j*8))&255;h*=16777619u;}
 return h;
}
static inline int zee_preferred_valid(const ZeePreferred *p){
 return p&&p->magic==0x48494454u&&p->version==1&&p->slot>=1&&p->slot<=8&&p->check==zee_preferred_check(p);
}
static inline int zee_preferred_make(const uint8_t record[80],unsigned slot,ZeePreferred *p){
 if(!p||!generic_saved_hid(record,slot))return 0;
 unsigned any=0,not_ff=0;for(unsigned i=74;i<80;i++){any|=record[i];not_ff|=record[i]^255u;}
 if(!any||!not_ff)return 0;
 uint64_t hash=UINT64_C(14695981039346656037);
 for(unsigned i=0;i<33;i++){
  unsigned a=record[6+2*i],b=record[7+2*i];hash^=a;hash*=UINT64_C(1099511628211);hash^=b;hash*=UINT64_C(1099511628211);
  if(!a&&!b)break;
 }
 for(unsigned i=74;i<80;i++){hash^=record[i];hash*=UINT64_C(1099511628211);}
 ZeePreferred v={0x48494454u,1,slot,(uint32_t)hash,(uint32_t)(hash>>32),0};v.check=zee_preferred_check(&v);*p=v;return 1;
}
static inline int zee_preferred_matches(const ZeePreferred *p,const uint8_t record[80],unsigned slot){
 ZeePreferred candidate;
 return zee_preferred_valid(p)&&p->slot==slot&&zee_preferred_make(record,slot,&candidate)&&p->lo==candidate.lo&&p->hi==candidate.hi;
}
/* Query fresh native records; an existing preference must match exactly.
 * Without one, retain unique-device selection. Never connect twice. */
static inline ZeeReconnect zee_game_reconnect_policy(int active,const ZeePreferred *preferred,int (*query)(int,void *),int (*connect)(int,int),int (*defer)(unsigned)){
 ZeeReconnect d={0};unsigned unique_slot=0,unique_flags=0;int unique_mode=0,selected_mode=0;
 int has_preferred=zee_preferred_valid(preferred);
 if(active||!query||!connect)return d;
 for(unsigned slot=1;slot<=8;slot++){
  uint8_t record[80] __attribute__((aligned(16)))={0};record[0]=80;
  ++d.queries;int status=query((int)slot,record);
  if(status>=0&&generic_saved_hid(record,slot)){
   ++d.matches;unique_slot=slot;unique_flags=(unsigned)record[72]|((unsigned)record[73]<<8);unique_mode=native_record_hid_mode(record,slot);
   if(has_preferred&&zee_preferred_matches(preferred,record,slot)){d.preferred=1;d.slot=slot;d.flags=unique_flags;selected_mode=unique_mode;}
  }
  memset(record,0,sizeof(record));
 }
 if(!has_preferred&&d.matches==1){d.slot=unique_slot;d.flags=unique_flags;selected_mode=unique_mode;}
 if(d.slot&&defer&&defer(d.slot)){d.deferred=1;return d;}
 if(d.slot){d.attempted=1;d.result=connect((int)d.slot,selected_mode);}
 return d;
}
static inline ZeeReconnect zee_game_reconnect_preferred(int active,const ZeePreferred *preferred,int (*query)(int,void *),int (*connect)(int,int)){
 return zee_game_reconnect_policy(active,preferred,query,connect,NULL);
}
static inline ZeeReconnect zee_game_reconnect(int active,int (*query)(int,void *),int (*connect)(int,int)){
 return zee_game_reconnect_preferred(active,NULL,query,connect);
}
#endif
