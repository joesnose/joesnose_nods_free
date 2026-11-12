#include "../src/zeemote_hid.c"
#include <assert.h>
#include <stdio.h>
static const uint8_t hatpad[]={0x05,1,0x09,5,0xa1,1,0x15,0,0x25,7,0x75,4,0x95,1,0x09,0x39,0x81,0x42,0x75,4,0x95,1,0x81,1,0x05,9,0x19,1,0x29,8,0x15,0,0x25,1,0x75,1,0x95,8,0x81,2,0xc0};
static const uint8_t stickpad[]={0x05,1,0x09,4,0xa1,1,0x15,0,0x26,0xff,0,0x75,8,0x95,2,0x09,0x30,0x09,0x31,0x81,2,0x05,9,0x19,1,0x29,8,0x15,0,0x25,1,0x75,1,0x95,8,0x81,2,0xc0};
static const uint8_t dpadpad[]={0x05,1,0x09,5,0xa1,1,0x15,0,0x25,1,0x75,1,0x95,4,0x09,0x90,0x09,0x91,0x09,0x92,0x09,0x93,0x81,2,0x05,9,0x19,1,0x29,4,0x95,4,0x81,2,0xc0};
int main(void){
 ZeeLayout l;ZeeState s;
 assert(!zee_parse_descriptor(hatpad,sizeof(hatpad),&l)&&l.payload_bytes==2);l.mapping=1;
 uint8_t h[]={0,4};assert(!zee_decode_report(&l,h,sizeof(h),&s)&&s.buttons==0x8010&&s.lx==128&&s.ly==128);
 assert(!zee_parse_descriptor(stickpad,sizeof(stickpad),&l)&&l.payload_bytes==3);l.mapping=1;
 uint8_t a[]={0,255,255};assert(!zee_decode_report(&l,a,sizeof(a),&s)&&s.buttons==0xf309&&s.lx==0&&s.ly==255);
 assert(!zee_parse_descriptor(dpadpad,sizeof(dpadpad),&l)&&l.payload_bytes==1);l.mapping=1;
 uint8_t d=0x51;assert(!zee_decode_report(&l,&d,1,&s)&&s.buttons==0xc010);
 uint8_t keyboard[sizeof(hatpad)];memcpy(keyboard,hatpad,sizeof(hatpad));keyboard[3]=6;
 assert(zee_parse_descriptor(keyboard,sizeof(keyboard),&l)==ZEE_UNSUPPORTED);
 uint8_t consumer[sizeof(hatpad)];memcpy(consumer,hatpad,sizeof(hatpad));consumer[1]=0x0c;
 assert(zee_parse_descriptor(consumer,sizeof(consumer),&l)==ZEE_UNSUPPORTED);
 puts("PASS: generic hat-only pad, joystick without hat, standard D-pad usages, generic button ordering/axes and rejection of keyboard/consumer application collections");
}
