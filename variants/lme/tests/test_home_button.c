#include <assert.h>
#include <stdio.h>
#include "home_button.h"
int main(void){
 ZeeHomeButton h={0};
 zee_home_report(&h,1,1,0);assert(!zee_home_take(&h,1,1,1)); /* connecting PS hold */
 zee_home_report(&h,1,0,10);zee_home_report(&h,1,1,20);
 assert(zee_home_take(&h,1,1,25)&&!zee_home_take(&h,1,1,26));
 zee_home_report(&h,1,1,30);assert(!zee_home_take(&h,1,1,31)); /* no repeats */
 zee_home_report(&h,1,0,40);zee_home_report(&h,1,1,50);zee_home_report(&h,1,0,55);
 assert(zee_home_take(&h,1,1,60)); /* short tap survives until worker */
 zee_home_report(&h,1,1,70);assert(!zee_home_take(&h,1,1,250070));
 zee_home_report(&h,1,0,80);zee_home_report(&h,1,1,90);zee_home_reset(&h);assert(!zee_home_take(&h,1,1,91));
 zee_home_report(&h,1,0,0xfffffff0u);zee_home_report(&h,1,1,0xfffffff8u);assert(zee_home_take(&h,1,1,0x10));
 zee_home_report(&h,0,0,0);zee_home_report(&h,0,1,10);assert(!zee_home_take(&h,1,1,11));
 zee_home_report(&h,1,0,20);zee_home_report(&h,1,1,30);assert(!zee_home_take(&h,1,0,31));
 uint32_t base=0x88380000,entry=base+0x4078;
 uint32_t w[HOME_GUARDS]={0x27bdfff0,0xafb00000,0x03608021,0xafbf0004,0x8cc50074,0x30a30001,0x10600006,0x001bdac0,0x0200d821,0x8fbf0004,0x8fb00000,0x03e00008,0x27bd0010,0,0,0x0200d821};
 w[13]=0x0c000000u|(((base+0x1a58)>>2)&0x3ffffffu);
 assert(zee_home_context(base,0xc300,entry,w));
 for(unsigned i=0;i<HOME_GUARDS;i++){uint32_t old=w[i];w[i]=0xffffffff;assert(!zee_home_context(base,0xc300,entry,w));w[i]=old;}
 assert(!zee_home_context(base,0x40c0,entry,w));assert(!zee_home_context(base,0xc300,entry+1,w));
 w[13]=0x0c000000u|(((base+0xc304)>>2)&0x3ffffffu);assert(!zee_home_context(base,0xc300,entry,w));
 uint32_t sw[4]={0x3c038838,0x8c64c2d0,0x03e00008,0x8c8200a4};
 assert(zee_home_status_context(base,0xc300,base+0x3f64,sw));
 for(unsigned i=0;i<4;i++){uint32_t old=sw[i];sw[i]=0xffffffff;assert(!zee_home_status_context(base,0xc300,base+0x3f64,sw));sw[i]=old;}
 for(unsigned status=0;status<65536;status++)assert(zee_home_open_argument(status)==!(status&0x100));
 puts("PASS: PS connect-hold suppression, press/release/tap/hold, one native press per edge, disconnect/stale/disabled rejection, timer wrap and native HOME ABI/context guards");
}
