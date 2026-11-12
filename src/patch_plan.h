#ifndef PATCH_PLAN_H
#define PATCH_PLAN_H
#include <stdint.h>
typedef struct {uint32_t entry[2],trampoline[4];} HookPlan;
static inline int mips_jump(uint32_t pc,uint32_t target,uint32_t *word){
 if(!word || (pc&3) || (target&3) || pc>0xfffffff8u ||
    ((pc+4)&0xf0000000u)!=(target&0xf0000000u))return -1;
 *word=0x08000000u|((target>>2)&0x03ffffffu);return 0;
}
/* Replace only this verified JAL; its existing argument delay slot remains. */
static inline int slc_call_plan(uint32_t base,uint32_t hook,uint32_t call,
 uint32_t delay,uint32_t following,uint32_t first,uint32_t second,uint32_t *out){
 uint32_t expected,jump;
 if(base>0xffffdfb0u || !out || mips_jump(base+0xa24,base+0x2044,&expected) ||
    call!=(expected|0x04000000u) || delay!=0x27a40004u ||
    following!=0x8fa400f0u || first!=0x27bdffe0u || second!=0xafb20018u ||
    mips_jump(base+0xa24,hook,&jump))return -1;
 *out=jump|0x04000000u;return 0;
}
/* Context order: two argument instructions, JAL/delay, return J/delay,
 * original callee's branch-free first pair. */
static inline int incoming_call_plan(uint32_t base,uint32_t hook,const uint32_t *context,uint32_t *out){
 uint32_t call,after,jump;
 if(!context||!out||base>0xffffd07fu || mips_jump(base+0xc90,base+0x2f78,&call) ||
    mips_jump(base+0xc98,base+0xa30,&after) ||
    context[0]!=0x94e40000u || context[1]!=0x30a5ffffu || context[2]!=(call|0x04000000u) ||
    context[3]!=0 || context[4]!=after || context[5]!=0x8fa400f0u ||
    context[6]!=0x27bdffa0u || context[7]!=0x24030001u || mips_jump(base+0xc90,hook,&jump))return -1;
 *out=jump|0x04000000u;return 0;
}
/* Incoming BSM length includes its four-byte header. The SLC consumer
 * needs three payload halfwords. Never read beyond that defined structure. */
static inline int slc_payload_length(unsigned packet_length){
 return packet_length>=10 && packet_length<=84?(int)packet_length-4:-1;
}
static inline int host_query_allowed(uint32_t base,uint32_t hi,uint32_t lo){
 return base<=0xffff21a4u && (hi&0xffff0000u)==0x3c020000u &&
        (lo&0xffff0000u)==0x90450000u &&
        ((hi&0xffffu)<<16)+(int16_t)lo==base+0xde5bu;
}
static inline int host_address_line(const uint8_t *address,char *out){
 if(!address||!out)return -1;
 unsigned any=0,all=0xff;for(unsigned i=0;i<6;++i){any|=address[i];all&=address[i];}
 if(!any || all==0xff)return -1;
 const char *hex="0123456789ABCDEF";
 for(unsigned i=0;i<6;++i){out[i*3]=hex[address[i]>>4];out[i*3+1]=hex[address[i]&15];out[i*3+2]=i==5?'\n':':';}
 return 0;
}
typedef enum {HOOK_PIN,HOOK_NO_PIN,HOOK_PROFILE,HOOK_HID_CONNECT} HookKind;
/* All permitted displaced instruction pairs are PC-independent and branch-free. */
static inline int native_hook_plan(uint32_t entry,uint32_t hook,uint32_t trampoline,
                                  const uint32_t original[2],HookKind kind,
                                  uint32_t module_base,HookPlan *out){
 if(!original||!out||entry>0xfffffff0u||trampoline>0xfffffff0u)return -1;
 int valid=0;
 if(kind==HOOK_PIN)valid=original[0]==0x27bdfff0u&&original[1]==0x24860054u;
 else if(kind==HOOK_NO_PIN)valid=original[0]==0x24870001u&&original[1]==0x001b12c0u;
 else if(kind==HOOK_HID_CONNECT)valid=original[0]==0x27bdfff0u&&original[1]==0x00a01021u;
 else if(kind==HOOK_PROFILE){
  uint32_t global=((original[0]&0xffffu)<<16)+(int16_t)original[1];
  valid=(original[0]&0xffff0000u)==0x3c070000u&&
        (original[1]&0xffff0000u)==0x90e30000u&&global==module_base+0xde5bu;
 }
 if(!valid)return -1;
 HookPlan p={{0,0},{original[0],original[1],0,0}};
 if(mips_jump(entry,hook,&p.entry[0]) || mips_jump(trampoline+8,entry+8,&p.trampoline[2]))return -1;
 *out=p;return 0;
}
static inline int registration_hook_plan(uint32_t entry,uint32_t hook,uint32_t trampoline,
                                        const uint32_t original[2],HookPlan *out){
 return native_hook_plan(entry,hook,trampoline,original,HOOK_PIN,0,out);
}
typedef struct {uint32_t entry,code[14];} HidMenuPlan;
/* UI s1 is the device type, s2 the already-validated 80-byte profile record.
 * Keep the original type-1 path; admit type 2 only with the HID capability bit.
 * All branch delay slots in this private user-mode stub are NOPs. */
static inline int hid_menu_plan(uint32_t site,uint32_t stub,uint32_t branch,
                                uint32_t delay,HidMenuPlan *out){
 if(!out||site<28||site>0xfffffff0u||stub>0xffffffc0u||
    branch!=0x1622fff8u||delay!=0x24040004u)return -1;
 HidMenuPlan p={0,{0x24020001u,0x12220008u,0,0x24020002u,
  0x16220007u,0,0x96420048u,0x30420002u,0x10400003u,0,0,0,0,0}};
 if(mips_jump(site,stub,&p.entry)||mips_jump(stub+40,site+8,&p.code[10])||
    mips_jump(stub+48,site-28,&p.code[12]))return -1;
 *out=p;return 0;
}
static inline int discovery_patch_allowed(uint32_t load_class,uint32_t compare){
 return load_class==0x240e0005u&&compare==0x104e0003u;
}
#endif
