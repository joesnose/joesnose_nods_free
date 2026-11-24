#include "game_reconnect.h"
#include <assert.h>
#include <stdio.h>
static unsigned queries,calls,match_slot=1,extra_slot,flags=0x0602,mutation;
static int status,was_active,last_slot,last_mode;
static const char *device_name="Zeemote: SteelSeries Free";
static int query(int slot,void *out){
 uint8_t *p=out;assert(p[0]==80&&!((uintptr_t)p&15));queries++;
 if((unsigned)slot!=match_slot&&(unsigned)slot!=extra_slot)return -1;
 p[4]=(uint8_t)slot;
 const char *n=device_name;for(unsigned i=0;i<=strlen(n);i++)p[6+i*2]=(uint8_t)n[i];
 p[72]=(uint8_t)flags;p[73]=(uint8_t)(flags>>8);
 if(mutation==1)p[6]='X';
 if(mutation==2)p[7]=1;
 if(mutation==3)p[4]=0;
 if(mutation==4)p[0]=84;
 if(mutation==5)p[5]=1;
 if(mutation==6)p[6]='z';
 if(mutation==7)p[6+2*strlen(n)]='!';
 return status;
}
static int connect(int slot,int mode){assert(!was_active);calls++;last_slot=slot;last_mode=mode;return -123;}
static void reset(void){queries=calls=extra_slot=mutation=was_active=0;match_slot=1;status=0;flags=0x0602;}
int main(void){
 reset();ZeeReconnect d=zee_game_reconnect(0,query,connect);
 assert(d.queries==8&&queries==8&&d.matches==1&&d.attempted&&calls==1&&last_slot==1&&last_mode==0&&d.result==-123);
 device_name="NES30 Pro";reset();flags=0x0422;d=zee_game_reconnect(0,query,connect);assert(calls==1&&last_mode==1);reset();flags=0x0622;d=zee_game_reconnect(0,query,connect);assert(calls==1&&last_mode==1);device_name="Zeemote: SteelSeries Free";
 for(unsigned slot=1;slot<=8;slot++){reset();match_slot=slot;flags=0x0402;d=zee_game_reconnect(0,query,connect);assert(calls==1&&last_slot==(int)slot&&last_mode==1);}
 reset();flags=0x0202;d=zee_game_reconnect(0,query,connect);assert(calls==1&&last_mode==0);
 for(unsigned i=1;i<=7;i++){reset();mutation=i;d=zee_game_reconnect(0,query,connect);assert(calls==((i==1||i==2||i==6||i==7)?1u:0u));}
 for(unsigned f=0;f<=65535;f++){
  reset();flags=f;d=zee_game_reconnect(0,query,connect);
  unsigned v=f&~0x20u;int expected=v==0x202||v==0x402||v==0x602;
  assert(calls==(unsigned)expected);if(expected)assert(last_mode==(f==0x402||f==0x422||f==0x622));
 }
 reset();extra_slot=2;d=zee_game_reconnect(0,query,connect);assert(d.matches==2&&!calls&&!d.attempted);
 reset();status=-1;d=zee_game_reconnect(0,query,connect);assert(!calls&&!d.matches);
 reset();flags=2;d=zee_game_reconnect(0,query,connect);assert(!calls);
 reset();was_active=1;d=zee_game_reconnect(1,query,connect);assert(!queries&&!calls);
 reset();d=zee_game_reconnect(0,NULL,connect);assert(!queries&&!calls);
 puts("PASS: fresh aligned eight-slot queries, any bounded UTF16 name, exact ID/size, native HID flags, unique target, native mode selection, active/ambiguous/error rejection and once-only original connection result");
}
