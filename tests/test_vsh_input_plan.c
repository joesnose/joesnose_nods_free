#include "vsh_input_plan.h"
#include <assert.h>
#include <stdio.h>
static const uint32_t vb=0x88200000,ctrl_read=0x880e7d20,hook_address=0x88300000;
static void fixture(uint32_t c[34],uint32_t s[2]){
 const uint32_t literal[34]={0x27bdfff0,0xafb20008,0x00a09021,0xafb10004,0xafb00000,0xafbf000c,
  0,0x00808021,0x00123900,0x02072021,0x00903025,0x28440004,0x03608821,0x001b2ac0,
  0x14800013,0x00c71825,0x00a0d821,0x3c098000,0x03634024,0x02002021,0x02402821,
  0x05000004,0x35260023,0,0,0x00403021,0x0220d821,0x8fbf000c,0x8fb20008,
  0x8fb10004,0x8fb00000,0x00c01021,0x03e00008,0x27bd0010};
 memcpy(c,literal,sizeof(literal));uint32_t j;
 assert(!vsh_input_jump(vb+0x218,vb+0x5598,&j));c[6]=j|0x04000000u;
 assert(!vsh_input_jump(vb+0x25c,vb+0x5608,&j));c[23]=j|0x04000000u;
 assert(!vsh_input_jump(vb+0x5608,ctrl_read,&s[0]));s[1]=0;
}
/* Execute the entire bridge wrapper, including pointer/count*16 validation,
 * saved registers and K1 restoration. Native calls are explicit mocks. */
static void execute_bridge(int result,int invalid){
 uint32_t c[34],s[2],patched;fixture(c,s);
 assert(!vsh_input_plan(vb,0x6aa8,vb+0x200,ctrl_read,hook_address,c,s,&patched));c[23]=patched;
 uint32_t r[32]={0},stack[4]={0};r[4]=invalid?0x89000000:0x09000000;r[5]=3;
 r[16]=0xaaaa;r[17]=0xbbbb;r[18]=0xcccc;r[27]=0x00100000;r[29]=0x09010010;r[31]=0x08801234;
 uint32_t pc=vb+0x200;unsigned native_calls=0,hook_calls=0;int returned=0;
 for(unsigned step=0;step<80;step++){
  if(pc==vb+0x5598){r[2]=4;pc=r[31];continue;}
  if(pc==hook_address){
   hook_calls++;assert(r[4]==0x09000000&&r[5]==3);
   /* The unchanged original stub jumps to the current export; that export
    * can retain its existing ARK detour. No call routes back to our hook. */
   assert((s[0]>>26)==2&&s[1]==0);
   assert((((vb+0x560c)&0xf0000000)|((s[0]&0x3ffffff)<<2))==ctrl_read);
   native_calls++;r[2]=(uint32_t)result;pc=r[31];continue;
  }
  if(pc==0x08801234){returned=1;break;}
  assert(pc>=vb+0x200&&pc<=vb+0x284);uint32_t w=c[(pc-vb-0x200)/4];
  unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31,rd=(w>>11)&31,sa=(w>>6)&31,fn=w&63;int32_t imm=(int16_t)w;
  if(op==3||w==0x03e00008||op==1||op==5){
   uint32_t delay=c[(pc-vb-0x200)/4+1],next=pc+8;
   if(op==3){r[31]=pc+8;next=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2);}
   else if(w==0x03e00008)next=r[31];
   else if((op==1&&(int32_t)r[rs]<0)||(op==5&&r[rs]!=r[rt]))next=pc+4+imm*4;
   if(delay==0x00808021)r[16]=r[4];
   else if(delay==0x00c71825)r[3]=r[6]|r[7];
   else if(delay==0x35260023)r[6]=r[9]|0x23;
   else if(delay==0x27bd0010)r[29]+=16;
   else assert(delay==0);
   pc=next;continue;
  }
  if(w==0){}
  else if(op==9)r[rt]=r[rs]+imm;
  else if(op==10)r[rt]=(int32_t)r[rs]<imm;
  else if(op==15)r[rt]=(w&65535)<<16;
  else if(op==0x2b||op==0x23){uint32_t a=r[rs]+imm;assert(a>=0x09010000&&a<=0x0901000c&&!(a&3));if(op==0x2b)stack[(a-0x09010000)/4]=r[rt];else r[rt]=stack[(a-0x09010000)/4];}
  else if(op==0){if(fn==0)r[rd]=r[rt]<<sa;else if(fn==0x21)r[rd]=r[rs]+r[rt];else if(fn==0x25)r[rd]=r[rs]|r[rt];else if(fn==0x24)r[rd]=r[rs]&r[rt];else assert(!"unknown special");}
  else assert(!"unknown bridge instruction");
  r[0]=0;pc+=4;
 }
 assert(returned&&r[16]==0xaaaa&&r[17]==0xbbbb&&r[18]==0xcccc&&r[27]==0x00100000&&r[29]==0x09010010);
 assert(hook_calls==(invalid?0u:1u)&&native_calls==(invalid?0u:1u));
 assert(r[2]==(invalid?0x80000023u:(uint32_t)result));
}
int main(void){
 uint32_t st[2],dest,j;
 assert(!vsh_input_jump(vb+0x5608,ctrl_read,&j));st[0]=j;st[1]=0;
 assert(!vsh_input_stub_target(vb,ctrl_read,0,0,st,&dest)&&dest==ctrl_read);
 assert(!vsh_input_jump(vb+0x5608,0x88340100,&j));st[0]=j;
 assert(!vsh_input_stub_target(vb,ctrl_read,0x88340000,0x8000,st,&dest)&&dest==0x88340100);
 assert(vsh_input_stub_target(vb,ctrl_read,0,0,st,&dest)<0);
 assert(vsh_input_stub_target(vb,ctrl_read,0x88340200,0x8000,st,&dest)<0);
 assert(vsh_input_stub_target(vb,ctrl_read,0x08340000,0x8000,st,&dest)<0);
 assert(vsh_input_stub_target(vb,ctrl_read,0x88340000,0xffffffff,st,&dest)<0);
 st[1]=1;assert(vsh_input_stub_target(vb,ctrl_read,0x88340000,0x8000,st,&dest)<0);
 st[1]=0;st[0]|=0x04000000;assert(vsh_input_stub_target(vb,ctrl_read,0x88340000,0x8000,st,&dest)<0);

 uint32_t c[34],s[2],patched;fixture(c,s);assert(!vsh_input_plan(vb,0x6aa8,vb+0x200,ctrl_read,hook_address,c,s,&patched));
 for(unsigned i=0;i<34;i++){c[i]^=1;assert(vsh_input_plan(vb,0x6aa8,vb+0x200,ctrl_read,hook_address,c,s,&patched)<0);c[i]^=1;}
 for(unsigned i=0;i<2;i++){s[i]^=1;assert(vsh_input_plan(vb,0x6aa8,vb+0x200,ctrl_read,hook_address,c,s,&patched)<0);s[i]^=1;}
 execute_bridge(1,0);execute_bridge(0,0);execute_bridge(-7,0);execute_bridge(1,1);
 puts("PASS: full VSH ABI/context and unchanged import guards; emitted call executes whole bridge, count*16 validation, K1/stack/callee-register restoration and once-only current-export chain; original errors preserved");
 return 0;
}
