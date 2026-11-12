#ifndef STATUS_TIMEOUT_H
#define STATUS_TIMEOUT_H
#include <stdint.h>
#define STATUS_TIMEOUT_US 2000000u
static inline int status_bound(unsigned response,int has_timeout,int callback,int target,int role){
 return !has_timeout&&target&&((role==1&&response==0x185&&callback==0)||(role==2&&response>=0x100&&response<=0x1ff&&(callback==0||callback==1)));
}
#define STATUS_GUARDS 11
static const uint32_t status_offsets[STATUS_GUARDS]={0x37c4,0x37c8,0x37d4,0x37d8,0x37dc,0x37e0,0x37e4,0x37e8,0x37ec,0x7340,0x7344};
static inline int status_context(uint32_t base,uint32_t size,const uint32_t *words){
 if(!words||(base&3)||size<0x7348||base>0xffffffffu-size)return 0;
 const uint32_t expected[STATUS_GUARDS]={0x02403021,0x00002821,0x96090000,0x96020002,0x8e080008,0xa7a90000,0xa7a20004,0x0c000000|(((base+0x7340)>>2)&0x03ffffff),0xafa80008,0x27bdffc0,0xafb60038};
 for(unsigned i=0;i<STATUS_GUARDS;i++)if(words[i]!=expected[i])return 0;
 return 1;
}
typedef int (*StatusReceive)(uint16_t *,uint32_t *,int);
static inline int status_receive_forward(uint16_t *reply,uint32_t *timeout,int cb,int target,int role,StatusReceive original,int *bounded){
 uint32_t local=STATUS_TIMEOUT_US;
 *bounded=status_bound(reply?reply[0]:0,timeout!=0,cb,target,role);
 return original(reply,*bounded?&local:timeout,cb);
}
#endif
