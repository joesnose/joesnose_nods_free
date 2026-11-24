#ifndef REGISTRATION_POLICY_H
#define REGISTRATION_POLICY_H
#include <stdint.h>
#include <string.h>
#define PIN_STUB_WORDS 272
#define PIN_WRAPPER_OFFSET 576
#define PIN_TABLE_OFFSET 768
#define PIN_DS4_TABLE_OFFSET 832
#define PIN_SELECTOR_OFFSET 896
#define PIN_STUB_BYTES (PIN_STUB_WORDS*4)
#define PIN_FREE_NAME_OFFSET 384
#define PIN_DS4_NAME_OFFSET 448
#define PIN_DS5_NAME_OFFSET 512
_Static_assert(sizeof("zeemote: steelseries free")*2<=PIN_DS4_NAME_OFFSET-PIN_FREE_NAME_OFFSET,"Free UTF16 name bound");
_Static_assert(sizeof("Wireless Controller")*2<=PIN_DS5_NAME_OFFSET-PIN_DS4_NAME_OFFSET,"DS4 UTF16 name bound");
_Static_assert(PIN_WRAPPER_OFFSET+27*4<=PIN_TABLE_OFFSET,"constructor wrapper bound");
_Static_assert(PIN_TABLE_OFFSET+14*4<=PIN_DS4_TABLE_OFFSET,"private table bound");
_Static_assert(PIN_DS4_TABLE_OFFSET+14*4<=PIN_STUB_BYTES,"DS4 table bound");
_Static_assert(PIN_WRAPPER_OFFSET+37*4<=PIN_TABLE_OFFSET,"automatic PIN wrapper bound");
_Static_assert(sizeof("DualSense Wireless Controller")*2<=PIN_WRAPPER_OFFSET-PIN_DS5_NAME_OFFSET,"DS5 UTF16 bound");
typedef struct {uint32_t entry,constructor_entry,pin_zero_delay,selector_entry,code[PIN_STUB_WORDS];} NativePinPlan;
/* One verified UI call site only. All offsets are from the examined UI module.
 * Free keeps its ASCII-case-insensitive exact-name exception without a CoD requirement.
 * Wireless Controller restores v0.14/v0.15 exact UTF16 name + peripheral major5/minor8.
 * These discovery heuristics select a native route, not authenticated identity. */
static inline int native_pin_context(uint32_t base,const uint32_t c[13]){
 uint32_t call,ctor,alloc,memset_call,branch;
 if(!c||base>0xfffdf48fu||mips_jump(base+0x13f1c,base+0x1427c,&call)||
 mips_jump(base+0x13f3c,base+0x13a84,&ctor)||mips_jump(base+0x13f2c,base+0x1abe0,&alloc)||
 mips_jump(base+0x13af8,base+0x1a9b8,&memset_call)||mips_jump(base+0x13f44,base+0x13f00,&branch))return 0;
 return c[0]==(call|0x04000000u)&&c[1]==0x02002821u&&c[2]==0x10400009u&&c[3]==0x02202021u&&
 c[4]==(alloc|0x04000000u)&&c[5]==0x24040078u&&c[6]==(ctor|0x04000000u)&&c[7]==0x00408021u&&
 c[8]==branch&&c[9]==0x02001821u&&c[10]==(memset_call|0x04000000u)&&c[11]==0x24060010u&&c[12]==base+0x13b28;
}
static inline int native_pin_plan(uint32_t base,uint32_t stub,const uint32_t context[13],NativePinPlan *out){
 if(!out||!native_pin_context(base,context)||stub>0xffffffffu-PIN_STUB_BYTES||(stub&3))return -1;
 uint32_t j;
 if(mips_jump(base+0x13f1c,stub,&j))return -1;
 memset(out,0,sizeof(*out));
 out->entry=j|0x04000000u;
 /* Sole examined constructor caller allocates120 bytes; PIN at+88 fits32. */
 out->pin_zero_delay=0x24060020u;
 /* v0 reproduces the original predicate. Other written registers are caller
  * temporaries. a0/a1, saved registers, sp and ra remain untouched. */
 /* Instruction labels: 0 stock predicate; 4 strict record check; 14 Free
  * folded-name loop; 29 restored DS4 class gates; 41 exact DS4 name loop;
  * 51 accept; 54 reject. Constructor rechecks from label4, skipping the stock
  * audio shortcut before assigning our private HID category. */
 static const uint32_t code[]={
  0x90a20000u,0x30420020u,0x14400030u,0x00000000u, /* 0 */
  0x8ca3ffb8u,0x24080054u,0x1468002fu,0x00000000u, /* 4 */
  0x94a3ffbcu,0x1060002cu,0x00000000u,0x24a8ffbeu, /* 8 */
  0x00000000u,0x00000000u,0x950a0000u,0x952b0000u, /* 12 */
  0x2543ffbfu,0x2c63001au,0x10600002u,0x00000000u, /* 16 */
  0x354a0020u,0x154b0007u,0x00000000u,0x1140001bu, /* 20 */
  0x00000000u,0x25080002u,0x25290002u,0x1000fff2u, /* 24 */
  0x00000000u,0x90a30002u,0x24080008u,0x14680016u, /* 28 */
  0x00000000u,0x90a30001u,0x3063001fu,0x24080005u, /* 32 */
  0x14680011u,0x00000000u,0x24a8ffbeu,0x00000000u, /* 36 */
  0x00000000u,0x950a0000u,0x952b0000u,0x154b000au, /* 40 */
  0x00000000u,0x11400005u,0x00000000u,0x25080002u, /* 44 */
  0x25290002u,0x1000fff7u,0x00000000u,0x24020001u, /* 48 */
  0x03e00008u,0x00000000u,0x00001021u,0x03e00008u, /* 52 */
  0x00000000u, /* 56 */
  0x8ca3ffb8u,0x24080054u,0x1468001au,0, /*57: strict DS5 size */
  0x94a3ffbcu,0x10600017u,0,
  0x90a30002u,0x24080008u,0x14680013u,0,
  0x90a30001u,0x3063001fu,0x24080005u,0x1468000eu,0,
  0x24a8ffbeu,0,0,0x950a0000u,0x952b0000u,0x154b0007u,0,
  0x1140ffe2u,0,0x25080002u,0x25290002u,0x1000fff7u,0,
  0x00001021u,0x03e00008u,0
 };
 _Static_assert(sizeof(code)<=PIN_FREE_NAME_OFFSET,"predicate code before names");
 uint32_t free_name=stub+PIN_FREE_NAME_OFFSET,ds4_name=stub+PIN_DS4_NAME_OFFSET;
 uint32_t ds5_name=stub+PIN_DS5_NAME_OFFSET;
 memcpy(out->code,code,sizeof(code));
 out->code[54]=0x10000002u;out->code[55]=0;
 out->code[12]=0x3c090000u|(free_name>>16);out->code[13]=0x35290000u|(free_name&0xffff);
 out->code[39]=0x3c090000u|(ds4_name>>16);out->code[40]=0x35290000u|(ds4_name&0xffff);
 out->code[74]=0x3c090000u|(ds5_name>>16);out->code[75]=0x35290000u|(ds5_name&0xffff);
 const char *names[3]={"zeemote: steelseries free","Wireless Controller","DualSense Wireless Controller"};
 const unsigned offsets[3]={PIN_FREE_NAME_OFFSET,PIN_DS4_NAME_OFFSET,PIN_DS5_NAME_OFFSET};
 for(unsigned n=0;n<3;n++)for(unsigned i=0;names[n][i];i++){
  unsigned byte=offsets[n]+i*2;out->code[byte/4]|=(uint32_t)(uint8_t)names[n][i]<<((byte%4)*8);
 }
 return 0;
}
/* Clone the PIN object's methods; only its category callback changes. */
static inline int native_pin_hybrid_plan(uint32_t base,uint32_t stub,const uint32_t context[13],
 const uint32_t table[14],const uint32_t category[2],NativePinPlan *out){
 static const uint32_t offsets[14]={0x1a528,0x1a538,0x13b28,0x13b48,0x1a4d8,0x13b50,0x13958,0x1a508,0x13984,0x1a560,0x1a518,0x1a520,0,0};
 if(base>0xffffffffu-0x20ba0u||!table||!category||!out||category[0]!=0x03e00008u||category[1]!=0x24020002u)return -1;
 for(unsigned i=0;i<14;i++)if(table[i]!=(offsets[i]?base+offsets[i]:0))return -1;
 if(native_pin_plan(base,stub,context,out))return -1;
 uint32_t target;
 if(mips_jump(base+0x13f3c,stub+PIN_WRAPPER_OFFSET,&target))return -1;
 out->constructor_entry=target|0x04000000u;
 uint32_t ctor,strict;
 if(mips_jump(stub+PIN_WRAPPER_OFFSET+6*4,base+0x13a84,&ctor)||
    mips_jump(stub+PIN_WRAPPER_OFFSET+11*4,stub+16,&strict))return -1;
 uint32_t clone=stub+PIN_TABLE_OFFSET;
 uint32_t wrapper[]={
  0x27bdffe0u,0xafbf001cu,0xafa40018u,0xafa50014u,
  0x10800012u,0, /* null allocation -> guarded return without constructor */
  ctor|0x04000000u,0,0xafa20010u,0x8fa50014u,0x24a50048u,
  strict|0x04000000u,0,0x10400005u,0, /* strict match, not audio shortcut */
  0x8fa40018u,0x3c030000u|(clone>>16),0x34630000u|(clone&0xffff),0xac830000u,
  0x8fa20010u,0x8fbf001cu,0x03e00008u,0x27bd0020u,
  0x00001021u,0x8fbf001cu,0x03e00008u,0x27bd0020u
 };
 memcpy(&out->code[PIN_WRAPPER_OFFSET/4],wrapper,sizeof(wrapper));
 memcpy(&out->code[PIN_TABLE_OFFSET/4],table,14*4);
 out->code[PIN_TABLE_OFFSET/4+3]=base+0x13950;
 return 0;
}
/* Preserve the working native PIN object, including its default UTF16 0000.
 * Only the exact, already-admitted DS4 object suppresses the entry screen. */
#define AUTO_PIN_GUARDS 13
static const unsigned auto_pin_offsets[AUTO_PIN_GUARDS]={0x1a510,0x1a514,0x13b00,0x13b04,0x13b08,0x13b0c,0x13b10,0x13b14,0x2588,0x2590,0x2594,0x2598,0x259c};
static inline void native_auto_pin_expected(uint32_t *w){
 const uint32_t e[AUTO_PIN_GUARDS]={0x03e00008,0x00001021,0x24030030,0x24020003,0x2442ffff,0xa6030000,0x0441fffd,0x26100002,0x8ea30000,0x8c620024,0x0040f809,0x02a02021,0x10400024};
 memcpy(w,e,sizeof(e));
}
static inline int native_auto_pin_context(const uint32_t *w){
 if(!w)return 0;
 uint32_t e[AUTO_PIN_GUARDS];native_auto_pin_expected(e);return !memcmp(w,e,sizeof(e));
}
static inline int native_pin_autopin_plan(uint32_t base,uint32_t stub,const uint32_t context[13],const uint32_t table[14],const uint32_t category[2],const uint32_t auto_words[AUTO_PIN_GUARDS],NativePinPlan *out){
 if(!native_auto_pin_context(auto_words)||native_pin_hybrid_plan(base,stub,context,table,category,out))return -1;
 uint32_t ctor,strict;
 if(mips_jump(stub+PIN_WRAPPER_OFFSET+6*4,base+0x13a84,&ctor)||mips_jump(stub+PIN_WRAPPER_OFFSET+11*4,stub+16,&strict))return -1;
 uint32_t free_table=stub+PIN_TABLE_OFFSET,ds4_table=stub+PIN_DS4_TABLE_OFFSET;
 uint32_t wrapper[]={
  0x27bdffe0,0xafbf001c,0xafa40018,0xafa50014,
  0x1080001c,0,ctor|0x04000000u,0,0xafa20010,0x8fa50014,0x24a50048,strict|0x04000000u,0,
  0x1040000f,0,0x8fa40018,
  0x3c030000u|(free_table>>16),0x34630000u|(free_table&65535),
  0x9488000a,0x24090057,0x11090005,0,0x24090044,0x15090004,0,0,
  0x3c030000u|(ds4_table>>16),0x34630000u|(ds4_table&65535),0xac830000,
  0x8fa20010,0x8fbf001c,0x03e00008,0x27bd0020,
  0x00001021,0x8fbf001c,0x03e00008,0x27bd0020
 };
 /* The first-character discriminator is reached only after the full strict
  * record/name/class check admitted exactly Free or Wireless Controller. */
 memcpy(&out->code[PIN_WRAPPER_OFFSET/4],wrapper,sizeof(wrapper));
 memcpy(&out->code[PIN_DS4_TABLE_OFFSET/4],&out->code[PIN_TABLE_OFFSET/4],14*4);
 out->code[PIN_DS4_TABLE_OFFSET/4+9]=base+0x1a510;
 uint32_t selector_jump,pin_register=base+0x13b28;
 if(mips_jump(base+0x2594,stub+PIN_SELECTOR_OFFSET,&selector_jump))return -1;
 out->selector_entry=selector_jump|0x04000000u;
 uint32_t selector[]={
  0x27bdffe0,0xafbf001c,0xafa40018,0xafa20014,
  0x8c880000,0x8d090008,0x3c0a0000u|(pin_register>>16),0x354a0000u|(pin_register&65535),
  0x152a0012,0,0x2485004c,0,0,0x1040000d,0,0x8fa40018,
  0x9488000a,0x24090057,0x11090004,0,0x24090044,0x15090005,0,
  0x00001021,0x8fbf001c,0x03e00008,0x27bd0020,
  0x8fa40018,0x8fa20014,0x8fbf001c,0x00400008,0x27bd0020
 };
 uint32_t strict_call;if(mips_jump(stub+PIN_SELECTOR_OFFSET+11*4,stub+16,&strict_call))return -1;
 selector[11]=strict_call|0x04000000u;
 memcpy(&out->code[PIN_SELECTOR_OFFSET/4],selector,sizeof(selector));
 _Static_assert(PIN_SELECTOR_OFFSET+sizeof(selector)<=PIN_STUB_BYTES,"PIN selector bound");
 return 0;
}
#endif
