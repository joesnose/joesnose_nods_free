#include "lifecycle_trace.h"
#include <assert.h>
#include <stdio.h>
static int first,second,calls;static unsigned phases[4],kinds[4],count;static int values[4];
static int remove_mock(void *p){assert(p==&first);calls++;return -123;}
static int term_mock(void *c,void *n){assert(c==&first&&n==&second);calls++;return 42;}
static void record(unsigned p,unsigned k,int r){assert(count<4);phases[count]=p;kinds[count]=k;values[count++]=r;}
int main(void){
 assert(life_remove_forward(&first,remove_mock,record)==-123&&calls==1&&count==2);
 assert(phases[0]==1&&phases[1]==2&&!kinds[0]&&!kinds[1]&&values[1]==-123);
 assert(life_term_forward(2,&first,&second,term_mock,record)==42&&calls==2&&count==4);
 assert(phases[2]==3&&phases[3]==4&&kinds[2]==2&&kinds[3]==2&&values[3]==42);
 uint32_t b=0x8822f800,c[LIFE_GUARDS];life_expected(b,c);assert(life_context_ok(b,0x4ec4,c));
 for(unsigned i=0;i<LIFE_GUARDS;i++){c[i]^=1;assert(!life_context_ok(b,0x4ec4,c));c[i]^=1;}
 assert(!life_context_ok(b,0x4e70,c));assert(!life_context_ok(0xfffff000,0x4ec4,c));
 assert(life_watched("SceHIDSvc_Recv")&&life_watched("SceBSMan_Q")&&life_watched("SceInputThread"));
 assert(!life_watched("Game title")&&!life_watched("DS4TraceLogger")&&!life_watched(NULL));
 puts("PASS: lifecycle begin/end metadata, exact original calls/args/results, nine native teardown guards and bounded kernel-thread name filter");
}
