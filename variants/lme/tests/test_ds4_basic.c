#include <assert.h>
#include <stdio.h>
#include "../src/zeemote_hid.c"
#include "../src/ds4_basic.c"
static void neutral(const ZeeState *s){assert(!s->buttons&&s->lx==128&&s->ly==128&&s->rx==128&&s->ry==128);}
int main(void){
 uint8_t r[82]={1,128,128,128,128,8,0,0,0,0};ZeeState s;
 assert(ds4_basic_decode(r,10,&s)==ZEE_OK);neutral(&s);
 const uint32_t hats[9]={0x10,0x30,0x20,0x60,0x40,0xc0,0x80,0x90,0};
 for(unsigned h=0;h<16;h++){
  r[5]=h;int status=ds4_basic_decode(r,10,&s);
  if(h<9)assert(!status&&s.buttons==hats[h]);else {assert(status==ZEE_INVALID);neutral(&s);}
 }
 const uint32_t face[4]={0x8000,0x4000,0x2000,0x1000};
 for(unsigned bits=0;bits<16;bits++){
  r[5]=8|(bits<<4);unsigned expected=0;
  for(unsigned i=0;i<4;i++)if(bits&(1u<<i))expected|=face[i];
  assert(!ds4_basic_decode(r,10,&s)&&s.buttons==expected);
 }
 r[5]=8;const uint32_t other[8]={0x100,0x200,0x100,0x200,1,8,0,0};
 for(unsigned bits=0;bits<256;bits++){
  r[6]=bits;unsigned expected=0;for(unsigned i=0;i<8;i++)if(bits&(1u<<i))expected|=other[i];
  assert(!ds4_basic_decode(r,10,&s)&&s.buttons==expected);
 }
 r[6]=0;
 for(unsigned v=0;v<256;v++){r[7]=v;assert(!ds4_basic_decode(r,10,&s)&&s.buttons==((v&1)?0x10000u:0));}
 r[7]=0;r[1]=0;r[2]=255;r[3]=37;r[4]=213;r[8]=255;r[9]=255;
 assert(!ds4_basic_decode(r,10,&s)&&!s.buttons&&s.lx==0&&s.ly==255&&s.rx==37&&s.ry==213);
 for(unsigned n=0;n<sizeof(r);n++)if(n!=10){assert(ds4_basic_decode(r,n,&s)==ZEE_INVALID);neutral(&s);}
 for(unsigned id=0;id<256;id++)if(id!=1){r[0]=id;assert(ds4_basic_decode(r,10,&s)==ZEE_INVALID);neutral(&s);}
 assert(ds4_basic_decode(NULL,10,&s)==ZEE_INVALID);neutral(&s);assert(ds4_basic_decode(r,10,NULL)==ZEE_INVALID);
 /* Walk SDP attribute encoding and validate the projection with production HID parser. */
 uint8_t out[128];memset(out,0xa5,sizeof(out));unsigned size=DS4_BASIC_ATTRIBUTE_SIZE;
 assert(ds4_basic_descriptor_attribute(2,0x054c,0x05c4,0x1124,0x206,out,sizeof(out),0)==(int)size);
 assert(out[0]==9&&out[1]==2&&out[2]==6&&out[3]==0x35&&out[4]+5u==size);
 assert(out[5]==0x35&&out[6]+7u==size&&out[7]==8&&out[8]==0x22&&out[9]==0x25&&out[10]+11u==size&&out[size]==0xa5);
 ZeeLayout l;assert(!zee_parse_descriptor(out+11,out[10],&l)&&l.report_id==1&&l.numbered&&l.payload_bytes==9&&l.count==19);
 assert(ds4_basic_projection(out+11,out[10]));out[11]^=1;assert(!ds4_basic_projection(out+11,out[10]));out[11]^=1;
 assert(!ds4_basic_projection(NULL,DS4_BASIC_DESCRIPTOR_SIZE));assert(!ds4_basic_projection(out+11,DS4_BASIC_DESCRIPTOR_SIZE-1));
 for(unsigned v=0;v<65536;v++){
  assert(ds4_basic_identity(v,0x05c4)==(v==0x054c));
  assert(ds4_basic_identity(0x054c,v)==(v==0x05c4||v==0x09cc));
  assert((ds4_basic_descriptor_attribute(2,0x054c,0x09cc,0x1124,v,out,sizeof(out),0)>0)==(v==0x206));
 }
 for(unsigned cap=0;cap<size;cap++){
  memset(out,0xa5,sizeof(out));assert(!ds4_basic_descriptor_attribute(2,0x054c,0x05c4,0x1124,0x206,out,cap,0));
  for(unsigned i=0;i<sizeof(out);i++)assert(out[i]==0xa5);
 }
 assert(!ds4_basic_descriptor_attribute(0,0x054c,0x05c4,0x1124,0x206,out,sizeof(out),0));
 assert(!ds4_basic_descriptor_attribute(9,0x054c,0x05c4,0x1124,0x206,out,sizeof(out),0));
 assert(!ds4_basic_descriptor_attribute(2,0x054c,0x0268,0x1124,0x206,out,sizeof(out),0));
 assert(ds4_basic_descriptor_attribute(2,0x054c,0x0ce6,0x1124,0x206,out,sizeof(out),0)==DS4_BASIC_ATTRIBUTE_SIZE);
 assert(!ds4_basic_descriptor_attribute(2,0x054c,0x05c4,0x1200,0x206,out,sizeof(out),0));
 assert(!ds4_basic_descriptor_attribute(2,0x054c,0x05c4,0x1124,0x206,NULL,sizeof(out),0));
 assert(!ds4_basic_descriptor_attribute(2,0x054c,0x05c4,0x1124,0x206,out,sizeof(out),1));
 puts("PASS: DS4 basic01 exact10-byte reports, face/shoulder/Share/Options/PS/hat combinations, counter neutrality, axis bounds, malformed neutral, Sony05c4/09cc policy, projected SDP encoding and descriptor parse; synthetic packets only");
}
