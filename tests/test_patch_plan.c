#include "patch_plan.h"
#include <assert.h>
#include <stdio.h>
static uint32_t destination(uint32_t pc,uint32_t word){return ((pc+4)&0xf0000000)|((word&0x03ffffff)<<2);}
int main(void){
 uint32_t original[]={0x27bdfff0,0x24860054};HookPlan p;
 assert(!registration_hook_plan(0x8823dc74,0x88300000,0x88301000,original,&p));
 assert(destination(0x8823dc74,p.entry[0])==0x88300000&&p.entry[1]==0);
 assert(p.trampoline[0]==0x27bdfff0&&p.trampoline[1]==0x24860054);
 assert(destination(0x88301008,p.trampoline[2])==0x8823dc7c&&p.trampoline[3]==0);
 /* Simulate the displaced stack adjustment and a2=device+0x54. */
 uint32_t sp=0x08900000,a0=0x08812340;
 sp+=(int16_t)p.trampoline[0];uint32_t a2=a0+(int16_t)p.trampoline[1];
 assert(sp==0x088ffff0&&a2==0x08812394);
 assert(registration_hook_plan(0x8823dc74,0x08300000,0x88301000,original,&p)<0);
 assert(registration_hook_plan(0x8823dc74,0x88300001,0x88301000,original,&p)<0);
 assert(registration_hook_plan(0x8823dc74,0x88300000,0x08301000,original,&p)<0);
 original[0]^=1;assert(registration_hook_plan(0x8823dc74,0x88300000,0x88301000,original,&p)<0);
 assert(discovery_patch_allowed(0x240e0005,0x104e0003));
 assert(!discovery_patch_allowed(0x240e0005,0x144e0003));
 assert(!discovery_patch_allowed(0x240effff,0x104e0003));
 uint32_t no_pin[]={0x24870001,0x001b12c0};
 assert(!native_hook_plan(0x8823df00,0x88300000,0x88301000,no_pin,HOOK_NO_PIN,0x88239200,&p));
 assert(p.trampoline[0]==no_pin[0]&&p.trampoline[1]==no_pin[1]);
 assert(destination(0x88301008,p.trampoline[2])==0x8823df08);
 assert(native_hook_plan(0x8823df00,0x88300000,0x88301000,no_pin,HOOK_PIN,0x88239200,&p)<0);
 for(unsigned base=0x88100000;base<0x88500000;base+=0x1234){
  uint32_t target=base+0xde5b,profile[]={0x3c070000u|((target+0x8000u)>>16),0x90e30000u|(target&0xffff)};
  assert(!native_hook_plan(base+0x4480,0x88500000,0x88501000,profile,HOOK_PROFILE,base,&p));
  assert(p.trampoline[0]==profile[0]&&p.trampoline[1]==profile[1]);
  profile[1]^=1;assert(native_hook_plan(base+0x4480,0x88500000,0x88501000,profile,HOOK_PROFILE,base,&p)<0);
 }
 puts("PASS: PIN/no-PIN/profile prologues, relocated address guards, trampoline semantics, jumps and discovery guards");
}
