#ifndef LIFECYCLE_TRACE_H
#define LIFECYCLE_TRACE_H
#include <stdint.h>
#include <string.h>
#define LIFE_GUARDS 9
static const unsigned life_offsets[LIFE_GUARDS]={0x4e3c,0x4e70,0x4e74,0xa44,0xa48,0x2d28,0x2d2c,0x34d4,0x34d8};
static inline void life_expected(uint32_t b,uint32_t c[LIFE_GUARDS]){
 const uint32_t v[LIFE_GUARDS]={b+0xa44,b+0x2d28,b+0x34d4,0x27bdffe0,0xafb20018,0x27bdfff0,0xafbf0008,0x27bdfff0,0xafbf0008};memcpy(c,v,sizeof(v));
}
static inline int life_context_ok(uint32_t b,uint32_t size,const uint32_t c[LIFE_GUARDS]){
 if(!c||(b&3)||size<0x4e80||b>0xffffffffu-size)return 0;
 uint32_t e[LIFE_GUARDS];life_expected(b,e);return !memcmp(e,c,sizeof(e));
}
typedef int (*LifeRemove)(void *);
typedef int (*LifeTerm)(void *,void *);
typedef void (*LifeRecord)(unsigned,unsigned,int);
static inline int life_remove_forward(void *e,LifeRemove original,LifeRecord record){
 record(1,0,0);int result=original(e);record(2,0,result);return result;
}
static inline int life_term_forward(unsigned kind,void *c,void *n,LifeTerm original,LifeRecord record){
 record(3,kind,0);int result=original(c,n);record(4,kind,result);return result;
}
static inline int life_watched(const char *name){
 return name&&(!strncmp(name,"SceBSMan",8)||!strncmp(name,"SceHIDSvc",9)||
  !strcmp(name,"SceInputThread")||!strcmp(name,"SceKernelModmgr")||!strcmp(name,"SceKernelLoadExec"));
}
#endif
