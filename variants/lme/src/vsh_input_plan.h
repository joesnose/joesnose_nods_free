#ifndef VSH_INPUT_PLAN_H
#define VSH_INPUT_PLAN_H
#include <stdint.h>
#include <string.h>
#define VSH_INPUT_CONTEXT_WORDS 34
static inline int vsh_input_jump(uint32_t pc,uint32_t target,uint32_t *out){
 if((pc&3)||(target&3)||((pc+4)&0xf0000000u)!=(target&0xf0000000u))return -1;
 *out=0x08000000u|((target>>2)&0x03ffffffu);return 0;
}
/* Accept only the stock export or a direct, aligned kernel-text jump into
 * the resident ARK VshCtrl module. Preserve that exact stub, including ARK's
 * menu/filter chain; no arbitrary J, syscall stub or delay instruction. */
static inline int vsh_input_stub_target(uint32_t base,uint32_t ctrl,
 uint32_t ark_base,uint32_t ark_size,const uint32_t stub[2],uint32_t *target){
 if(!stub||!target||(base&3)||base>0xffffffffu-0x5610u||
    (stub[0]>>26)!=2||stub[1])return -1;
 uint32_t dest=((base+0x560c)&0xf0000000u)|((stub[0]&0x03ffffffu)<<2);
 if(dest==ctrl){*target=dest;return 0;}
 if((ark_base&3)||ark_base<0x88000000u||ark_base>=0x88400000u||
    ark_size<8||ark_base>0xffffffffu-ark_size||ark_size>0x88400000u-ark_base||
    dest<ark_base||dest-ark_base>ark_size-8)return -1;
 *target=dest;return 0;
}
/* One two-argument/16-byte-sample read-positive call. No controller code or
 * import stub is changed. The original stub continues through ARK's chain. */
static inline int vsh_input_plan(uint32_t base,uint32_t size,uint32_t exported,
 uint32_t read_positive,uint32_t hook,const uint32_t context[VSH_INPUT_CONTEXT_WORDS],
 const uint32_t stub[2],uint32_t *patched){
 if(!context||!stub||!patched||(base&3)||base>0xffffffffu-0x5610u||size<0x5610u||exported!=base+0x200)return -1;
 uint32_t intr,call,original_stub,redirect;
 if(vsh_input_jump(base+0x218,base+0x5598,&intr)||vsh_input_jump(base+0x25c,base+0x5608,&call)||
    vsh_input_jump(base+0x5608,read_positive,&original_stub)||vsh_input_jump(base+0x25c,hook,&redirect))return -1;
 const uint32_t expected[VSH_INPUT_CONTEXT_WORDS]={
  0x27bdfff0,0xafb20008,0x00a09021,0xafb10004,0xafb00000,0xafbf000c,
  intr|0x04000000u,0x00808021,0x00123900,0x02072021,0x00903025,0x28440004,
  0x03608821,0x001b2ac0,0x14800013,0x00c71825,0x00a0d821,0x3c098000,
  0x03634024,0x02002021,0x02402821,0x05000004,0x35260023,
  call|0x04000000u,0,0x00403021,0x0220d821,0x8fbf000c,0x8fb20008,
  0x8fb10004,0x8fb00000,0x00c01021,0x03e00008,0x27bd0010
 };
 for(unsigned i=0;i<VSH_INPUT_CONTEXT_WORDS;i++)if(context[i]!=expected[i])return -(int)(16+i);
 if(stub[0]!=original_stub||stub[1]!=0)return -3;
 *patched=redirect|0x04000000u;return 0;
}
/* LME's VshControl patches the CALL at +25c directly, rather than the
 * stock import stub. Verify every other instruction and preserve its callee.
 * Callers must establish that lme_base/size belong to the named VshControl. */
static inline int vsh_input_lme_plan(uint32_t base,uint32_t size,uint32_t exported,
 uint32_t ctrl,uint32_t lme_base,uint32_t lme_size,uint32_t hook,
 const uint32_t context[VSH_INPUT_CONTEXT_WORDS],const uint32_t stub[2],
 uint32_t *original,uint32_t *patched){
 if(!context||!stub||!original||!patched||(context[23]>>26)!=3)return -1;
 uint32_t jump_stub[2]={context[23]&~0x04000000u,0},callee;
 // Reuse strict kernel module bounds; reject stock-target fallback here.
 if(vsh_input_stub_target(base,0,lme_base,lme_size,jump_stub,&callee)||callee==0)return -2;
 uint32_t normalized[VSH_INPUT_CONTEXT_WORDS],stock_call;
 memcpy(normalized,context,sizeof(normalized));
 if(vsh_input_jump(base+0x25c,base+0x5608,&stock_call))return -3;
 normalized[23]=stock_call|0x04000000u;
 int result=vsh_input_plan(base,size,exported,ctrl,hook,normalized,stub,patched);
 if(result)return result;
 *original=callee;return 0;
}
#endif
