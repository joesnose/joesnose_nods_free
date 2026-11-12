#include "zeemote_hid.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Synthetic specification-based descriptor, NOT a SteelSeries capture:
 * Game Pad application, 16 buttons, unsigned X/Y/Z/Rz, 4-bit hat+padding. */
static const uint8_t descriptor[]={
 0x05,1,0x09,5,0xa1,1,0x85,7,
 0x05,9,0x19,1,0x29,16,0x15,0,0x25,1,0x75,1,0x95,16,0x81,2,
 0x05,1,0x09,0x30,0x09,0x31,0x09,0x32,0x09,0x35,0x15,0,0x26,0xff,0,0x75,8,0x95,4,0x81,2,
 0x09,0x39,0x15,0,0x25,7,0x75,4,0x95,1,0x81,0x42,0x75,4,0x95,1,0x81,1,0xc0
};
static void fail_descriptor(const uint8_t *d,size_t n){ZeeLayout l,saved;memset(&l,0xa5,sizeof(l));saved=l;int status=zee_parse_descriptor(d,n,&l);if(status>=0)fprintf(stderr,"unexpected descriptor acceptance n=%zu first=%02x mutation37=%02x\n",n,n?d[0]:0,n>37?d[37]:0);assert(status<0);assert(!memcmp(&l,&saved,sizeof(l)));}
int main(void){
 ZeeLayout l;assert(!zee_parse_descriptor(descriptor,sizeof(descriptor),&l));assert(l.report_id==7&&l.numbered==1&&l.payload_bytes==7);
 uint8_t report[]={7,0,0,128,128,128,128,8};ZeeState s;
 assert(!zee_decode_report(&l,report,sizeof(report),&s));assert(!s.buttons&&s.lx==128&&s.ly==128&&s.rx==128&&s.ry==128);
 const uint32_t hats[]={0x10,0x30,0x20,0x60,0x40,0xc0,0x80,0x90};
 for(unsigned i=0;i<8;i++){report[7]=i;assert(!zee_decode_report(&l,report,8,&s));assert(s.buttons==hats[i]);}
 report[7]=8;report[1]=0x59;report[2]=0x18;report[3]=0;report[4]=255;report[5]=0;report[6]=255;
 assert(!zee_decode_report(&l,report,8,&s));assert(s.buttons==(0x4000|0x8000|0x1000|0x100|8|1));assert(s.lx==0&&s.ly==255&&s.rx==0&&s.ry==255);
 for(unsigned i=0;i<16;i++){report[1]=(uint8_t)(1u<<i);report[2]=(uint8_t)((1u<<i)>>8);assert(!zee_decode_report(&l,report,8,&s));static const uint32_t expected[]={0x4000,0x2000,0,0x8000,0x1000,0,0x100,0x200,0,0,0,8,1,0,0,0};assert(s.buttons==expected[i]);}
 memset(&s,0xa5,sizeof(s));ZeeState old=s;for(unsigned n=0;n<8;n++){assert(zee_decode_report(&l,report,n,&s)<0);assert(!memcmp(&s,&old,sizeof(s)));}report[0]=6;assert(zee_decode_report(&l,report,8,&s)==ZEE_UNSUPPORTED);assert(!memcmp(&s,&old,sizeof(s)));report[0]=7;
 /* Every descriptor prefix is either incomplete or lacks its closing collection. */
 for(unsigned n=0;n<sizeof(descriptor);n++)fail_descriptor(descriptor,n);
 uint8_t bad[sizeof(descriptor)];memcpy(bad,descriptor,sizeof(bad));bad[3]=6;fail_descriptor(bad,sizeof(bad)); /* keyboard must not inject */
 memcpy(bad,descriptor,sizeof(bad));bad[7]=0;fail_descriptor(bad,sizeof(bad)); /* report ID0 forbidden */
 memcpy(bad,descriptor,sizeof(bad));bad[19]=65;fail_descriptor(bad,sizeof(bad)); /* report count bound */
 memcpy(bad,descriptor,sizeof(bad));bad[40]=33;fail_descriptor(bad,sizeof(bad)); /* unsupported bit width */
 const uint8_t long_item[]={0xfe,0,0};fail_descriptor(long_item,sizeof(long_item));
 const uint8_t pop[]={0xb4};fail_descriptor(pop,sizeof(pop));
 /* Signed 12-bit X/Y straddle byte boundaries, with button padding before them. */
 const uint8_t signed_d[]={0x05,1,0x09,5,0xa1,1,0x05,9,0x09,1,0x15,0,0x25,1,0x75,1,0x95,1,0x81,2,0x75,7,0x95,1,0x81,1,0x05,1,0x09,0x30,0x09,0x31,0x16,0,0xf8,0x26,0xff,7,0x75,12,0x95,2,0x81,2,0x09,0x39,0x15,0,0x25,7,0x75,4,0x95,1,0x81,0x42,0x75,4,0x95,1,0x81,1,0xc0};
 assert(!zee_parse_descriptor(signed_d,sizeof(signed_d),&l));uint8_t signed_r[]={1,0,0xf8,0x7f,8};assert(!zee_decode_report(&l,signed_r,5,&s));assert(s.buttons==0x4000&&s.lx==0&&s.ly==255);
 /* Physical/unit globals and feature-only large counts must not reject a
  * valid input layout or shift the input report offsets. */
 uint8_t metadata[sizeof(descriptor)+16];memcpy(metadata,descriptor,sizeof(descriptor)-1);
 const uint8_t extras[]={0x35,0,0x46,0x3b,1,0x65,0x14,0x96,0x2c,1,0xb1,1,0xc0};
 memcpy(metadata+sizeof(descriptor)-1,extras,sizeof(extras));ZeeLayout physical;
 assert(!zee_parse_descriptor(metadata,sizeof(descriptor)-1+sizeof(extras),&physical));assert(physical.payload_bytes==7);
 assert(!zee_parse_descriptor(signed_d,sizeof(signed_d),&l));uint8_t centered[]={0,0,0,0,8};assert(!zee_decode_report(&l,centered,5,&s));assert(s.lx==128&&s.ly==128);
 for(unsigned i=0;i<l.count;i++)if(l.fields[i].kind==5){ZeeLayout corrupt=l;corrupt.fields[i].maximum=corrupt.fields[i].minimum+8;assert(zee_decode_report(&corrupt,centered,5,&s)<0);}
 /* Report mutation never accesses past exact report size or changes output on failure. */
 for(unsigned b=0;b<5;b++)for(unsigned v=0;v<256;v++){uint8_t r[5];memcpy(r,signed_r,5);r[b]=v;ZeeState state={0};int result=zee_decode_report(&l,r,5,&state);assert(result==0||result<0);}
 puts("PASS: descriptor-derived numbered/unnumbered reports, all buttons/hats, signed cross-byte axes, neutral/extremes, bounds/truncation/malformed rejection and unchanged failure outputs (synthetic HID fixtures; no Free hardware layout claim)");
 return 0;
}
