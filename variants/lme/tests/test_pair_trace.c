#include "pair_trace.h"
#include <assert.h>
#include <stdio.h>
static unsigned calls,records;static int result=1;static uint8_t packet[84];static int selector;static PairEvent captured;
static int original(const void *s,unsigned n,void *p,unsigned cap,void *t,int cb){assert(s==&selector&&n==23&&p==packet&&cap==84&&!t&&cb==1);calls++;return result;}
static void record(const PairEvent *p){captured=*p;records++;}
int main(void){
 packet[0]=0x69;packet[1]=2;packet[2]=44;packet[4]=2;memset(packet+6,'7',32);uint8_t saved[84];memcpy(saved,packet,84);
 assert(pair_trace_forward(&selector,23,packet,84,NULL,1,original,record)==1&&calls==1&&records==1);
 assert(captured.id==0x269&&captured.slot==2&&!captured.word1&&!memcmp(saved,packet,84));
 result=-123;assert(pair_trace_forward(&selector,23,packet,84,NULL,1,original,record)==-123&&calls==2&&records==1);
 result=0;assert(!pair_trace_forward(&selector,23,packet,84,NULL,1,original,record)&&calls==3&&records==1);
 PairEvent e;packet[0]=0x64;packet[2]=84;packet[6]=0x34;packet[7]=0x12;assert(pair_trace_decode(packet,84,&e)&&e.word1==0x1234);
 packet[2]=85;assert(!pair_trace_decode(packet,84,&e));packet[2]=3;assert(!pair_trace_decode(packet,84,&e));packet[2]=84;
 assert(!pair_trace_decode(packet,83,&e));packet[1]=3;assert(!pair_trace_decode(packet,84,&e));
 uint32_t base=0x88239200,hook=0x88310000,patched=0;
 uint32_t context[9]={0x27a40060,0x24050017,0x03a03021,0x24070054,0x00004021,0,0x24090001,0x044001ca,0x03a0a821};
 context[5]=0x0c000000|(((base+0x7480)>>2)&0x3ffffff);
 assert(!pair_trace_plan(base,0xce00,hook,context,&patched));assert((patched&0x3ffffff)<<2==(hook&0x0fffffff));
 for(unsigned i=0;i<9;i++){context[i]^=1;uint32_t out=0xaabbccdd;assert(pair_trace_plan(base,0xce00,hook,context,&out)<0&&out==0xaabbccdd);context[i]^=1;}
 assert(pair_trace_plan(base,0xce00,0x08310000,context,&patched)<0);
 puts("PASS: passive six-argument receive forwarding/exact results, bounded numeric metadata, no PIN/raw packet logging or mutation, all nine caller guards and unchanged failure output");
}
