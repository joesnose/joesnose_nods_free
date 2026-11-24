#include "patch_plan.h"
#include "registration_policy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const uint32_t ui=0x09100000,stub=0x08900000,record_address=0x09004000;
static uint32_t context[13];
static uint32_t call(uint32_t off,uint32_t target){uint32_t w;assert(!mips_jump(ui+off,ui+target,&w));return w|0x04000000u;}
static void context_init(void){
 uint32_t c[]={call(0x13f1c,0x1427c),0x02002821,0x10400009,0x02202021,call(0x13f2c,0x1abe0),0x24040078,call(0x13f3c,0x13a84),0x00408021,0,0x02001821,call(0x13af8,0x1a9b8),0x24060010,ui+0x13b28};
 assert(!mips_jump(ui+0x13f44,ui+0x13f00,&c[8]));memcpy(context,c,sizeof(c));
}
static uint8_t byte_at(const NativePinPlan *p,const uint8_t record[84],uint32_t a){
 if(a>=record_address&&a<record_address+84)return record[a-record_address];
 assert(a>=stub&&a<stub+PIN_STUB_BYTES);unsigned off=a-stub;return(uint8_t)(p->code[off/4]>>((off%4)*8));
}
static unsigned run(const NativePinPlan *p,const uint8_t record[84]){
 uint32_t r[32],before[32];for(unsigned i=0;i<32;i++)r[i]=0x12340000+i;r[0]=0;r[5]=record_address+72;r[31]=0x08808888;memcpy(before,r,sizeof(r));
 uint32_t pc=stub;
 for(unsigned steps=0;steps<800;steps++){
  assert(pc>=stub&&pc<stub+PIN_FREE_NAME_OFFSET&&!(pc&3));uint32_t w=p->code[(pc-stub)/4];unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31;int imm=(int16_t)w;
  if(!w){pc+=4;continue;}
  if(op==3)assert(!"leaf predicate unexpectedly calls native code");
  if(op==0&&w==0x03e00008){assert(p->code[(pc-stub)/4+1]==0);for(unsigned i=0;i<32;i++)if(i!=2&&i!=3&&i!=8&&i!=9&&i!=10&&i!=11)assert(r[i]==before[i]);assert(r[2]<=1);return r[2];}
  if(op==4||op==5){assert(p->code[(pc-stub)/4+1]==0);int take=op==4?r[rs]==r[rt]:r[rs]!=r[rt];pc=take?pc+4+imm*4:pc+8;continue;}
  if(op==0&&w==0x00001021){r[2]=0;pc+=4;continue;}
  if(op==9)r[rt]=r[rs]+imm;
  else if(op==11)r[rt]=r[rs]<(uint32_t)imm;
  else if(op==12)r[rt]=r[rs]&(w&65535);
  else if(op==13)r[rt]=r[rs]|(w&65535);
  else if(op==15)r[rt]=(w&65535)<<16;
  else if(op==0x23||op==0x25||op==0x24){uint32_t a=r[rs]+imm;unsigned n=op==0x23?4:op==0x25?2:1;assert(!(a&(n-1)));r[rt]=0;for(unsigned i=0;i<n;i++)r[rt]|=(uint32_t)byte_at(p,record,a+i)<<(8*i);}
  else assert(!"unknown instruction");
  pc+=4;
 }
 assert(!"predicate did not return");return 0;
}
/* Observed native assembler at BSMan+6500: one high byte plus a LE
 * halfword, serialized into the UI record in high/middle/low order. */
static void native_discovery_class(uint8_t record[84],uint8_t high,const uint8_t low_word_wire[2]){
 unsigned word=(unsigned)low_word_wire[0]|((unsigned)low_word_wire[1]<<8);
 record[72]=high;record[73]=(uint8_t)(word>>8);record[74]=(uint8_t)word;
}
static void make_record(uint8_t record[84],const char *name){memset(record,0,84);record[0]=84;record[4]=7;for(unsigned i=0;name[i]&&i<32;i++)record[6+2*i]=(uint8_t)name[i];const uint8_t wire_low_word[]={0x08,0x25};native_discovery_class(record,0,wire_low_word);}
int main(void){
 context_init();NativePinPlan p;assert(!native_pin_plan(ui,stub,context,&p));assert(p.entry==call(0x13f1c,stub-ui));
 assert(p.pin_zero_delay==0x24060020u);
 /* Stock object is120 bytes: record84 at+4; full PIN32 at+88.
  * Check corrected memset covers all transmitted bytes without object overrun. */
 uint8_t object[122];memset(object,0xa5,sizeof(object));memset(object+1+88,0,p.pin_zero_delay&0xffff);
 for(unsigned i=88;i<120;i++)assert(object[i+1]==0);
 assert(object[0]==0xa5&&object[121]==0xa5&&object[88]==0xa5);
 /* Native discovery assembler +6500 writes CoD as high byte then the
  * high/low bytes of its remaining native halfword. Reproduce that boundary,
  * independently of the predicate's byte loads. */
 uint8_t r[84],saved[84];make_record(r,"Zeemote: SteelSeries Free");memcpy(saved,r,84);assert(r[72]==0&&r[73]==0x25&&r[74]==8);assert(run(&p,r)==1);assert(!memcmp(saved,r,84));
 unsigned checks=0;for(unsigned low=0;low<256;low++)for(unsigned major=0;major<256;major++){uint8_t wire[]={low,major};native_discovery_class(r,0,wire);unsigned expected=1;assert(run(&p,r)==expected);checks++;}
 /* No undocumented CoD value is required for this explicitly named target. */
 make_record(r,"Zeemote: SteelSeries Free");r[72]=8;r[74]=0;assert(run(&p,r));
 /* All original audio-service predicate cases still return true, regardless
  * of low class bits or name; other high-service bytes must not force PIN. */
 for(unsigned high=0;high<256;high++){make_record(r,"Other");r[72]=high;assert(run(&p,r)==((high&32)!=0));}
 const char *others[]={"PLAYSTATION(R)3 Controller","Zeemote: SteelSeries Free X","Zeemote: SteelSeries Frees","Zeemote: SteelSeries Fre","X Zeemote: SteelSeries Free","","Xbox Wireless Controller"};
 for(unsigned i=0;i<sizeof(others)/sizeof(others[0]);i++){make_record(r,others[i]);assert(run(&p,r)==0);r[72]=32;assert(run(&p,r)==1);}
 const char *cases[]={"zeemote: steelseries free","Zeemote: SteelSeries FREE","ZEEMOTE: STEELSERIES FREE","zEeMoTe: sTeElSeRiEs fReE"};
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){make_record(r,cases[i]);assert(run(&p,r)==1);}
 make_record(r,"Zeemote: SteelSeries Free");r[6]=0x7a;r[7]=1;assert(!run(&p,r)); /* not ASCII Z */
 make_record(r,"Zeemote: SteelSeries Free");for(unsigned i=6;i<70;i+=2){r[i+1]=r[i];r[i]=0;}assert(!run(&p,r)); /* wrong UTF16 endian */
 make_record(r,"Zeemote: SteelSeries Free");r[6]=0x3a;assert(!run(&p,r)); /* ':' cannot fold into Z */
 make_record(r,"Zeemote: SteelSeries Free");r[0]=80;assert(!run(&p,r));r[0]=84;r[4]=0;assert(!run(&p,r));
 /* Restore the v0.14/v0.15 DS4 discovery rule in addition to Free. */
 make_record(r,"Wireless Controller");memcpy(saved,r,84);assert(run(&p,r)==1&&!memcmp(saved,r,84));
 for(unsigned low=0;low<256;low++)for(unsigned major=0;major<256;major++){
  uint8_t wire[]={low,major};native_discovery_class(r,0,wire);assert(run(&p,r)==(low==8&&(major&31)==5));
 }
 const char *wireless_others[]={"wireless controller","WIRELESS CONTROLLER","Wireless Controller X","X Wireless Controller","Wireless Controlle","Wireless ControllerX"};
 for(unsigned i=0;i<sizeof(wireless_others)/sizeof(wireless_others[0]);i++){make_record(r,wireless_others[i]);assert(!run(&p,r));}
 make_record(r,"Wireless Controller");r[0]=80;assert(!run(&p,r));r[0]=84;r[4]=0;assert(!run(&p,r));
 make_record(r,"Wireless Controller");r[7]=1;assert(!run(&p,r)); /* Unicode lookalike */
 make_record(r,"Wireless Controller");r[73]=8;r[74]=0x25;assert(!run(&p,r)); /* reversed CoD */
 for(unsigned high=0;high<256;high++){make_record(r,"Wireless Controller");r[72]=high;r[74]=0;assert(run(&p,r)==!!(high&32));}
 for(unsigned i=0;i<13;i++){uint32_t old=context[i];context[i]^=1;assert(native_pin_plan(ui,stub,context,&p)<0);context[i]=old;}
 assert(native_pin_plan(ui,stub+1,context,&p)<0);assert(native_pin_plan(ui,0x88900000,context,&p)<0);
 make_record(r,"DualSense Wireless Controller");assert(run(&p,r)==1);r[0]=80;assert(run(&p,r)==0);r[0]=84;r[74]=0;assert(run(&p,r)==0);
 make_record(r,"DualSense Wireless Controller X");assert(!run(&p,r));
 printf("PASS: %u executable MIPS CoD-independence cases, ASCII-case-insensitive exact UTF16 name/NUL and real-record gating, restored DS4 exact-name/class rule, original predicate preservation, unchanged record/registers and all context guards\n",checks);
 return 0;
}
