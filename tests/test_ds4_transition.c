#include <assert.h>
#include <stdio.h>
#include "ds4_local.h"
#include "ds4_transition.h"
static unsigned context,connects;static int connected_mode,result_value;
static uint8_t saved[80];static ZeePreferred preferred;static Ds4LocalScope scope;
static int query(int slot,void *buffer){if(slot!=4)return -1;memcpy(buffer,saved,80);ds4_scope_observe(&scope,4,buffer,0,&preferred);return 0;}
static int selected(unsigned slot){return ds4_scope_selected(&scope,slot,0,0);}
static int defer(unsigned slot){return ds4_transition_defer(context,selected(slot),ds4_scope_active(&scope,slot,0,0));}
static int connect_controller(int slot,int mode){assert(slot==4);connects++;connected_mode=ds4_transition_mode(context,selected(slot),mode);return result_value;}
int main(void){
 saved[0]=80;saved[4]=4;saved[72]=2;saved[73]=6;const char name[]="Wireless Controller";
 for(unsigned i=0;i<sizeof(name);i++)saved[6+i*2]=(uint8_t)name[i];
 for(unsigned i=74;i<80;i++)saved[i]=(uint8_t)(i+1);
 assert(zee_preferred_make(saved,4,&preferred));
 context=0x100;ZeeReconnect d=zee_game_reconnect_policy(0,&preferred,query,connect_controller,defer);
 assert(d.attempted&&!d.deferred&&connects==1&&connected_mode==1); /* saved-profile path in VSH */
 connects=0;
 context=0x200;result_value=-42;d=zee_game_reconnect_policy(0,&preferred,query,connect_controller,defer);
 assert(d.attempted&&!d.deferred&&d.result==-42&&connects==1&&connected_mode==1);
 assert(!ds4_scope_active(&scope,4,0,0)); /* errors never create a link */
 uint8_t buf[100];assert(!ds4_local_attribute(4,ds4_scope_active(&scope,4,0,0),0x1124,0x206,buf,sizeof(buf),NULL,0));
 ds4_scope_link(&scope,4,0);assert(ds4_scope_active(&scope,4,0,0));
 assert(ds4_local_attribute(4,ds4_scope_active(&scope,4,0,0),0x1124,0x206,buf,sizeof(buf),NULL,0)==79);
 d=zee_game_reconnect_policy(0,&preferred,query,connect_controller,defer);assert(d.deferred&&connects==1); /* incoming link not raced */
 d=zee_game_reconnect_policy(1,&preferred,query,connect_controller,defer);assert(!d.queries&&connects==1);
 ds4_scope_link(&scope,4,3);assert(!ds4_scope_active(&scope,4,0,0));
 saved[74]^=1;d=zee_game_reconnect_policy(0,&preferred,query,connect_controller,defer);assert(!d.slot&&!d.attempted&&connects==1);
 for(unsigned ctx=0;ctx<=0x300;ctx++)for(int mode=-2;mode<=2;mode++)assert(ds4_transition_mode(ctx,0,mode)==mode);
 assert(ds4_transition_mode(0x100,1,1)==1&&ds4_transition_mode(0x200,1,1)==1);
 assert(!ds4_scope_selected(&scope,4,0x054c,0x0268));
 puts("PASS: preferred Sony VSH/GAME saved-profile reconnect, native errors preserved, real-link metadata authorization, incoming-link/active rejection and stale target isolation");
}
