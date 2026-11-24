#define main predicate_test_main
#include "test_registration_policy.c"
#undef main
static const uint32_t object_address=0x09006000,stack_address=0x09008000;
static unsigned constructor_calls,selector_calls;static int selector_not_pin;
static uint8_t read_byte(const NativePinPlan *p,const uint8_t record[84],uint8_t object[128],uint8_t stack[128],uint32_t a){
 if(a>=ui+0x20b68&&a<ui+0x20b68+56){unsigned off=a-(ui+0x20b68);uint32_t v=off/4==2?ui+(selector_not_pin?0x13934:0x13b28):0;return (uint8_t)(v>>((off%4)*8));}
 if(a>=object_address&&a<object_address+128)return object[a-object_address];
 if(a>=stack_address&&a<stack_address+128)return stack[a-stack_address];
 return byte_at(p,record,a);
}
static void write_byte(uint8_t object[128],uint8_t stack[128],uint32_t a,uint8_t value){
 if(a>=object_address&&a<object_address+128)object[a-object_address]=value;
 else {assert(a>=stack_address&&a<stack_address+128);stack[a-stack_address]=value;}
}
static unsigned run_stub(const NativePinPlan *p,const uint8_t record[84],int allocation_failed,int selector){
 uint8_t object[128],stack[128];memset(object,0xa5,sizeof(object));memset(stack,0xa5,sizeof(stack));
 uint32_t r[32],saved[32];for(unsigned i=0;i<32;i++)r[i]=0x43210000+i;r[0]=0;r[4]=allocation_failed?0:object_address;r[5]=record_address;r[16]=r[4];r[29]=stack_address+64;r[31]=0x08804444;memcpy(saved,r,sizeof(r));
 uint32_t pc=stub+(selector?PIN_SELECTOR_OFFSET:PIN_WRAPPER_OFFSET);constructor_calls=selector_calls=0;
 if(selector){uint32_t table=ui+0x20b68;memcpy(object,&table,4);memcpy(object+4,record,84);memset(object+88,0,32);for(unsigned i=0;i<4;i++)object[88+i*2]=0x30;r[2]=ui+0x1a560;}
 for(unsigned step=0;step<500;step++){
  if(pc==ui+0x13a84){
   assert(r[4]==object_address&&r[5]==record_address);constructor_calls++;
   memcpy(object+4,record,84);memset(object+88,0,32);for(unsigned i=0;i<4;i++)object[88+i*2]=0x30;
   uint32_t table=ui+0x20b68;memcpy(object,&table,4);
   /* Deliberately clobber caller temporaries to exercise wrapper saves. */
   for(unsigned i=2;i<16;i++)r[i]=0xdead0000+i;
   r[2]=0x12345678;pc=r[31];continue;
  }
  if(selector&&pc==ui+0x1a560){selector_calls++;for(unsigned i=16;i<32;i++)assert(r[i]==saved[i]);assert(!constructor_calls);return 1;}
  assert(pc>=stub&&pc<stub+PIN_STUB_BYTES&&!(pc&3));uint32_t w=p->code[(pc-stub)/4];unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31;int imm=(int16_t)w;
  if(!w){pc+=4;continue;}
  if(op==0&&(w==0x03e00008u||w==0x00400008u)){
   uint32_t dest=w==0x00400008u?r[2]:r[31],delay=p->code[(pc-stub)/4+1];assert(delay==0||delay==0x27bd0020u);if(delay)r[29]+=32;
   if(dest==saved[31]){for(unsigned i=16;i<32;i++)assert(r[i]==saved[i]);assert(r[2]==(selector||allocation_failed?0:0x12345678));assert(object[120]==0xa5&&object[127]==0xa5);if(allocation_failed){assert(!constructor_calls);return 0;}assert(constructor_calls==(selector?0:1)&&!memcmp(object+4,record,84));for(unsigned i=0;i<32;i++)assert(object[88+i]==(i<8&&!(i&1)?0x30:0));uint32_t table;memcpy(&table,object,4);return selector?r[2]:table;}
   pc=dest;continue;
  }
  if(op==3){assert(p->code[(pc-stub)/4+1]==0);r[31]=pc+8;pc=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2);continue;}
  if(op==4||op==5){assert(p->code[(pc-stub)/4+1]==0);int take=op==4?r[rs]==r[rt]:r[rs]!=r[rt];pc=take?pc+4+imm*4:pc+8;continue;}
  if(op==0&&w==0x00001021u){r[2]=0;pc+=4;continue;}
  if(op==9)r[rt]=r[rs]+imm;
  else if(op==11)r[rt]=r[rs]<(uint32_t)imm;
  else if(op==12)r[rt]=r[rs]&(w&65535);
  else if(op==13)r[rt]=r[rs]|(w&65535);
  else if(op==15)r[rt]=(w&65535)<<16;
  else if(op==0x2b){uint32_t a=r[rs]+imm;assert(!(a&3));for(unsigned i=0;i<4;i++)write_byte(object,stack,a+i,(uint8_t)(r[rt]>>(8*i)));}
  else if(op==0x23||op==0x25||op==0x24){uint32_t a=r[rs]+imm;unsigned n=op==0x23?4:op==0x25?2:1;assert(!(a&(n-1)));r[rt]=0;for(unsigned i=0;i<n;i++)r[rt]|=(uint32_t)read_byte(p,record,object,stack,a+i)<<(8*i);}
  else assert(!"unknown wrapper opcode");
  pc+=4;
 }
 assert(!"wrapper failed to terminate");return 0;
}
static unsigned wrapper_run(const NativePinPlan *p,const uint8_t record[84],int failed){return run_stub(p,record,failed,0);}
static unsigned selector_run(const NativePinPlan *p,const uint8_t record[84]){return run_stub(p,record,0,1);}
/* Focused reproduction of the verified stock action builder's category
 * routing: audio category8 with only HID0402 yields no action; HID category2
 * reaches the admitted HID connection branch. Empty action count triggers the
 * separately verified cleanup/delete path. */
static unsigned stock_actions(unsigned category,unsigned profile){
 if(category==8)return (profile&4)!=0;
 if(category==2)return (profile&2)!=0;
 return 0;
}
int main(void){
 context_init();uint32_t table[14];const uint32_t offsets[14]={0x1a528,0x1a538,0x13b28,0x13b48,0x1a4d8,0x13b50,0x13958,0x1a508,0x13984,0x1a560,0x1a518,0x1a520,0,0};for(unsigned i=0;i<14;i++)table[i]=offsets[i]?ui+offsets[i]:0;
 uint32_t before[14];memcpy(before,table,sizeof(table));uint32_t category[]={0x03e00008,0x24020002};NativePinPlan p;assert(!native_pin_hybrid_plan(ui,stub,context,table,category,&p));
 assert(!memcmp(table,before,sizeof(table)));for(unsigned i=0;i<14;i++)assert(p.code[PIN_TABLE_OFFSET/4+i]==(i==3?ui+0x13950:table[i]));
 uint8_t record[84];make_record(record,"Zeemote: SteelSeries Free");assert(wrapper_run(&p,record,0)==stub+PIN_TABLE_OFFSET);assert(wrapper_run(&p,record,1)==0);
 make_record(record,"Other");assert(wrapper_run(&p,record,0)==ui+0x20b68);record[72]=32;assert(wrapper_run(&p,record,0)==ui+0x20b68); /* audio shortcut must NOT clone */
 make_record(record,"PLAYSTATION(R)3 Controller");assert(wrapper_run(&p,record,0)==ui+0x20b68);
 make_record(record,"Wireless Controller");assert(wrapper_run(&p,record,0)==stub+PIN_TABLE_OFFSET);assert(wrapper_run(&p,record,1)==0);
 record[74]=0;assert(wrapper_run(&p,record,0)==ui+0x20b68);record[74]=8;record[73]=4;assert(wrapper_run(&p,record,0)==ui+0x20b68);
 make_record(record,"Wireless Controller X");assert(wrapper_run(&p,record,0)==ui+0x20b68);
 make_record(record,"Zeemote: SteelSeries FREE");assert(wrapper_run(&p,record,0)==stub+PIN_TABLE_OFFSET);
 make_record(record,"zEeMoTe: sTeElSeRiEs fReE");record[73]=0;record[74]=0;assert(wrapper_run(&p,record,0)==stub+PIN_TABLE_OFFSET);
 assert(stock_actions(8,0x0402)==0&&stock_actions(2,0x0402)==1);
 for(unsigned i=0;i<14;i++){table[i]^=4;assert(native_pin_hybrid_plan(ui,stub,context,table,category,&p)<0);table[i]^=4;}
 category[1]^=1;assert(native_pin_hybrid_plan(ui,stub,context,table,category,&p)<0);
 /* Execute the DS4-only automatic-default-PIN wrapper, preserving native
  * record/PIN data and all non-DS4/UI routes. */
 category[1]^=1; /* restore after the deliberate mismatch above */
 uint32_t auto_words[AUTO_PIN_GUARDS];native_auto_pin_expected(auto_words);assert(auto_words[1]==0x00001021u); /* actual move v0,zero, not nop */
 NativePinPlan automatic;assert(!native_pin_autopin_plan(ui,stub,context,table,category,auto_words,&automatic));
 make_record(record,"Wireless Controller");assert(wrapper_run(&automatic,record,0)==stub+PIN_DS4_TABLE_OFFSET);assert(wrapper_run(&automatic,record,1)==0);
 make_record(record,"Zeemote: SteelSeries Free");assert(wrapper_run(&automatic,record,0)==stub+PIN_TABLE_OFFSET);
 make_record(record,"Other");assert(wrapper_run(&automatic,record,0)==ui+0x20b68);
 make_record(record,"Wireless Controller X");assert(wrapper_run(&automatic,record,0)==ui+0x20b68);
 make_record(record,"Wireless Controller");record[74]=0;assert(wrapper_run(&automatic,record,0)==ui+0x20b68);
 for(unsigned i=0;i<14;i++)assert(automatic.code[PIN_DS4_TABLE_OFFSET/4+i]==(i==9?ui+0x1a510:automatic.code[PIN_TABLE_OFFSET/4+i]));
 assert(automatic.code[PIN_DS4_TABLE_OFFSET/4+2]==ui+0x13b28); /* genuine PIN registration retained */
 for(unsigned i=0;i<AUTO_PIN_GUARDS;i++){auto_words[i]^=1;assert(native_pin_autopin_plan(ui,stub,context,table,category,auto_words,&automatic)<0);auto_words[i]^=1;}
 native_auto_pin_expected(auto_words);assert(!native_pin_autopin_plan(ui,stub,context,table,category,auto_words,&automatic));
 make_record(record,"DualSense Wireless Controller");assert(wrapper_run(&automatic,record,0)==stub+PIN_DS4_TABLE_OFFSET);assert(selector_run(&automatic,record)==0&&!selector_calls);
 make_record(record,"Wireless Controller");assert(selector_run(&automatic,record)==0&&!selector_calls);
 selector_not_pin=1;assert(selector_run(&automatic,record)==1&&selector_calls==1);selector_not_pin=0;
 make_record(record,"Zeemote: SteelSeries Free");assert(selector_run(&automatic,record)==1&&selector_calls==1);
 make_record(record,"Other");assert(selector_run(&automatic,record)==1&&selector_calls==1);
 make_record(record,"Wireless Controller X");assert(selector_run(&automatic,record)==1&&selector_calls==1);
 make_record(record,"DualSense Wireless Controller");record[0]=80;assert(selector_run(&automatic,record)==1&&selector_calls==1);
 make_record(record,"DualSense Wireless Controller");record[74]=0;assert(selector_run(&automatic,record)==1&&selector_calls==1);
 puts("PASS: executable constructor-wrapper MIPS preserves record84/PIN32, saved ABI and stock return; null/nonmatch/audio/DS3 isolation; clone changes only category; category8 cleanup versus category2 HID action");
}
