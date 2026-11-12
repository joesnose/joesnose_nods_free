#include <assert.h>
#include <stdio.h>
#include "game_reconnect.h"
static unsigned queries,calls,last_slot,last_mode,mutation,only_other;
static unsigned target_flags=0x622;
static void record(uint8_t p[80],unsigned slot){
 memset(p,0,80);p[0]=80;p[4]=(uint8_t)slot;
 const char *name=slot==2?"NES30 Pro":slot==1?"Free":"GameStick";
 for(unsigned i=0;i<strlen(name);i++)p[6+2*i]=(uint8_t)name[i];
 unsigned flags=slot==2?target_flags:0x602;p[72]=(uint8_t)flags;p[73]=(uint8_t)(flags>>8);
 for(unsigned i=0;i<6;i++)p[74+i]=(uint8_t)(slot*10+i);
}
static int query(int slot,void *out){
 assert(!((uintptr_t)out&15));++queries;
 if(only_other&&(unsigned)slot!=only_other)return -1;
 if(slot>3)return -1;
 record(out,(unsigned)slot);uint8_t *p=out;
 if(slot==2){if(mutation==1)p[74]^=1;if(mutation==2)p[6]='X';if(mutation==3)return -1;if(mutation==4)p[40]=77;}
 return 0;
}
static int connect(int slot,int mode){++calls;last_slot=(unsigned)slot;last_mode=(unsigned)mode;return -123;}
static void reset(void){queries=calls=mutation=only_other=0;target_flags=0x622;}
int main(void){
 uint8_t p[80];record(p,2);ZeePreferred pref;assert(zee_preferred_make(p,2,&pref)&&zee_preferred_valid(&pref));
 reset();ZeeReconnect d=zee_game_reconnect_preferred(0,&pref,query,connect);
 assert(d.matches==3&&d.preferred&&calls==1&&queries==8&&last_slot==2&&last_mode==1&&d.result==-123);
 reset();target_flags=0x422;d=zee_game_reconnect_preferred(0,&pref,query,connect);assert(calls==1&&last_mode==1);
 for(unsigned i=1;i<=3;i++){reset();mutation=i;d=zee_game_reconnect_preferred(0,&pref,query,connect);assert(!calls&&!d.preferred&&!d.attempted);}
 reset();mutation=4;d=zee_game_reconnect_preferred(0,&pref,query,connect);assert(calls==1); /* ignored name padding */
 reset();only_other=1;d=zee_game_reconnect_preferred(0,&pref,query,connect);assert(d.matches==1&&!calls); /* never silently substitute */
 for(unsigned i=0;i<sizeof(pref);i++){
  ZeePreferred bad=pref;((uint8_t *)&bad)[i]^=1;assert(!zee_preferred_valid(&bad));
 }
 reset();d=zee_game_reconnect_preferred(1,&pref,query,connect);assert(!queries&&!calls);
 reset();d=zee_game_reconnect_preferred(0,NULL,query,connect);assert(d.matches==3&&!calls);
 reset();only_other=2;d=zee_game_reconnect_preferred(0,NULL,query,connect);assert(calls==1&&last_slot==2&&last_mode==1);
 reset();target_flags=0x8622;d=zee_game_reconnect_preferred(0,&pref,query,connect);assert(!calls);
 record(p,2);memset(p+74,0,6);ZeePreferred invalid;assert(!zee_preferred_make(p,2,&invalid));
 memset(p+74,255,6);assert(!zee_preferred_make(p,2,&invalid));
 puts("PASS: preferred NES30 selected among3 devices; fresh identity/name/address checks, mode/flag changes, padding, invalid checksum, missing/reassigned target, active rejection and once-only original result");
}
