#include "patch_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t destination(uint32_t pc,uint32_t w){return ((pc+4)&0xf0000000)|((w&0x3ffffff)<<2);}
static uint32_t run(const HidMenuPlan *p,uint32_t stub,uint32_t type,uint16_t flags){
 uint32_t r[32]={0};r[17]=type;r[18]=0x09001000;r[4]=4;r[31]=0x08804444;
 uint32_t pc=stub;
 for(unsigned step=0;step<30;++step){
  assert(pc>=stub&&pc<stub+56&&!(pc&3));uint32_t w=p->code[(pc-stub)/4];
  unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31;int32_t imm=(int16_t)w;
  if(!w){pc+=4;continue;}
  if(op==9){r[rt]=r[rs]+imm;pc+=4;}
  else if(op==0x0c){r[rt]=r[rs]&(w&0xffff);pc+=4;}
  else if(op==0x25){assert(r[rs]+imm==0x09001048);r[rt]=flags;pc+=4;}
  else if(op==4||op==5){
   assert(p->code[(pc-stub)/4+1]==0);
   int taken=op==4?r[rs]==r[rt]:r[rs]!=r[rt];
   pc=taken?pc+4+imm*4:pc+8;
  }else if(op==2){
   assert(p->code[(pc-stub)/4+1]==0);
   assert(r[17]==type&&r[18]==0x09001000&&r[4]==4&&r[31]==0x08804444);
   return destination(pc,w);
  }else assert(!"unsupported opcode in route stub");
 }
 assert(!"route did not terminate");return 0;
}
int main(void){
 const uint32_t site=0x09254eb8,stub=0x08900000;HidMenuPlan p;
 assert(!hid_menu_plan(site,stub,0x1622fff8,0x24040004,&p));
 assert(destination(site,p.entry)==stub);
 const uint16_t flags[]={0,1,2,4,8,0x402,0x730f,0xffff};unsigned checks=0;
 for(int type=-2;type<13;++type)for(unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);++i){
  int accept=type==1||(type==2&&(flags[i]&2));
  assert(run(&p,stub,(uint32_t)type,flags[i])==(accept?site+8:site-28));++checks;
 }
 assert(hid_menu_plan(site,0x88900000,0x1622fff8,0x24040004,&p)<0);
 assert(hid_menu_plan(site,stub,0x1622fff9,0x24040004,&p)<0);
 assert(hid_menu_plan(site,stub,0x1622fff8,0x24040005,&p)<0);
 assert(hid_menu_plan(site,stub+1,0x1622fff8,0x24040004,&p)<0);
 uint32_t conn[]={0x27bdfff0,0x00a01021};HookPlan h;
 assert(!native_hook_plan(0x88243930,0x88300000,0x88301000,conn,HOOK_HID_CONNECT,0x88239200,&h));
 assert(h.trampoline[0]==conn[0]&&h.trampoline[1]==conn[1]);
 printf("PASS: %u executable MIPS HID-route cases, preserved registers, profile gating and patch guards\n",checks);
}
