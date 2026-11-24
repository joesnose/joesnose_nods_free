#include "ctrl_entry_plan.h"
#include <assert.h>
#include <stdio.h>
static uint32_t base=0x880e9e00,entries[4],words[4][8],hooks[4],tramp[4];
static void fixture(void){
 for(unsigned i=0;i<4;i++){
  entries[i]=base+0x1ae0+i*32;hooks[i]=0x88300100+i*0x40;tramp[i]=0x88302000+i*16;uint32_t j;
  assert(!vsh_input_jump(entries[i]+12,base+0x1e70,&j));
  uint32_t w[8]={0x27bdfff0,0x3021,0xafbf0000,j|0x04000000u,i?0x24070000u+i:0x3821,0x8fbf0000,0x03e00008,0x27bd0010};memcpy(words[i],w,sizeof(w));
 }
}
static void plain(uint32_t w,uint32_t r[32],uint32_t *saved){
 unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31,rd=(w>>11)&31;
 if(!w)return;
 if(op==9)r[rt]=r[rs]+(int16_t)w;
 else if(op==0&&((w&63)==0x21))r[rd]=r[rs]+r[rt];
 else if(op==0x2b){assert(r[rs]+(int16_t)w==0x09000ff0);*saved=r[rt];}
 else if(op==0x23){assert(r[rs]+(int16_t)w==0x09000ff0);r[rt]=*saved;}
 else assert(!"unsupported instruction");
 r[0]=0;
}
static void execute(unsigned i,int result,const NodsCtrlEntryPlan *p){
 uint32_t r[32]={0},saved=0,pc=tramp[i];unsigned calls=0;
 r[4]=0x09010000;r[5]=3;r[6]=99;r[7]=99;r[29]=0x09001000;r[31]=0x88305000;
 for(unsigned step=0;step<20&&pc!=0x88305000;step++){
  if(pc==base+0x1e70){calls++;assert(r[4]==0x09010000&&r[5]==3&&r[6]==0&&r[7]==i);r[2]=(uint32_t)result;pc=r[31];continue;}
  const uint32_t *list;unsigned offset;
  if(pc>=tramp[i]&&pc<tramp[i]+16){list=p->trampoline[i];offset=(pc-tramp[i])/4;}
  else {assert(pc>=entries[i]+8&&pc<entries[i]+32);list=words[i];offset=(pc-entries[i])/4;}
  uint32_t w=list[offset],op=w>>26,next=pc+4;
  if(op==2||op==3||w==0x03e00008){
   next=w==0x03e00008?r[31]:((pc+4)&0xf0000000u)|((w&0x3ffffff)<<2);
   if(op==3)r[31]=pc+8;
   plain(list[offset+1],r,&saved);pc=next;
  }else{plain(w,r,&saved);pc=next;}
 }
 assert(pc==0x88305000&&r[29]==0x09001000&&r[31]==0x88305000&&r[2]==(uint32_t)result&&calls==1);
}
int main(void){
 fixture();NodsCtrlEntryPlan p;assert(!nods_ctrl_entry_plan(base,0x2850,entries,words,hooks,tramp,&p));
 for(unsigned i=0;i<4;i++){execute(i,1,&p);execute(i,-7,&p);}
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<8;j++){words[i][j]^=1;assert(nods_ctrl_entry_plan(base,0x2850,entries,words,hooks,tramp,&p)<0);words[i][j]^=1;}
 fixture();tramp[0]++;assert(nods_ctrl_entry_plan(base,0x2850,entries,words,hooks,tramp,&p)<0);
 fixture();entries[1]=entries[0];assert(nods_ctrl_entry_plan(base,0x2850,entries,words,hooks,tramp,&p)<0);
 puts("PASS: exact Ctrl entry guards; four trampoline paths execute original once, preserve stack/RA/arguments and original success/error result");
}
