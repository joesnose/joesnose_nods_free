#include <assert.h>
#include <stdio.h>
#include "game_reconnect.h"
static uint8_t records[8][80];static int queries,calls,last_slot,last_mode,query_error;
static void make(unsigned slot,const char *name,unsigned flags){
 uint8_t *p=records[slot-1];memset(p,0,80);p[0]=80;p[4]=(uint8_t)slot;
 for(unsigned i=0;i<=strlen(name);i++){assert(i<33);p[6+i*2]=(uint8_t)name[i];}
 p[72]=(uint8_t)flags;p[73]=(uint8_t)(flags>>8);
 for(unsigned i=74;i<80;i++)p[i]=(uint8_t)(i+slot); /* synthetic nonzero fingerprint input */
}
static int query(int slot,void *buffer){
 uint8_t *p=buffer;assert(slot>=1&&slot<=8&&p[0]==80);queries++;
 for(unsigned i=1;i<80;i++)assert(!p[i]);
 if(query_error||!records[slot-1][0])return -1;
 memcpy(p,records[slot-1],80);return 0;
}
static int connect(int slot,int mode){calls++;last_slot=slot;last_mode=mode;return -1234;}
static void reset_counts(void){queries=calls=last_slot=last_mode=0;}
int main(void){
 make(1,"PLAYSTATION(R)3 Controller",0x0602);make(2,"Zeemote: SteelSeries Free",0x0602);make(3,"NES30 Pro",0x0622);make(4,"Wireless Controller",0x0402);
 ZeePreferred preferred;assert(zee_preferred_make(records[3],4,&preferred));
 records[3][72]=2;records[3][73]=6; /* observed reboot transition0402 ->0602 */
 assert(zee_preferred_matches(&preferred,records[3],4));
 reset_counts();ZeeReconnect d=zee_game_reconnect_preferred(0,&preferred,query,connect);
 assert(d.preferred&&d.matches==4&&d.slot==4&&d.flags==0x0602&&d.attempted&&d.result==-1234&&calls==1&&last_slot==4&&last_mode==1&&queries==8);
 NativeProfileDecision decision;reset_counts();
 assert(native_profile_connect(4,0,query,connect,&decision)==-1234&&calls==1&&queries==1&&decision.valid&&decision.selected_mode==1&&last_mode==1);
 reset_counts();assert(native_profile_connect(4,1,query,connect,&decision)==-1234&&calls==1&&!queries&&last_mode==1);
 reset_counts();assert(native_profile_connect(4,-7,query,connect,&decision)==-1234&&calls==1&&!queries&&last_mode==-7);
 for(unsigned slot=1;slot<=3;slot++){
  assert(zee_preferred_make(records[slot-1],slot,&preferred));reset_counts();d=zee_game_reconnect_preferred(0,&preferred,query,connect);
  assert(calls==1&&last_slot==(int)slot&&last_mode==(slot==3?1:0));
 }
 uint8_t *p=records[3];
 for(unsigned flags=0;flags<65536;flags++){
  p[72]=(uint8_t)flags;p[73]=(uint8_t)(flags>>8);
  assert(native_record_hid_mode(p,4)==(flags==0x0402||flags==0x0422||flags==0x0622||flags==0x0602));
 }
 const char *other_names[]={"Wireless Controller X","wireless controller","Wireless Controlle","Xbox Wireless Controller","Unknown"};
 for(unsigned i=0;i<sizeof(other_names)/sizeof(other_names[0]);i++){make(4,other_names[i],0x0602);assert(!native_record_hid_mode(records[3],4));}
 make(4,"Wireless Controller",0x0602);p=records[3];p[7]=1;assert(!native_record_hid_mode(p,4));p[7]=0;
 p[0]=84;assert(!native_record_hid_mode(p,4));p[0]=80;p[4]=3;assert(!native_record_hid_mode(p,4));p[4]=4;
 p[1]=1;assert(!native_record_hid_mode(p,4));p[1]=0;
 assert(!native_record_hid_mode(NULL,4)&&!native_record_hid_mode(p,0)&&!native_record_hid_mode(p,9));
 query_error=1;reset_counts();assert(native_profile_connect(4,0,query,connect,&decision)==-1234&&calls==1&&queries==1&&last_mode==0&&!decision.valid);query_error=0;
 memset(records[0],0,80);memset(records[1],0,80);memset(records[2],0,80);
 reset_counts();d=zee_game_reconnect(0,query,connect);assert(d.matches==1&&calls==1&&last_mode==1); /* unique saved Wireless Controller */
 assert(zee_preferred_make(records[3],4,&preferred));reset_counts();d=zee_game_reconnect_preferred(1,&preferred,query,connect);assert(!queries&&!calls&&!d.attempted);
 puts("PASS: observed Wireless Controller0402-to0602 preferred/unique/manual reconnect retains generic mode1, DS3/Free0602 remain legacy0, all flag words, exact bounded UTF16/ID/size validation, errors/explicit modes/one-attempt behavior");
}
