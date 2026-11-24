/* Original selected-target DS4 host metadata adapter. No radio identity writes. MIT. */
#ifndef DS4_LOCAL_H
#define DS4_LOCAL_H
#include "game_reconnect.h"
#include "ds4_basic.h"
typedef struct {unsigned selected,linked;} Ds4LocalScope;
static inline unsigned ds4_slot_bit(unsigned slot){return slot>=1&&slot<=8?1u<<(slot-1):0;}
static inline void ds4_scope_observe(Ds4LocalScope *s,unsigned slot,const uint8_t *record,int status,const ZeePreferred *preferred){
 if(!zee_preferred_valid(preferred)){s->selected=0;return;}
 if(s->selected&~ds4_slot_bit(preferred->slot))s->selected=0;
 if(slot!=preferred->slot)return;
 s->selected=status>=0&&generic_saved_hid(record,slot)&&native_sony_wireless_name(record)&&zee_preferred_matches(preferred,record,slot)?ds4_slot_bit(slot):0;
}
static inline void ds4_scope_link(Ds4LocalScope *s,unsigned slot,unsigned state){
 unsigned bit=ds4_slot_bit(slot);if(state==0)s->linked|=bit;else if(state==2||state==3)s->linked&=~bit;
}
static inline int ds4_scope_selected(const Ds4LocalScope *s,unsigned slot,unsigned vendor,unsigned product){
 return !!(s->selected&ds4_slot_bit(slot))&&(!vendor||!product||sony_basic_identity(vendor,product));
}
static inline int ds4_scope_active(const Ds4LocalScope *s,unsigned slot,unsigned vendor,unsigned product){
 return ds4_scope_selected(s,slot,vendor,product)&&!!(s->linked&ds4_slot_bit(slot));
}
static inline int ds4_local_attribute(unsigned slot,int active,unsigned service,unsigned attribute,void *buffer,unsigned capacity,void *timeout,int cb){
 if(!ds4_slot_bit(slot)||!active||service!=0x1124||!buffer||timeout||cb)return 0;
 if(attribute==0x206)return ds4_basic_projected_attribute(buffer,capacity);
 if(attribute==0x201&&capacity>=6){const uint8_t p[6]={9,2,1,9,1,0x11};memcpy(buffer,p,6);return 6;}
 if(attribute==0x203&&capacity>=5){const uint8_t p[5]={9,2,3,8,0};memcpy(buffer,p,5);return 5;}
 return 0;
}
static inline int ds4_hid_context_index(uint32_t base,uintptr_t context){
 if(!base||base>0xffffffffu-0x4f50)return 0;
 return context==base+0x4f34?1:context==base+0x4f44?2:0;
}
static inline int ds4_backend_index(uint32_t base,uintptr_t backend){
 if(!base||base>0xffffffffu-0xdec0)return 0;
 return backend==base+0xde80?1:backend==base+0xdeb0?2:0;
}
#define LOCAL_GUARDS 40
static const unsigned local_offsets[LOCAL_GUARDS]={0xa1d4,0xa1d8,0xa1e0,0xa1e4,0xa1e8,0x9d44,0x9d4c,0x9d50,0x9d54,0x9d58,0xac10,0xac14,0xac18,0xac1c,0xac20,0xac24,0xac28,0x58d8,0x58dc,0x9cf8,0x9d00,0x9d04,0x9d08,0x9d0c,0x9d28,0x9d2c,0x9d30,0x9d34,0x9d68,0x9d78,0x9d7c,0x9d84,0x9d98,0x9da4,0x9da8,0xa218,0xa220,0xa228,0xa22c,0xa230};
static inline void ds4_local_expected(uint32_t b,uint32_t country,uint32_t descriptor,uint32_t *w){
 const uint32_t e[LOCAL_GUARDS]={0x27a60002,0x2202021,0x3a02821,0x0c000000|(((b+0x58d8)>>2)&0x3ffffff),0x3821,0x27a50210,0x27a60212,0x2002021,0x0c000000|(((b+0x58d8)>>2)&0x3ffffff),0x2403821,0xa01821,0x27bdfff0,0xc03821,0x24a50002,0xafbf0000,0x0c000000|(((b+0x58d8)>>2)&0x3ffffff),0x603021,0x27bdffd0,0xafb50024,0x24060201,0x24080205,0x24051124,0x2404821,0x5021,0x0c000000|(((b+0x55e0)>>2)&0x3ffffff),0xa7a00212,0x4400051,0x93a40005,0x24060203,0x0c000000|((country>>2)&0x3ffffff),0x5021,0x93a30004,0x24060206,0x0c000000|((descriptor>>2)&0x3ffffff),0x5021,0x8e040000,0x8e250008,0x2004021,0x0c000000|(((b+0xccc0)>>2)&0x3ffffff),0x24060040};
 for(unsigned i=0;i<LOCAL_GUARDS;i++)w[i]=e[i];
}
static inline int ds4_local_context(uint32_t b,uint32_t size,uint32_t country,uint32_t descriptor,const uint32_t *w){
 if(!w||(b|country|descriptor)&3||(((b+0x9d7c)^country)&0xf0000000u)||(((b+0x9da8)^descriptor)&0xf0000000u)||size<0xac2c||b>0xffffffffu-size)return 0;
 uint32_t e[LOCAL_GUARDS];ds4_local_expected(b,country,descriptor,e);
 for(unsigned i=0;i<LOCAL_GUARDS;i++)if(w[i]!=e[i])return 0;
 return 1;
}
#endif
