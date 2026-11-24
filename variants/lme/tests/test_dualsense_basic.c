#include <assert.h>
#include <stdio.h>
#include "../src/zeemote_hid.c"
#include "../src/ds4_basic.c"
#include "ds4_local.h"
int main(void){
 uint8_t report[10]={1,0x7d,0x7e,0x83,0x82,8,0,0,0,0};ZeeState state;
 assert(!ds4_basic_decode(report,10,&state)&&!state.buttons&&state.lx==0x7d&&state.ly==0x7e);
 report[5]=0x28;report[6]=0x30;report[7]=1;
 assert(!ds4_basic_decode(report,10,&state)&&state.buttons==(0x4000|1|8|0x10000));
 assert(sony_basic_identity(0x054c,0x0ce6)&&!sony_basic_identity(0x1234,0x0ce6)&&!sony_basic_identity(0x054c,0x0268)&&!sony_basic_identity(0x054c,0x0df2));
 uint8_t saved[80]={0};saved[0]=80;saved[4]=4;saved[72]=2;saved[73]=6;
 const char name[]="DualSense Wireless Controller";for(unsigned i=0;i<sizeof(name);i++)saved[6+i*2]=(uint8_t)name[i];for(unsigned i=74;i<80;i++)saved[i]=(uint8_t)i;
 assert(native_sony_wireless_name(saved)&&native_record_hid_mode(saved,4)==1);
 ZeePreferred p;assert(zee_preferred_make(saved,4,&p));Ds4LocalScope s={0};ds4_scope_observe(&s,4,saved,0,&p);
 assert(!ds4_scope_active(&s,4,0x054c,0x0ce6));ds4_scope_link(&s,4,0);assert(ds4_scope_active(&s,4,0x054c,0x0ce6));
 assert(!ds4_scope_active(&s,4,0x2dc8,0x3820));
 uint8_t attribute[100];assert(ds4_basic_descriptor_attribute(4,0x054c,0x0ce6,0x1124,0x206,attribute,sizeof(attribute),0)==79);
 ZeeLayout l;assert(!zee_parse_descriptor(attribute+11,68,&l)&&l.numbered&&l.report_id==1&&l.payload_bytes==9);
 ds4_scope_link(&s,4,3);assert(!ds4_scope_active(&s,4,0x054c,0x0ce6));
 saved[7]=1;assert(!native_sony_wireless_name(saved));
 puts("PASS: published DualSense minimal BT01 sample layout, buttons/PS, exact054c:0ce6 policy, name/reconnect selection, genuine-link gating, descriptor projection and disconnect; no hardware pairing claim");
}
