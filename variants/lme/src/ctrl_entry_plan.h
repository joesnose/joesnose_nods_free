/* Exact Sony two-argument Ctrl wrapper plan; no writes in this helper. */
#ifndef NODS_CTRL_ENTRY_PLAN_H
#define NODS_CTRL_ENTRY_PLAN_H
#include "vsh_input_plan.h"
typedef struct {uint32_t patch[4],trampoline[4][4];} NodsCtrlEntryPlan;
static inline int nods_ctrl_entry_plan(uint32_t base,uint32_t size,const uint32_t entry[4],const uint32_t words[4][8],const uint32_t hook[4],const uint32_t trampoline[4],NodsCtrlEntryPlan *out){
 if(!entry||!words||!hook||!trampoline||!out||(base&3)||base<0x88000000u||base>=0x88400000u||size<32||size>0x88400000u-base)return -1;
 uint32_t common=0;
 for(unsigned i=0;i<4;i++){
  uint32_t a=entry[i];if((a&3)||a<base||a-base>size-32)return -2;
  for(unsigned j=0;j<i;j++)if(a<entry[j]+32&&entry[j]<a+32)return -3;
  const uint32_t *w=words[i];uint32_t delay=i?0x24070000u+i:0x00003821u;
  if(w[0]!=0x27bdfff0u||w[1]!=0x00003021u||w[2]!=0xafbf0000u||(w[3]>>26)!=3||w[4]!=delay||w[5]!=0x8fbf0000u||w[6]!=0x03e00008u||w[7]!=0x27bd0010u)return -4;
  uint32_t callee=((a+16)&0xf0000000u)|((w[3]&0x03ffffffu)<<2);
  if((callee&3)||callee<base||callee-base>size-8||(i&&callee!=common))return -5;
  common=callee;
  if(hook[i]<0x88000000u||hook[i]>=0x88400000u||trampoline[i]<0x88000000u||trampoline[i]>0x883ffff0u||(trampoline[i]&15))return -6;
  if(vsh_input_jump(a,hook[i],&out->patch[i])||vsh_input_jump(trampoline[i]+8,a+8,&out->trampoline[i][2]))return -7;
  out->trampoline[i][0]=w[0];out->trampoline[i][1]=w[1];out->trampoline[i][3]=0;
 }
 return 0;
}
#endif
