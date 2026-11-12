/* Original DS4 minimal Bluetooth report translator, MIT.
 * Wire layout cross-checked against Linux hid-playstation and Bluepad32.
 * Basic input only; this does not pair or change controller report mode. */
#include "ds4_basic.h"
int ds4_basic_decode(const uint8_t *r,size_t n,ZeeState *out){
 if(!out)return ZEE_INVALID;
 ZeeState s={0,128,128,128,128};*out=s;
 if(!r||n!=DS4_BASIC_REPORT_SIZE||r[0]!=1||(r[5]&15)>8)return ZEE_INVALID;
 static const uint32_t hats[9]={0x10,0x30,0x20,0x60,0x40,0xc0,0x80,0x90,0};
 static const uint32_t face[4]={0x8000,0x4000,0x2000,0x1000};
 s.lx=r[1];s.ly=r[2];s.rx=r[3];s.ry=r[4];s.buttons=hats[r[5]&15];
 for(unsigned i=0;i<4;i++)if(r[5]&(0x10u<<i))s.buttons|=face[i];
 if(r[6]&0x05)s.buttons|=0x100; /* L1 or digital L2 -> PSP L */
 if(r[6]&0x0a)s.buttons|=0x200; /* R1 or digital R2 -> PSP R */
 if(r[6]&0x10)s.buttons|=1; /* Share -> Select */
 if(r[6]&0x20)s.buttons|=8; /* Options -> Start */
 if(r[7]&1)s.buttons|=0x10000; /* PS -> HOME */
 *out=s;return ZEE_OK;
}
