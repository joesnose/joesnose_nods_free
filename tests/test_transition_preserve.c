#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static int thread=7,authorized,owned,calls,walks;
static uintptr_t ctx=0x1000,backend=0x2000;
static int quiesce_ok=1,quiesce_calls;
static int zeemote_sony_quiesce_context(const void *c){assert((uintptr_t)c==ctx);quiesce_calls++;return quiesce_ok;}
static int native_control(unsigned op,void *buffer,int cb){calls++;return -19+(int)op+(buffer!=0)+cb;}
static int sceKernelGetThreadId(void){return thread;}
static int ds4_context_authorized(const void *c){return (uintptr_t)c==ctx&&authorized;}
static int zeemote_sony_context_owned(const void *c){return (uintptr_t)c==ctx&&owned;}
static uintptr_t _lw(uintptr_t p){assert(p==ctx+8||p==backend+12);return p==ctx+8?backend:(uintptr_t)native_control;}
#include "../src/transition_preserve.h"
static void walk(void){walks++;assert(transition_thread==thread);assert(transition_control_receive(6,0,0,native_control,(void *)ctx)==0);}
int main(void){
 for(int reboot=0;reboot<2;reboot++)for(int same=0;same<2;same++)for(authorized=0;authorized<2;authorized++)for(owned=0;owned<2;owned++)for(unsigned op=0;op<8;op++)for(int b=0;b<2;b++)for(int cb=0;cb<2;cb++){
  transition_thread=reboot?(same?thread:thread+1):-1;calls=0;
  int skip=reboot&&same&&authorized&&owned&&op==6&&!b&&!cb;
  int result=transition_control_receive(op,(void *)(uintptr_t)b,cb,native_control,(void *)ctx);
  assert(calls==!skip);assert(result==(skip?0:-19+(int)op+b+cb));
 }
 authorized=owned=1;transition_thread=-1;transition_walk_original=walk;transition_reboot_walk();assert(walks==1&&transition_thread==-1);
 quiesce_ok=0;transition_thread=thread;authorized=owned=1;calls=quiesce_calls=0;
 assert(transition_control_receive(6,0,0,native_control,(void *)ctx)==-13&&calls==1&&quiesce_calls==1);
 puts("PASS: reboot/thread/ownership/authorization/op/arguments matrix; native results and exactly-once fallback; reboot scope restoration");
}
