#ifndef PAIR_TRACE_H
#define PAIR_TRACE_H
#include <stdint.h>
#include <string.h>
typedef struct {unsigned id,length,slot,word1;} PairEvent;
typedef int (*PairReceive)(const void *,unsigned,void *,unsigned,void *,int);
static inline int pair_trace_decode(const uint8_t *p,unsigned cap,PairEvent *out){
 if(!p||!out||cap!=84)return 0;
 unsigned id=p[0]|((unsigned)p[1]<<8),len=p[2]|((unsigned)p[3]<<8);
 if(id<0x262||id>0x273||len<4||len>cap)return 0;
 PairEvent e={id,len,0,0};
 if((id==0x262||id==0x264||id==0x269)&&len>=6){
  unsigned slot=p[4]|((unsigned)p[5]<<8);if(slot>=1&&slot<=8)e.slot=slot;
  if(id!=0x269&&len>=8)e.word1=p[6]|((unsigned)p[7]<<8);
 }
 *out=e;return 1;
}
/* Forward six EABI arguments and exact result. Observe numeric metadata only,
 * after native receive; never log PIN/key/name/address bytes or rewrite them. */
static inline int pair_trace_forward(const void *sel,unsigned count,void *packet,unsigned cap,
 void *timeout,int callbacks,PairReceive original,void (*record)(const PairEvent *)){
 int result=original(sel,count,packet,cap,timeout,callbacks);PairEvent e;
 if(result>0&&count==23&&!timeout&&callbacks==1&&pair_trace_decode(packet,cap,&e)&&record)record(&e);
 return result;
}
#define PAIR_TRACE_WORDS 9
static inline int pair_trace_plan(uint32_t base,uint32_t size,uint32_t hook,
 const uint32_t context[PAIR_TRACE_WORDS],uint32_t *patched){
 if(!context||!patched||(base&3)||(hook&3)||size<0x7484||base>0xffffffffu-size||
    ((base+0x9e8)&0xf0000000u)!=(hook&0xf0000000u))return -1;
 uint32_t call=0x0c000000u|(((base+0x7480)>>2)&0x03ffffffu);
 const uint32_t expected[PAIR_TRACE_WORDS]={0x27a40060,0x24050017,0x03a03021,0x24070054,
  0x00004021,call,0x24090001,0x044001ca,0x03a0a821};
 if(memcmp(context,expected,sizeof(expected)))return -1;
 *patched=0x0c000000u|((hook>>2)&0x03ffffffu);return 0;
}
#endif
