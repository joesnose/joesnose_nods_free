#include <assert.h>
#include <stdio.h>
#include "completion_wait.h"
static const void *expected_sel;static void *expected_packet,*expected_timeout;
static unsigned expected_count,expected_cap,expected_bits,expected_mode;static int expected_cb,expected_id,bound,calls;
static unsigned *expected_out;
static int receive(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb){
 ++calls;assert(sel==expected_sel&&count==expected_count&&packet==expected_packet&&cap==expected_cap&&cb==expected_cb);
 if(bound){assert(timeout&&*(uint32_t *)timeout==COMPLETION_WAIT_US);*(uint32_t *)timeout=0;}else assert(timeout==expected_timeout);
 return -1234;
}
static int wait_native(int id,unsigned bits,unsigned mode,unsigned *out,unsigned *timeout){
 ++calls;assert(id==expected_id&&bits==expected_bits&&mode==expected_mode&&out==expected_out);
 if(bound){assert(timeout&&*timeout==COMPLETION_WAIT_US);*timeout=0;}else assert(timeout==expected_timeout);
 if(out)*out=0xabcdef01;
 return -5678;
}
int main(void){
 /* Captured call-site words independently validate our relocated guard model. */
 const uint32_t capture[COMPLETION_GUARDS]={0x27bdff90,0x24020185,0x260c6f00,0x0e091744,
 0x240b0275,0x27a60040,0x27a40030,0x24050001,0x24070010,0x00004021,0x00004821,0xa7b20030,0x0e0901a0,0xa7ab0032,0x0440ffe7,
 0x27bdff90,0x00a0a021,0x3c028824,0x24525cc0,0x14c00003,0x3c038824,0x24725cd0,0x24060069,0x240e0169,
 0x0240f809,0x00004021,0x24050002,0x24060001,0x27a70040,0x0440ffdd,0x0e091770};
 uint32_t w[COMPLETION_GUARDS],patch[2]={0};completion_expected(0x88239200,w);
 for(unsigned i=0;i<COMPLETION_GUARDS;i++)assert(w[i]==capture[i]);
 assert(completion_plan(0x88239200,0xd000,0x88002000,0x88003000,w,patch));
 assert(patch[0]==0x0e000800&&patch[1]==0x0e000c00);
 for(unsigned i=0;i<COMPLETION_GUARDS;i++){w[i]^=1;patch[0]=patch[1]=0x1234;assert(!completion_plan(0x88239200,0xd000,0x88002000,0x88003000,w,patch));assert(patch[0]==0x1234&&patch[1]==0x1234);w[i]^=1;}
 assert(!completion_plan(0x88239201,0xd000,0x88002000,0x88003000,w,patch));
 assert(!completion_plan(0x88239200,0xcbc3,0x88002000,0x88003000,w,patch));
 assert(!completion_plan(0xfffff000,0xd000,0x88002000,0x88003000,w,patch));
 assert(!completion_plan(0x88239200,0xd000,0x08002000,0x88003000,w,patch));
 assert(!completion_plan(0x88239200,0xd000,0x88002000,0x08003000,w,patch));
 assert(!completion_plan(0x88239200,0xd000,0x88002001,0x88003000,w,patch));
 assert(!completion_plan(0x88239200,0xd000,0x88002000,0x88003000,0,patch));
 for(unsigned event=0;event<65536;event++)for(unsigned flags=0;flags<32;flags++){
  uint8_t sel[4]={2,0,(uint8_t)event,(uint8_t)(event>>8)};uint32_t budget=99,packet=0;
  expected_sel=(flags&1)?0:sel;expected_count=(flags&2)?2:1;expected_cap=(flags&4)?15:16;expected_timeout=(flags&8)?&budget:0;expected_cb=(flags&16)?1:0;expected_packet=&packet;
  bound=event==0x275&&expected_sel&&expected_count==1&&expected_cap==16&&!expected_timeout&&!expected_cb;calls=0;
  assert(completion_status_forward(expected_sel,expected_count,&packet,expected_cap,expected_timeout,expected_cb,receive)==-1234&&calls==1&&budget==99);
 }
 for(unsigned flags=0;flags<64;flags++){
  unsigned out=0,budget=77;expected_id=(flags&1)?-1:2;expected_bits=(flags&2)?1:2;expected_mode=(flags&4)?0:1;expected_out=(flags&8)?0:&out;expected_timeout=(flags&16)?&budget:0;int known=!(flags&32);
  bound=flags==0;calls=0;
  assert(completion_disconnect_forward(expected_id,expected_bits,expected_mode,expected_out,expected_timeout,wait_native,known,2)==-5678&&calls==1&&budget==77);
  if(expected_out)assert(out==0xabcdef01);
 }
 assert(!completion_disconnect_bound(0,2,1,(unsigned *)1,0,1,2));
 for(unsigned profiles=0;profiles<65536;profiles++)assert(completion_disconnect_bound(2,2,1,(unsigned *)1,0,1,profiles)==!!(profiles&2));
 puts("PASS: status result selectors, fresh budgets, native errors and argument/output forwarding, pending disconnect eligibility, all31 firmware guards and invalid plans");
}
