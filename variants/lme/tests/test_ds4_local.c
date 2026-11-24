#include <assert.h>
#include <stdio.h>
#include "../src/zeemote_hid.c"
#include "ds4_local.h"
static uint8_t saved[80];static ZeePreferred pref;static Ds4LocalScope scope;static unsigned connects;
static int query(int slot,void *p){if(slot!=4)return -1;memcpy(p,saved,80);ds4_scope_observe(&scope,4,p,0,&pref);return 0;}
static int connect_native(int slot,int mode){assert(slot==4&&mode==1);connects++;return -123;}
static int defer(unsigned slot){return ds4_scope_selected(&scope,slot,0,0);}
int main(void){
 saved[0]=80;saved[4]=4;saved[72]=2;saved[73]=6;const char *name="Wireless Controller";
 for(unsigned i=0;i<=strlen(name);i++)saved[6+i*2]=(uint8_t)name[i];
 for(unsigned i=74;i<80;i++)saved[i]=(uint8_t)i;
 assert(zee_preferred_make(saved,4,&pref));
 ZeeReconnect d=zee_game_reconnect_policy(0,&pref,query,connect_native,defer);
 assert(d.deferred&&!d.attempted&&!connects&&scope.selected==8);
 assert(!ds4_scope_active(&scope,4,0,0));ds4_scope_link(&scope,4,0);assert(ds4_scope_active(&scope,4,0,0));
 assert(ds4_scope_active(&scope,4,0x054c,0x09cc));assert(!ds4_scope_active(&scope,4,0x054c,0x0268));assert(!ds4_scope_active(&scope,4,0x2dc8,0x3820));
 uint8_t b[128];memset(b,0xa5,sizeof(b));assert(ds4_local_attribute(4,1,0x1124,0x201,b,6,NULL,0)==6);assert(!memcmp(b,"\x09\x02\x01\x09\x01\x11",6)&&b[6]==0xa5);
 assert(ds4_local_attribute(4,1,0x1124,0x203,b,5,NULL,0)==5&&b[4]==0);
 assert(ds4_local_attribute(4,1,0x1124,0x206,b,128,NULL,0)==79);
 ZeeLayout l;assert(!zee_parse_descriptor(b+11,b[10],&l)&&l.report_id==1&&l.payload_bytes==9);
 assert(!ds4_local_attribute(4,0,0x1124,0x206,b,128,NULL,0));assert(!ds4_local_attribute(4,1,0x1200,0x206,b,128,NULL,0));
 assert(!ds4_local_attribute(4,1,0x1124,0x206,b,78,NULL,0));assert(!ds4_local_attribute(4,1,0x1124,0x201,b,5,NULL,0));
 assert(!ds4_local_attribute(4,1,0x1124,0x203,b,4,NULL,0));assert(!ds4_local_attribute(4,1,0x1124,0x206,b,128,b,0));assert(!ds4_local_attribute(4,1,0x1124,0x206,b,128,NULL,1));
 ds4_scope_link(&scope,4,3);assert(!ds4_scope_active(&scope,4,0,0));ds4_scope_link(&scope,4,0);ds4_scope_link(&scope,4,2);assert(!ds4_scope_active(&scope,4,0,0));
 saved[74]^=1;ds4_scope_observe(&scope,4,saved,0,&pref);assert(!scope.selected);saved[74]^=1;
 ds4_scope_observe(&scope,4,saved,-1,&pref);assert(!scope.selected);ds4_scope_observe(&scope,4,saved,0,&pref);assert(scope.selected==8);
 assert(ds4_hid_context_index(0x8822f800,0x88234734)==1&&ds4_hid_context_index(0x8822f800,0x88234744)==2);
 assert(!ds4_hid_context_index(0x8822f800,0x88234738)&&!ds4_hid_context_index(0xfffff000,0));
 assert(ds4_backend_index(0x88239200,0x88247080)==1&&ds4_backend_index(0x88239200,0x882470b0)==2);
 assert(!ds4_backend_index(0x88239200,0x88247084)&&!ds4_backend_index(0xfffff000,0));
 uint32_t w[LOCAL_GUARDS];ds4_local_expected(0x88239200,0x88401000,0x88402000,w);
 assert(ds4_local_context(0x88239200,0xd000,0x88401000,0x88402000,w));
 for(unsigned i=0;i<LOCAL_GUARDS;i++){w[i]^=1;assert(!ds4_local_context(0x88239200,0xd000,0x88401000,0x88402000,w));w[i]^=1;}
 assert(!ds4_local_context(0x88239200,0xac2b,0x88401000,0x88402000,w));
 assert(!ds4_local_context(0x88239200,0xd000,0x08401000,0x88402000,w));
 puts("PASS: selected fingerprint + real-link gating, cached non-DS4 refusal, passive auto selection, disconnect/error invalidation, local parser/country/descriptor metadata, context/backend bounds and all40 guarded native instructions");
}
