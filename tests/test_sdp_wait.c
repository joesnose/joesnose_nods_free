#include <assert.h>
#include <stdio.h>
#include "sdp_wait.h"
static const void *expected_sel;static void *expected_packet,*expected_timeout;
static unsigned expected_count,expected_cap,calls;static int expected_cb,bound;
static int receive(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb){
 ++calls;assert(sel==expected_sel&&count==expected_count&&packet==expected_packet&&cap==expected_cap&&cb==expected_cb);
 if(bound){assert(timeout&&*(uint32_t *)timeout==SDP_WAIT_US);*(uint32_t *)timeout=0;}
 else assert(timeout==expected_timeout);
 return -321;
}
int main(void){
 uint32_t w[SDP_GUARDS]={0x02402021,0x24050001,0x02203021,0x2407004c,0x02a04021,0x0e0901a0,0x03c04821,0x00408021,0x27bdffd0,0xafb60028};
 assert(sdp_context(0x88239200,0xd000,0x88400000,w));
 for(unsigned i=0;i<SDP_GUARDS;i++){w[i]^=1;assert(!sdp_context(0x88239200,0xd000,0x88400000,w));w[i]^=1;}
 assert(!sdp_context(0x88239200,0x7487,0x88400000,w));assert(!sdp_context(0x88239201,0xd000,0x88400000,w));assert(!sdp_context(0x88239200,0xd000,0x18400000,w));
 uint8_t packet[76]={0};uint32_t budget=123;
 for(unsigned event=0;event<65536;event++)for(unsigned f=0;f<32;f++){
  uint8_t sel[4]={0x34,0x12,event&255,event>>8};
  expected_sel=(f&1)?NULL:sel;expected_count=(f&2)?2:1;expected_cap=(f&4)?75:76;expected_timeout=(f&8)?&budget:NULL;expected_cb=(f&16)?1:0;expected_packet=packet;
  bound=expected_sel&&expected_count==1&&expected_cap==76&&!expected_timeout&&event==0x272;calls=0;
  assert(sdp_forward(expected_sel,expected_count,packet,expected_cap,expected_timeout,expected_cb,receive)==-321&&calls==1&&budget==123);
 }
 uint8_t sel[4]={0,0,0x72,2};assert(!sdp_bound(sel,1,76,NULL,2));assert(!sdp_bound(sel,1,76,NULL,-1));
 puts("PASS: all65536 event IDs and32 argument/mode combinations, fresh native budget, exact six-argument once-only forwarding/results and10 SDP call-context guards");
}
