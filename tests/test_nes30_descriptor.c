#include <assert.h>
#include <stdio.h>
#include "../src/zeemote_hid.c"
#include "nes30_descriptor_fixture.h"
int main(void){
 ZeeLayout l;ZeeState s;
 assert(zee_parse_descriptor(nes30_descriptor,sizeof(nes30_descriptor),&l)==ZEE_OK);
 assert(l.numbered&&l.report_id==3&&l.payload_bytes==9);l.mapping=2;
 /* Synthesized states exercising the real captured descriptor, not captured reports. */
 uint8_t r[10]={3,15,128,128,128,128,0,0,0,0};
 assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&!s.buttons&&s.lx==128&&s.ly==128);
 r[8]=1;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x2000);
 r[8]=2;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x4000);
 r[8]=8;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x1000);
 r[8]=16;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x8000);
 r[8]=4;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x10000);
 r[8]=0;r[9]=4;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==1);
 r[9]=8;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==8);
 r[9]=0;r[8]=0;r[1]=2;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.buttons==0x20);
 r[1]=15;r[2]=0;r[3]=255;assert(zee_decode_report(&l,r,sizeof(r),&s)==ZEE_OK&&s.lx==0&&s.ly==255);
 r[0]=2;assert(zee_decode_report(&l,r,sizeof(r),&s)!=ZEE_OK);r[0]=3;
 assert(zee_decode_report(&l,r,sizeof(r)-1,&s)!=ZEE_OK);
 printf("PASS: actual NES30 Pro Bluetooth descriptor86 bytes, reportID%u, payload%u, fields%u; synthetic neutral/button/hat/axes and wrong-ID/truncated-report rejection\n",l.report_id,l.payload_bytes,l.count);
}
