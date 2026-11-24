#include <assert.h>
#include <stdio.h>
#include "ds4_link_setup.h"
int main(void){
 Ds4LinkSetup s={0};
 assert(!ds4_setup_event(&s,0,0)&&!ds4_setup_event(&s,9,0));
 assert(!ds4_setup_take(&s,4,1));assert(ds4_setup_event(&s,4,0));
 assert(!ds4_setup_take(&s,4,0));unsigned first=ds4_setup_take(&s,4,1);
 assert(first&&ds4_setup_current(&s,4,first)&&!ds4_setup_take(&s,4,1));
 assert(!ds4_setup_event(&s,4,0)&&!ds4_setup_take(&s,4,1)); /* duplicate up is not another setup */
 ds4_setup_event(&s,4,3);assert(!ds4_setup_current(&s,4,first));
 assert(ds4_setup_event(&s,4,0));unsigned second=ds4_setup_take(&s,4,1);
 assert(second!=first&&ds4_setup_current(&s,4,second));
 ds4_setup_event(&s,4,2);assert(!ds4_setup_current(&s,4,second));
 assert(ds4_setup_event(&s,4,0));ds4_setup_event(&s,4,3);assert(!ds4_setup_take(&s,4,1));
 ds4_setup_event(&s,1,0);assert(!ds4_setup_take(&s,4,1)&&ds4_setup_take(&s,1,1));
 assert(ds4_setup_wait_bound(1,0x17a,0x17a,0,0)&&ds4_setup_wait_bound(1,0x124,0x124,0,0));
 for(unsigned reply=0;reply<65536;reply++)assert(ds4_setup_wait_bound(1,0x17a,reply,0,0)==(reply==0x17a));
 assert(!ds4_setup_wait_bound(0,0x17a,0x17a,0,0)&&!ds4_setup_wait_bound(1,0x17a,0x17a,1,0)&&!ds4_setup_wait_bound(1,0x17a,0x17a,0,1));
 uint32_t words[DS4_SETUP_GUARDS];ds4_setup_expected(0x88239200,words);
 assert(ds4_setup_context(0x88239200,0xd000,words));
 for(unsigned i=0;i<DS4_SETUP_GUARDS;i++){words[i]^=1;assert(!ds4_setup_context(0x88239200,0xd000,words));words[i]^=1;}
 assert(!ds4_setup_context(0x88239200,0xac0f,words));
 assert(!ds4_setup_context(0xfffff000,0xd000,words));assert(!ds4_setup_context(0x88239201,0xd000,words));
 puts("PASS: one setup per confirmed link generation, duplicate/failed/disconnected links, readiness gating, independent slots, stale command continuation refusal and all33 native ABI guards");
}
