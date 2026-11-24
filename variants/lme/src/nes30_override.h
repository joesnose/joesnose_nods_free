#ifndef NES30_OVERRIDE_H
#define NES30_OVERRIDE_H
#include <stdint.h>
#include <string.h>
#include "nes30_capture.h"
#define NES30_ATTRIBUTE_SIZE (11u+sizeof(nes30_capture_descriptor))
static inline int nes30_descriptor_attribute(unsigned slot,unsigned vendor,unsigned product,unsigned service,unsigned attribute,void *buffer,unsigned capacity,int callbacks){
 if(slot<1||slot>8||vendor!=0x2dc8||product!=0x3820||service!=0x1124||attribute!=0x206||!buffer||capacity<NES30_ATTRIBUTE_SIZE||callbacks!=0)return 0;
 const uint8_t prefix[11]={0x09,0x02,0x06,0x35,0x5c,0x35,0x5a,0x08,0x22,0x25,0x56};
 memcpy(buffer,prefix,sizeof(prefix));memcpy((uint8_t *)buffer+sizeof(prefix),nes30_capture_descriptor,sizeof(nes30_capture_descriptor));return (int)NES30_ATTRIBUTE_SIZE;
}
#define NES30_GUARDS 8
static const uint32_t nes30_offsets[NES30_GUARDS]={0x9d9c,0x9da0,0x9da4,0x9da8,0x9dac,0x58f0,0x5930,0x5950};
static inline int nes30_context(uint32_t b,uint32_t size,uint32_t hook,const uint32_t *w){
 if(!w||(b&3)||(hook&3)||size<0x9db0||b>0xffffffffu-size||(((b+0x9da8)^hook)&0xf0000000u))return 0;
 const uint32_t e[NES30_GUARDS]={0x03a03821,0x24080205,0x0c000000|(((b+0x55e0)>>2)&0x03ffffff),0x00005021,0x04400032,0x00049100,0x958b0010,0x958d0012};
 for(unsigned i=0;i<NES30_GUARDS;i++)if(w[i]!=e[i])return 0;
 return 1;
}
#endif
