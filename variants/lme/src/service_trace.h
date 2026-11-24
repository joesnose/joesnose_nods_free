#ifndef SERVICE_TRACE_H
#define SERVICE_TRACE_H
#include <stdint.h>
static inline unsigned trace_u16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static inline int service_fragment(const void *packet,unsigned capacity,unsigned *status,unsigned *more,unsigned *length){
 if(!packet||capacity!=76||!status||!more||!length)return 0;
 const uint8_t *p=packet;unsigned n=trace_u16(p+2);
 if(trace_u16(p)!=0x272||n<12||n>capacity)return 0;
 unsigned size=trace_u16(p+10);if(size>n-12)return 0;
 *status=trace_u16(p+6);*more=trace_u16(p+8);*length=size;return 1;
}
#endif
