#include <assert.h>
#include <stdio.h>
#include "status_timeout.h"
static uint16_t *expected_reply;
static uint32_t *expected_timeout;
static int expected_cb,expect_bound,calls;
static int receive(uint16_t *reply,uint32_t *timeout,int cb){
 ++calls;assert(reply==expected_reply);assert(cb==expected_cb);
 if(expect_bound){assert(timeout&&*timeout==STATUS_TIMEOUT_US);*timeout=0;}
 else assert(timeout==expected_timeout);
 if(reply)reply[1]=0x1234;
 return -123;
}
int main(void){
 uint32_t words[STATUS_GUARDS]={0x02403021,0x00002821,0x96090000,0x96020002,0x8e080008,0xa7a90000,0xa7a20004,0x0e090150,0xafa80008,0x27bdffc0,0xafb60038};
 assert(status_context(0x88239200,0xd000,words));
 for(unsigned i=0;i<STATUS_GUARDS;i++){words[i]^=1;assert(!status_context(0x88239200,0xd000,words));words[i]^=1;}
 assert(!status_context(0x88239201,0xd000,words));assert(!status_context(0x88239200,0x7347,words));assert(!status_context(0xfffff000,0xd000,words));
 for(unsigned response=0;response<65536;response++)for(unsigned flags=0;flags<32;flags++){
  uint16_t reply[6]={(uint16_t)response,0};uint32_t timeout=71;int bounded=-1;
  int has=flags&1,cb=(flags>>1)&1,target=(flags>>2)&1,role=(flags>>3)&3;
  expected_reply=reply;expected_timeout=has?&timeout:0;expected_cb=cb;
  expect_bound=!has&&target&&((role==1&&response==0x185&&!cb)||(role==2&&response>=0x100&&response<=0x1ff));calls=0;
  assert(status_receive_forward(reply,expected_timeout,cb,target,role,receive,&bounded)==-123);
  assert(calls==1&&bounded==expect_bound&&reply[1]==0x1234&&timeout==71);
 }
 assert(!status_bound(0x16e,0,2,1,2));assert(!status_bound(0x16e,0,-1,1,2));
 expected_reply=0;expected_timeout=0;expected_cb=0;expect_bound=0;calls=0;int bounded=-1;
 assert(status_receive_forward(0,0,0,1,1,receive,&bounded)==-123&&calls==1&&!bounded);
 puts("PASS: all response IDs and timeout/callback/identity/worker/main/unrelated-role combinations, fresh native timeout, once-only exact forwarding, native errors/results and all11 firmware guards");
}
