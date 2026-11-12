#include <assert.h>
#include <stdio.h>
#include "../src/patch_plan.h"
int main(void){
 uint32_t b=0x88239200,w=0,call=0x0e08ec91;
 assert(!slc_call_plan(b,0x88400000,call,0x27a40004,0x8fa400f0,0x27bdffe0,0xafb20018,&w));
 assert(w==0x0e100000);
 uint32_t words[]={call,0x27a40004,0x8fa400f0,0x27bdffe0,0xafb20018};
 for(unsigned i=0;i<5;++i){words[i]^=1;
 assert(slc_call_plan(b,0x88400000,words[0],words[1],words[2],words[3],words[4],&w));words[i]^=1;}
 assert(slc_call_plan(b,0x08400000,call,words[1],words[2],words[3],words[4],&w));
 assert(slc_call_plan(b,0x88400001,call,words[1],words[2],words[3],words[4],&w));
 puts("SLC call guard tests passed");return 0;
}
