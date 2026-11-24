/* Original DS4-only default for optional HIDCountryCode metadata. MIT. */
#ifndef DS4_COUNTRY_H
#define DS4_COUNTRY_H
#include <stdint.h>
#include <string.h>
#include "ds4_basic.h"
/* SDP encoded AttributeID0203 followed by uint8 country0. This is local
 * compatibility metadata, not a controller capture or connection result. */
static inline int ds4_country_attribute(unsigned slot,unsigned vendor,unsigned product,unsigned service,unsigned attribute,void *buffer,unsigned capacity,int callbacks,int has_timeout){
 if(slot<1||slot>8||!sony_basic_identity(vendor,product)||service!=0x1124||attribute!=0x203||!buffer||capacity<5||callbacks!=0||has_timeout)return 0;
 const uint8_t data[5]={0x09,0x02,0x03,0x08,0x00};memcpy(buffer,data,sizeof(data));return sizeof(data);
}
#define DS4_COUNTRY_GUARDS 10
static const unsigned ds4_country_offsets[DS4_COUNTRY_GUARDS]={0x9d60,0x9d64,0x9d68,0x9d6c,0x9d70,0x9d74,0x9d78,0x9d7c,0x9d80,0x9d84};
static inline int ds4_country_context(uint32_t b,uint32_t size,uint32_t hook,const uint32_t *w){
 if(!w||(b|hook)&3||size<0x9d88||b>0xffffffffu-size||(((b+0x9d7c)^hook)&0xf0000000u))return 0;
 const uint32_t expected[DS4_COUNTRY_GUARDS]={0x02002021,0x24051124,0x24060203,0x03a03821,0x24080205,0x02404821,
  0x0c000000|(((b+0x55e0)>>2)&0x3ffffff),0x00005021,0x0440003d,0x93a30004};
 for(unsigned i=0;i<DS4_COUNTRY_GUARDS;i++)if(w[i]!=expected[i])return 0;
 return 1;
}
#endif
