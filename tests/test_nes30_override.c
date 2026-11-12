#include <assert.h>
#include <stdio.h>
#include "../src/zeemote_hid.c"
#include "nes30_override.h"
#include "service_trace.h"
int main(void){
 _Static_assert(sizeof(nes30_capture_descriptor)==86,"captured descriptor length");
 uint8_t out[128];memset(out,0xa5,sizeof(out));
 assert(nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1124,0x206,out,128,0)==97);
 /* Independently walk the encoded AttributeID and nested descriptor sequences. */
 assert(out[0]==0x09&&out[1]==2&&out[2]==6);
 assert(out[3]==0x35&&out[4]+5u==97&&out[5]==0x35&&out[6]+7u==97);
 assert(out[7]==0x08&&out[8]==0x22&&out[9]==0x25&&out[10]==86);
 assert(!memcmp(out+11,nes30_capture_descriptor,86)&&out[97]==0xa5);
 ZeeLayout l;assert(!zee_parse_descriptor(out+11,out[10],&l)&&l.report_id==3&&l.payload_bytes==9);
 for(unsigned v=0;v<65536;v++){
  assert((nes30_descriptor_attribute(2,v,0x3820,0x1124,0x206,out,128,0)>0)==(v==0x2dc8));
  assert((nes30_descriptor_attribute(2,0x2dc8,v,0x1124,0x206,out,128,0)>0)==(v==0x3820));
  assert((nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1124,v,out,128,0)>0)==(v==0x206));
 }
 for(unsigned cap=0;cap<97;cap++){
  memset(out,0xa5,sizeof(out));assert(!nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1124,0x206,out,cap,0));assert(out[0]==0xa5&&out[127]==0xa5);
 }
 assert(!nes30_descriptor_attribute(0,0x2dc8,0x3820,0x1124,0x206,out,128,0));
 assert(!nes30_descriptor_attribute(9,0x2dc8,0x3820,0x1124,0x206,out,128,0));
 assert(!nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1200,0x206,out,128,0));
 assert(!nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1124,0x206,NULL,128,0));
 assert(!nes30_descriptor_attribute(2,0x2dc8,0x3820,0x1124,0x206,out,128,1));
 uint32_t w[NES30_GUARDS]={0x03a03821,0x24080205,0x0e08f9f8,0x00005021,0x04400032,0x00049100,0x958b0010,0x958d0012};
 assert(nes30_context(0x88239200,0xd000,0x88400000,w));
 for(unsigned i=0;i<NES30_GUARDS;i++){w[i]^=1;assert(!nes30_context(0x88239200,0xd000,0x88400000,w));w[i]^=1;}
 unsigned status,more,length;uint8_t p[76]={0x72,2,15,0,0,0,0,0,1,0,3,0,99,98,97};
 assert(service_fragment(p,76,&status,&more,&length)&&!status&&more==1&&length==3);
 p[10]=4;assert(!service_fragment(p,76,&status,&more,&length));p[10]=3;
 p[2]=11;assert(!service_fragment(p,76,&status,&more,&length));p[2]=77;assert(!service_fragment(p,76,&status,&more,&length));p[2]=15;p[0]=0x71;assert(!service_fragment(p,76,&status,&more,&length));
 puts("PASS: NES30 exact identity/attribute/slot/mode and97-byte SDP encoding/capacity, production descriptor parse, all8 native guards and bounded fragment metadata");
}
