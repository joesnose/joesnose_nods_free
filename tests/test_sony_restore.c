#include <assert.h>
#include <stdio.h>
#include "sony_restore.h"
static void fixture(uint8_t *raw,const char *name){
 memset(raw,0,80);raw[0]=4;
 for(unsigned i=0;name[i];i++)raw[2+i*2]=(uint8_t)name[i];
 raw[68]=2;raw[69]=6;
 raw[79]=11;raw[78]=22;raw[76]=33;raw[72]=44;raw[75]=55;raw[74]=66;
}
static unsigned public_calls,startup_calls;static int native_result;
static int public_read(int slot,void *out){assert(slot==4&&((uint8_t *)out)[0]==80);public_calls++;uint8_t raw[80];fixture(raw,"Wireless Controller");assert(sony_restore_record(raw,4,out));return native_result;}
static int startup_read(unsigned slot,void *out,int cb){assert(slot==4&&!cb);startup_calls++;fixture(out,"Wireless Controller");return native_result;}
int main(void){
 for(int ready=0;ready<2;ready++)for(int success=0;success<2;success++){
  uint8_t raw[80]={0},record[80]={0};int status=99;public_calls=startup_calls=0;native_result=success?1:-33;
  int valid=sony_restore_read_record(ready,4,public_read,startup_read,raw,record,&status);
  assert(valid==success&&status==native_result&&public_calls==(unsigned)ready&&startup_calls==(unsigned)!ready);
  if(success){ZeePreferred target;assert(zee_preferred_make(record,4,&target));SonyRestoreState a={4,0x8602,123,4,2,{11,22,33,44,55,66}};
   assert(sony_restore_accept(&target,record,&a,&a));Ds4LocalScope scope={0};ds4_scope_observe(&scope,4,record,0,&target);ds4_scope_link(&scope,4,0);assert(ds4_scope_active(&scope,4,0x054c,0x05c4));}
 }

 assert(sony_restore_keep_scope((int)0x802f0203u,0,8,8,4));
 assert(!sony_restore_keep_scope((int)0x802f0203u,1,8,8,4));
 assert(!sony_restore_keep_scope((int)0x802f0203u,0,8,0,4));
 assert(!sony_restore_keep_scope((int)0x802f0203u,0,0,8,4));
 assert(!sony_restore_keep_scope((int)0x802f0140u,0,8,8,4));
 assert(!sony_restore_keep_scope((int)0x802f0203u,0,8,8,5));
 for(unsigned model=0;model<2;model++){
  uint8_t raw[80],p[80];fixture(raw,model?"DualSense Wireless Controller":"Wireless Controller");
  assert(sony_restore_record(raw,4,p));assert(p[0]==80&&p[4]==4&&p[72]==2&&p[73]==6);
  const uint8_t address[]={11,22,33,44,55,66};assert(!memcmp(p+74,address,6));
  ZeePreferred target;assert(zee_preferred_make(p,4,&target));
  SonyRestoreState a={4,0x8602,123,4,2,{11,22,33,44,55,66}},b=a;
  assert(sony_restore_accept(&target,p,&a,&b));assert(sony_restore_target_hint(&target,&a));
  Ds4LocalScope scope={0};ds4_scope_observe(&scope,4,p,0,&target);assert(!ds4_scope_active(&scope,4,0,0));
  ds4_scope_link(&scope,4,0);assert(ds4_scope_active(&scope,4,0,0));uint8_t attribute[100];assert(ds4_local_attribute(4,ds4_scope_active(&scope,4,0,0),0x1124,0x206,attribute,sizeof(attribute),0,0)==79);ds4_scope_link(&scope,4,3);assert(!ds4_scope_active(&scope,4,0,0));
  for(unsigned i=0;i<6;i++){b=a;b.address[i]^=1;assert(!sony_restore_accept(&target,p,&a,&b));}
  b=a;b.slot=5;assert(!sony_restore_accept(&target,p,&a,&b));b=a;b.flags=0x8604;assert(!sony_restore_accept(&target,p,&a,&b));
  b=a;b.event=0;assert(!sony_restore_accept(&target,p,&a,&b));b=a;b.event++;assert(!sony_restore_accept(&target,p,&a,&b));
  b=a;b.backend_slot=5;assert(!sony_restore_accept(&target,p,&a,&b));b=a;b.backend_mode=0;assert(!sony_restore_accept(&target,p,&a,&b));
  b=a;b.backend_mode=3;assert(!sony_restore_accept(&target,p,&a,&b));a=b;assert(sony_restore_accept(&target,p,&a,&b));
  p[74]^=1;assert(!sony_restore_accept(&target,p,&a,&b));p[74]^=1;target.check^=1;assert(!sony_restore_accept(&target,p,&a,&b));
  memset(p,0xa5,80);raw[0]=5;assert(!sony_restore_record(raw,4,p)&&p[0]==0xa5);
 }
 uint8_t raw[80],p[80];fixture(raw,"Other Gamepad");assert(!sony_restore_record(raw,4,p));
 fixture(raw,"Wireless Controller");raw[68]=1;assert(!sony_restore_record(raw,4,p));
 /* Generic registrations and reused slots must be rejected before querying. */
 fixture(raw,"8Bitdo NES30 Pro");
 memset(p,0,80);p[0]=80;p[4]=4;p[72]=2;p[73]=6;
 const char *nes="8Bitdo NES30 Pro";for(unsigned i=0;nes[i];i++)p[6+2*i]=(uint8_t)nes[i];
 const uint8_t addr[6]={11,22,33,44,55,66};memcpy(p+74,addr,6);
 ZeePreferred non_sony;assert(zee_preferred_make(p,4,&non_sony));
 SonyRestoreState live={4,0x8602,123,4,2,{11,22,33,44,55,66}};
 assert(!sony_restore_target_hint(&non_sony,&live));
 fixture(raw,"Wireless Controller");assert(sony_restore_record(raw,4,p));ZeePreferred sony;assert(zee_preferred_make(p,4,&sony));assert(sony_restore_target_hint(&sony,&live));
 for(unsigned i=0;i<6;i++){SonyRestoreState stale=live;stale.address[i]^=1;assert(!sony_restore_target_hint(&sony,&stale));}
 live.slot=5;assert(!sony_restore_target_hint(&sony,&live));
 puts("PASS: warm-start raw/export ABI, DS4/DualSense target matching, two live native snapshots, address/event/backend/race rejection and disconnect invalidation");
}
