#include <assert.h>
#include <stdio.h>
#include "ds4_country.h"
int main(void){
 uint8_t out[16];memset(out,0xa5,sizeof(out));
 assert(ds4_country_attribute(4,0x054c,0x05c4,0x1124,0x203,out,5,0,0)==5);
 const uint8_t expected[5]={9,2,3,8,0};assert(!memcmp(out,expected,5)&&out[5]==0xa5);
 /* Native consumer loads byte4 after the attributed uint8, not a sequence. */
 assert(out[4]==0);
 for(unsigned v=0;v<65536;v++){
  assert((ds4_country_attribute(4,v,0x05c4,0x1124,0x203,out,16,0,0)>0)==(v==0x054c));
  assert((ds4_country_attribute(4,0x054c,v,0x1124,0x203,out,16,0,0)>0)==(v==0x05c4||v==0x09cc||v==0x0ce6));
  assert((ds4_country_attribute(4,0x054c,0x05c4,0x1124,v,out,16,0,0)>0)==(v==0x203));
 }
 for(unsigned cap=0;cap<5;cap++){
  memset(out,0xa5,sizeof(out));assert(!ds4_country_attribute(4,0x054c,0x09cc,0x1124,0x203,out,cap,0,0));
  for(unsigned i=0;i<sizeof(out);i++)assert(out[i]==0xa5);
 }
 assert(!ds4_country_attribute(0,0x054c,0x05c4,0x1124,0x203,out,16,0,0));
 assert(!ds4_country_attribute(9,0x054c,0x05c4,0x1124,0x203,out,16,0,0));
 assert(!ds4_country_attribute(4,0x054c,0x0268,0x1124,0x203,out,16,0,0));
 assert(ds4_country_attribute(4,0x054c,0x0ce6,0x1124,0x203,out,16,0,0)==5);
 assert(!ds4_country_attribute(4,0x2dc8,0x3820,0x1124,0x203,out,16,0,0));
 assert(!ds4_country_attribute(4,0x054c,0x05c4,0x1200,0x203,out,16,0,0));
 assert(!ds4_country_attribute(4,0x054c,0x05c4,0x1124,0x203,NULL,16,0,0));
 assert(!ds4_country_attribute(4,0x054c,0x05c4,0x1124,0x203,out,16,1,0));
 assert(!ds4_country_attribute(4,0x054c,0x05c4,0x1124,0x203,out,16,0,1));
 uint32_t words[DS4_COUNTRY_GUARDS]={0x02002021,0x24051124,0x24060203,0x03a03821,0x24080205,0x02404821,0x0e08f9f8,0x00005021,0x0440003d,0x93a30004};
 assert(ds4_country_context(0x88239200,0xd000,0x88401000,words));
 for(unsigned i=0;i<DS4_COUNTRY_GUARDS;i++){words[i]^=1;assert(!ds4_country_context(0x88239200,0xd000,0x88401000,words));words[i]^=1;}
 assert(!ds4_country_context(0x88239201,0xd000,0x88401000,words));
 assert(!ds4_country_context(0x88239200,0x9d87,0x88401000,words));
 assert(!ds4_country_context(0xfffff000,0xd000,0x88401000,words));
 assert(!ds4_country_context(0x88239200,0xd000,0x08401000,words));
 assert(!ds4_country_context(0x88239200,0xd000,0x88401001,words));
 assert(!ds4_country_context(0x88239200,0xd000,0x88401000,NULL));
 puts("PASS: DS4 exact identity/slot/service/country-attribute/default encoding/capacity/mode/timeout policy, non-DS4 isolation and all10 relocated native call/consumer guards");
}
