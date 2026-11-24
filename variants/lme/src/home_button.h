/* DS4 PS -> native PSP HOME press; original edge policy and API guards. */
#ifndef ZEE_HOME_BUTTON_H
#define ZEE_HOME_BUTTON_H
#include <stdint.h>
typedef struct {unsigned armed,down,pending,stamp;} ZeeHomeButton;
static inline void zee_home_reset(ZeeHomeButton *h){*h=(ZeeHomeButton){0};}
static inline void zee_home_report(ZeeHomeButton *h,int enabled,int down,unsigned now){
 if(!enabled){zee_home_reset(h);return;}
 if(!down){h->armed=1;h->down=0;return;}
 if(h->armed&&!h->down){h->pending=1;h->stamp=now;}
 h->down=1;
}
static inline int zee_home_take(ZeeHomeButton *h,int enabled,int active,unsigned now){
 if(!h->pending)return 0;
 h->pending=0;
 return enabled&&active&&(unsigned)(now-h->stamp)<250000u;
}
#define HOME_GUARDS 16
static const unsigned home_offsets[HOME_GUARDS]={0,8,12,20,24,28,32,36,40,44,48,52,56,60,64,72};
static inline int zee_home_context(uint32_t base,uint32_t size,uint32_t entry,const uint32_t *w){
 if(!w||(base|entry)&3||size<0x4c||base>0xffffffffu-size||entry<base||entry-base>size-0x4c)return 0;
 const uint32_t e[13]={0x27bdfff0,0xafb00000,0x03608021,0xafbf0004,0x8cc50074,0x30a30001,0x10600006,0x001bdac0,0x0200d821,0x8fbf0004,0x8fb00000,0x03e00008,0x27bd0010};
 for(unsigned i=0;i<13;i++)if(w[i]!=e[i])return 0;
 if(w[13]>>26!=3||w[14]||w[15]!=0x0200d821)return 0;
 uint32_t target=((entry+64)&0xf0000000u)|((w[13]&0x3ffffffu)<<2);
 return target>=base&&target-base<size;
}
/* Native GetStatus reads the same status word used by HOME itself. */
static inline int zee_home_status_context(uint32_t base,uint32_t size,uint32_t entry,const uint32_t w[4]){
 return w&&!((base|entry)&3)&&size>=16&&base<=0xffffffffu-size&&entry>=base&&entry-base<=size-16&&
  (w[0]&0xffff0000u)==0x3c030000u&&(w[1]&0xffff0000u)==0x8c640000u&&w[2]==0x03e00008u&&w[3]==0x8c8200a4u;
}
static inline int zee_home_open_argument(unsigned native_status){return !(native_status&0x100u);}
#endif
