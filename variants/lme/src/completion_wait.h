/* Targeted native completion waits; no global receiver hook. MIT. */
#ifndef COMPLETION_WAIT_H
#define COMPLETION_WAIT_H
#include <stdint.h>
#define COMPLETION_WAIT_US 5000000u
typedef int (*CompletionReceive)(const void *,unsigned,void *,unsigned,void *,int);
typedef int (*CompletionWait)(int,unsigned,unsigned,unsigned *,unsigned *);
static inline int completion_status_bound(const void *selectors,unsigned count,unsigned cap,const void *timeout,int cb){
 if(!selectors||count!=1||cap!=16||timeout||cb!=0)return 0;
 const unsigned char *p=selectors;
 return ((unsigned)p[2]|((unsigned)p[3]<<8))==0x275;
}
static inline int completion_status_forward(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb,CompletionReceive original){
 uint32_t budget=COMPLETION_WAIT_US;
 return original(sel,count,packet,cap,completion_status_bound(sel,count,cap,timeout,cb)?&budget:timeout,cb);
}
static inline int completion_disconnect_bound(int id,unsigned bits,unsigned mode,const unsigned *out,const unsigned *timeout,int known_original,unsigned profiles){
 return (profiles&2)&&id>0&&bits==2&&mode==1&&out&&!timeout&&known_original;
}
static inline int completion_disconnect_forward(int id,unsigned bits,unsigned mode,unsigned *out,unsigned *timeout,CompletionWait original,int known_original,unsigned profiles){
 unsigned budget=COMPLETION_WAIT_US;
 return original(id,bits,mode,out,completion_disconnect_bound(id,bits,mode,out,timeout,known_original,profiles)?&budget:timeout);
}
#define COMPLETION_GUARDS 31
static const unsigned completion_offsets[COMPLETION_GUARDS]={
 0x32f4,0x3340,0x33bc,0x33c4,0x33f4,0x33f8,0x33fc,0x3400,0x3404,0x3408,0x340c,0x3410,0x3414,0x3418,0x341c,
 0x2b10,0x2b1c,0x2b14,0x2b2c,0x2b40,0x2b48,0x2b4c,0x2ba0,0x2bac,0x2ccc,0x2cd0,0x2cc0,0x2cc4,0x2cc8,0x2cd4,0x2ce8};
static inline void completion_expected(uint32_t b,uint32_t w[COMPLETION_GUARDS]){
 uint32_t cb=b+0xcac0,plain=b+0xcad0,seq=b+0xdd00;
 const uint32_t e[COMPLETION_GUARDS]={
 0x27bdff90,0x24020185,0x260c0000|(seq&0xffff),0x0c000000|(((b+0xcb10)>>2)&0x3ffffff),
 0x240b0275,0x27a60040,0x27a40030,0x24050001,0x24070010,0x00004021,0x00004821,0xa7b20030,
 0x0c000000|(((b+0x7480)>>2)&0x3ffffff),0xa7ab0032,0x0440ffe7,
 0x27bdff90,0x00a0a021,0x3c020000|((cb+0x8000u)>>16),0x24520000|(cb&0xffff),0x14c00003,
 0x3c030000|((plain+0x8000u)>>16),0x24720000|(plain&0xffff),0x24060069,0x240e0169,
 0x0240f809,0x00004021,0x24050002,0x24060001,0x27a70040,0x0440ffdd,
 0x0c000000|(((b+0xcbc0)>>2)&0x3ffffff)};
 for(unsigned i=0;i<COMPLETION_GUARDS;i++)w[i]=e[i];
}
static inline int completion_plan(uint32_t b,uint32_t size,uint32_t result_hook,uint32_t disconnect_hook,const uint32_t *w,uint32_t patched[2]){
 if(!w||!patched||(b|result_hook|disconnect_hook)&3||size<0xcbc4||b>0xffffffffu-size||
    (((b+0x3418)^result_hook)&0xf0000000u)||(((b+0x2cd0)^disconnect_hook)&0xf0000000u))return 0;
 uint32_t e[COMPLETION_GUARDS];completion_expected(b,e);
 for(unsigned i=0;i<COMPLETION_GUARDS;i++)if(w[i]!=e[i])return 0;
 patched[0]=0x0c000000|((result_hook>>2)&0x3ffffff);
 patched[1]=0x0c000000|((disconnect_hook>>2)&0x3ffffff);return 1;
}
#endif
