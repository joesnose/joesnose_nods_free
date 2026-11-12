#ifndef SDP_WAIT_H
#define SDP_WAIT_H
#include <stdint.h>
#define SDP_WAIT_US 5000000u
#define SDP_GUARDS 10
static const uint32_t sdp_offsets[SDP_GUARDS]={0x56b4,0x56b8,0x56bc,0x56c0,0x56c4,0x56c8,0x56cc,0x56d0,0x7480,0x7484};
static inline int sdp_context(uint32_t b,uint32_t size,uint32_t hook,const uint32_t *w){
 if(!w||(b&3)||(hook&3)||size<0x7488||b>0xffffffffu-size||(((b+0x56cc)^hook)&0xf0000000u))return 0;
 const uint32_t e[SDP_GUARDS]={0x02402021,0x24050001,0x02203021,0x2407004c,0x02a04021,0x0c000000|(((b+0x7480)>>2)&0x03ffffff),0x03c04821,0x00408021,0x27bdffd0,0xafb60028};
 for(unsigned i=0;i<SDP_GUARDS;i++)if(w[i]!=e[i])return 0;
 return 1;
}
static inline int sdp_bound(const void *selectors,unsigned count,unsigned cap,const void *timeout,int cb){
 if(!selectors||count!=1||cap!=0x4c||timeout||(cb!=0&&cb!=1))return 0;
 const unsigned char *p=selectors;
 return ((unsigned)p[2]|((unsigned)p[3]<<8))==0x272;
}
typedef int (*SdpReceive)(const void *,unsigned,void *,unsigned,void *,int);
static inline int sdp_forward(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb,SdpReceive original){
 uint32_t budget=SDP_WAIT_US;
 return original(sel,count,packet,cap,sdp_bound(sel,count,cap,timeout,cb)?&budget:timeout,cb);
}
#endif
