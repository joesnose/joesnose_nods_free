#include <assert.h>
#include <stdio.h>
#include "patch_plan.h"
int main(void){
 uint32_t ctx[]={0x94e40000,0x30a5ffff,0x0e08f05e,0,0x0a08e70c,0x8fa400f0,0x27bdffa0,0x24030001},word;
 assert(!incoming_call_plan(0x88239200,0x88400000,ctx,&word));assert(word==0x0e100000);
 for(unsigned i=0;i<8;i++){ctx[i]^=1;assert(incoming_call_plan(0x88239200,0x88400000,ctx,&word));ctx[i]^=1;}
 assert(incoming_call_plan(0x88239200,0x08400000,ctx,&word));
 assert(incoming_call_plan(0x88239200,0x88400001,ctx,&word));
 assert(slc_payload_length(10)==6);
 assert(slc_payload_length(14)==10);
 assert(slc_payload_length(84)==80);
 for(unsigned n=0;n<10;n++)assert(slc_payload_length(n)==-1);
 assert(slc_payload_length(85)==-1);
 puts("PASS: incoming call-site guards and inclusive receive-length bounds; ten-byte packet has six payload bytes");
}
