/* Selected-controller native link setup policy and ABI guards. Original MIT. */
#ifndef DS4_LINK_SETUP_H
#define DS4_LINK_SETUP_H
#include <stdint.h>
typedef struct {unsigned linked,pending,epoch[8];} Ds4LinkSetup;
static inline unsigned ds4_setup_bit(unsigned slot){return slot>=1&&slot<=8?1u<<(slot-1):0;}
static inline int ds4_setup_event(Ds4LinkSetup *s,unsigned slot,unsigned state){
 unsigned bit=ds4_setup_bit(slot);if(!bit)return 0;
 if(state==0){if(s->linked&bit)return 0;s->linked|=bit;s->pending|=bit;++s->epoch[slot-1];return 1;}
 if(state==2||state==3){s->linked&=~bit;s->pending&=~bit;++s->epoch[slot-1];}
 return 0;
}
static inline unsigned ds4_setup_take(Ds4LinkSetup *s,unsigned slot,int eligible){
 unsigned bit=ds4_setup_bit(slot);if(!eligible||!(s->linked&s->pending&bit))return 0;
 s->pending&=~bit;return s->epoch[slot-1];
}
static inline int ds4_setup_current(const Ds4LinkSetup *s,unsigned slot,unsigned epoch){
 unsigned bit=ds4_setup_bit(slot);return bit&&epoch&&(s->linked&bit)&&s->epoch[slot-1]==epoch;
}
static inline int ds4_setup_wait_bound(int owned,unsigned active,unsigned response,int has_timeout,int cb){return owned&&active==response&&(response==0x17a||response==0x124)&&!has_timeout&&!cb;}
#define DS4_SETUP_GUARDS 33
static const unsigned ds4_setup_offsets[DS4_SETUP_GUARDS]={0x34f8,0x3500,0x3504,0x3508,0x3510,0x3518,0x3534,0x3538,0x3548,0x354c,0xab8c,0xab94,0xab98,0xab9c,0xaba0,0xaba4,0xabac,0xabc8,0xabcc,0xabd0,0xabd4,0xabf0,0xabf4,0xabf8,0xabfc,0xac00,0xa27c,0xa280,0xa284,0xa288,0xa28c,0xa290,0xa294};
static inline void ds4_setup_expected(uint32_t b,uint32_t *out){
 const uint32_t w[DS4_SETUP_GUARDS]={0x27bdffb0u,0x00805021u,0x00a01821u,0x240c0004u,0x24070079u,0x2402017au,0xa7aa0020u,0xa7a30022u,0x0c000000u|(((b+0x3720u)>>2)&0x3ffffffu),0xa7ac0002u,0x27bdff50u,0x240c0001u,0x240f0012u,0x240a0002u,0x24080024u,0x24070010u,0x24090124u,0xa7ab0020u,0xa7ac0024u,0xa7af0028u,0xa7aa002au,0xa7ac0022u,0xa7af0026u,0xa7a0002cu,0x0c000000u|(((b+0x3720u)>>2)&0x3ffffffu),0xa7a0002eu,0x240d0268u,0x15cdffdeu,0x02202021u,0x0c000000u|(((b+0x34f8u)>>2)&0x3ffffffu),0x00002821u,0x0c000000u|(((b+0xab8cu)>>2)&0x3ffffffu),0x02202021u};
 for(unsigned i=0;i<DS4_SETUP_GUARDS;i++)out[i]=w[i];
}
static inline int ds4_setup_context(uint32_t b,uint32_t size,const uint32_t *words){
 if(!b||(b&3)||size<0xac10||b>0xffffffffu-size||!words)return 0;
 uint32_t expected[DS4_SETUP_GUARDS];ds4_setup_expected(b,expected);
 for(unsigned i=0;i<DS4_SETUP_GUARDS;i++)if(words[i]!=expected[i])return 0;
 return 1;
}
#endif
