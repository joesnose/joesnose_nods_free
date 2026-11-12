#ifndef NATIVE_PROFILE_POLICY_H
#define NATIVE_PROFILE_POLICY_H
#include <stdint.h>
#include <string.h>
/* Flag-only default retains legacy0602 behavior. Fresh-record selection below
 * distinguishes the previously registered Wireless Controller HID route. */
static inline int native_saved_hid_mode(unsigned flags){
 return flags==0x0402u||flags==0x0422u||flags==0x0622u;
}
static inline int native_wireless_name(const uint8_t record[80]){
 if(!record)return 0;
 static const char name[]="Wireless Controller";
 for(unsigned i=0;i<sizeof(name);i++)if(record[6+i*2]!=(uint8_t)name[i]||record[7+i*2])return 0;
 return 1;
}
static inline int native_dualsense_name(const uint8_t record[80]){
 if(!record)return 0;
 static const char name[]="DualSense Wireless Controller";
 for(unsigned i=0;i<sizeof(name);i++)if(record[6+i*2]!=(uint8_t)name[i]||record[7+i*2])return 0;
 return 1;
}
static inline int native_sony_wireless_name(const uint8_t record[80]){return native_wireless_name(record)||native_dualsense_name(record);}
/* A bounded saved-name hint for the demonstrated ambiguous0602 case. This
 * chooses a native route, not authentication or a DS4 packet decoder. */
static inline int native_record_hid_mode(const uint8_t record[80],unsigned slot){
 if(!record||slot<1||slot>8||record[0]!=80||record[1]||record[2]||record[3]||
    ((unsigned)record[4]|((unsigned)record[5]<<8))!=slot)return 0;
 unsigned flags=(unsigned)record[72]|((unsigned)record[73]<<8);
 if(native_saved_hid_mode(flags))return 1;
 if(flags!=0x0602)return 0;
 return native_sony_wireless_name(record);
}
typedef struct {
 int queried,query_result,valid;
 unsigned id,flags;
 int requested_mode,selected_mode;
} NativeProfileDecision;
typedef int (*NativeProfileQuery)(int,void *);
typedef int (*NativeProfileConnect)(int,int);
/* Query the current native record for this request. Never use UI/cache data.
 * A failed/empty/mismatched query preserves the caller's exact mode. */
static inline int native_profile_connect(int slot,int mode,NativeProfileQuery query,
 NativeProfileConnect connect,NativeProfileDecision *decision){
 NativeProfileDecision d={0,0,0,0,0,mode,mode};
 if(mode==0 && slot>=1 && slot<=8 && query){
  uint8_t record[80] __attribute__((aligned(16)))={0};record[0]=0x50;
  d.queried=1;d.query_result=query(slot,record);
  if(d.query_result>=0 && record[0]==0x50 && !record[1] && !record[2] && !record[3]){
   d.id=(unsigned)record[4]|((unsigned)record[5]<<8);
   d.flags=(unsigned)record[72]|((unsigned)record[73]<<8);
   d.valid=d.id==(unsigned)slot;
   if(d.valid && native_record_hid_mode(record,(unsigned)slot))d.selected_mode=1;
  }
  /* Native records may contain names/addresses. Retain only numeric policy fields. */
  memset(record,0,sizeof(record));
 }
 if(decision)*decision=d;
 return connect(slot,d.selected_mode);
}
#endif
