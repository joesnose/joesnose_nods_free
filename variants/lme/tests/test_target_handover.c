#include <assert.h>
#include <stdio.h>
#include "target_handover.h"
static void record_make(uint8_t *r,unsigned slot,const char *name){memset(r,0,80);r[0]=80;r[4]=(uint8_t)slot;r[72]=2;r[73]=4;for(unsigned i=0;name[i];i++)r[6+2*i]=(uint8_t)name[i];for(unsigned i=74;i<80;i++)r[i]=(uint8_t)(i+slot);}
int main(void){
 uint8_t ds4[80],ds5[80],attr[100];record_make(ds4,4,"Wireless Controller");record_make(ds5,5,"DualSense Wireless Controller");
 ZeePreferred p;assert(zee_preferred_make(ds4,4,&p));Ds4LocalScope s={0};
 ds4_scope_observe(&s,4,ds4,0,&p);ds4_scope_link(&s,4,0);
 assert(sony_target_prepare(&s,&p,5,ds5)==2&&p.slot==5&&s.selected==16&&s.linked==8);
 assert(!ds4_scope_active(&s,5,0,0)); /* selecting is never a successful link */
 ds4_scope_link(&s,5,0);assert(ds4_scope_active(&s,5,0,0));
 assert(ds4_local_attribute(5,ds4_scope_active(&s,5,0,0),0x1124,0x201,attr,sizeof(attr),NULL,0)==6);
 assert(ds4_local_attribute(5,ds4_scope_active(&s,5,0,0),0x1124,0x206,attr,sizeof(attr),NULL,0)==79);
 unsigned links=s.linked;assert(sony_target_prepare(&s,&p,5,ds5)==1&&s.linked==links); /* worker persistence must not erase link */
 assert(ds4_scope_active(&s,5,0,0)&&!ds4_scope_active(&s,4,0,0));
 ds4_scope_link(&s,5,3);assert(!ds4_scope_active(&s,5,0,0));
 ds4_scope_link(&s,5,0);assert(sony_target_prepare(&s,&p,5,ds5)==1&&ds4_scope_active(&s,5,0,0)); /* incoming event before selection */
 ZeePreferred saved=p;Ds4LocalScope previous=s;ds5[0]=84;
 assert(!sony_target_prepare(&s,&p,5,ds5)&&!memcmp(&p,&saved,sizeof(p))&&!memcmp(&s,&previous,sizeof(s)));ds5[0]=80;
 record_make(ds5,5,"Other Controller");assert(!sony_target_prepare(&s,&p,5,ds5));
 record_make(ds5,5,"DualSense Wireless Controller");ds5[7]=1;assert(!sony_target_prepare(&s,&p,5,ds5));
 record_make(ds5,5,"DualSense Wireless Controller");ds4_scope_link(&s,5,2);assert(sony_target_prepare(&s,&p,5,ds5)==1&&!ds4_scope_active(&s,5,0,0));
 puts("PASS: slot4 DS4 to slot5 DualSense selected before metadata, genuine-link gating, persistence and incoming-event order, disconnect/failure neutral, malformed/non-Sony identity isolation");
}
