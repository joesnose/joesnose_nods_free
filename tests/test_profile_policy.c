#include "profile_policy.h"
#include <assert.h>
#include <stdio.h>
static int query_count,connect_count,query_status,reported_id,queried_slot,connected_slot,connected_mode;
static unsigned returned_flags;
static int bad_size;
static int query(int slot,void *buffer){
 uint8_t *p=buffer;assert(((uintptr_t)p&15)==0);
 assert(p[0]==80);for(unsigned i=1;i<80;i++)assert(p[i]==0);
 assert(connect_count==0);query_count++;queried_slot=slot;
 p[4]=(uint8_t)reported_id;p[5]=(uint8_t)(reported_id>>8);
 p[72]=(uint8_t)returned_flags;p[73]=(uint8_t)(returned_flags>>8);
 if(bad_size)p[1]=1;
 return query_status;
}
static int connect(int slot,int mode){
 connect_count++;connected_slot=slot;connected_mode=mode;return -123456;
}
static NativeProfileDecision run(int slot,int mode,int status,int id,unsigned flags,int expected){
 query_count=connect_count=0;query_status=status;reported_id=id;returned_flags=flags;
 NativeProfileDecision d;
 assert(native_profile_connect(slot,mode,query,connect,&d)==-123456);
 assert(connect_count==1 && connected_slot==slot && connected_mode==expected);
 assert(d.requested_mode==mode && d.selected_mode==expected);
 assert(query_count==((slot>=1&&slot<=8&&mode==0)?1:0));
 if(query_count)assert(queried_slot==slot);
 return d;
}
int main(void){
 NativeProfileDecision d=run(2,0,0,2,0x0402,1);assert(d.valid && d.queried);
 run(2,0,1,2,0x0402,1);
 run(2,0,-1,2,0x0402,0);
 run(2,0,0,1,0x0402,0);run(2,0,0,0,0x0402,0);
 run(2,0,0,2,0x0202,0);run(2,0,0,2,0x0602,0);
 run(2,0,0,2,0x8402,0);run(2,0,0,2,0x0403,0);run(2,0,0,2,0x1402,0);run(2,0,0,2,0x0400,0);
 run(2,0,0,2,0,0);run(2,1,0,2,0x0202,1);run(2,-7,0,2,0x0402,-7);
 run(0,0,0,0,0x0402,0);run(9,0,0,9,0x0402,0);
 bad_size=1;run(2,0,0,2,0x0402,0);bad_size=0;
 query_count=connect_count=0;
 assert(native_profile_connect(2,0,NULL,connect,&d)==-123456);
 assert(connect_count==1 && connected_mode==0 && !d.queried);
 /* A changed record is queried again; a previous valid result cannot be reused. */
 run(2,0,0,2,0x0402,1);run(2,0,0,2,0x0202,0);
 for(unsigned flags=0;flags<=65535;flags++){
  int expected=flags==0x0402||flags==0x0422||flags==0x0622;
  run(3,0,0,3,flags,expected);
 }
 puts("PASS: production query/connect path, fresh aligned records, all 65536 flag combinations, fail-preserving behavior and no retry");
 return 0;
}
