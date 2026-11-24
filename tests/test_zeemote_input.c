#define ZEE_INPUT_HOST_TEST
#include "../src/zeemote_input.c"
#define main vsh_fixture_main
#include "test_vsh_input_plan.c"
#undef main
static uint64_t time_us;static unsigned syscall_patches,code_writes;
static int original_result=1,native_calls,mutate_on_irq,alias_bad,missing_bridge,ark_present;
static unsigned app_mode=0x100;
static unsigned physical,firmware=0x06060010;
static uint32_t bridge_words[0x6000/4];
static uint32_t impose_words[2];
static int impose_present,impose_bad_stub;
static SceModule impose_module={0x88380000,0xc300};
static SceModule ark_module={0x88340000,0x8000};
static SceModule ctrl_module={0x880e6200,0x2850},bridge_module={0x88200000,0x6aa8};
unsigned sceKernelGetSystemTimeLow(void){return (unsigned)time_us;}
int sceKernelCpuSuspendIntr(void){if(mutate_on_irq){bridge_words[0x25c/4]^=1;mutate_on_irq=0;}return 0;}
void sceKernelCpuResumeIntr(int x){(void)x;}
void sceKernelDcacheWritebackAll(void){}void sceKernelIcacheInvalidateAll(void){}
SceModule *sceKernelFindModuleByName(const char *n){if(!strcmp(n,"sceController_Service"))return &ctrl_module;if(!strcmp(n,"sceImpose_Driver"))return impose_present?&impose_module:NULL;if(!strcmp(n,"VshCtrl"))return ark_present?&ark_module:NULL;assert(!strcmp(n,"sceVshBridge_Driver"));return missing_bridge?NULL:&bridge_module;}
unsigned sceKernelDevkitVersion(void){return firmware;}
unsigned sceKernelInitKeyConfig(void){return app_mode;}
uint32_t _lw(uint32_t a){if(a>=0x8838b290&&a<=0x8838b294)return impose_words[(a-0x8838b290)/4];assert(app_mode==0x100);assert(a>=vb&&a<vb+sizeof(bridge_words)&&!(a&3));return bridge_words[(a-vb)/4];}
void _sw(uint32_t w,uint32_t a){if(a==0x8838b290){impose_words[0]=w;code_writes++;return;}assert(app_mode==0x100);assert(a==vb+0x25c);bridge_words[(a-vb)/4]=w;code_writes++;}
uint32_t zee_test_code_address(const void *p){assert(p==(void *)vsh_read_pos||p==(void *)peek_pos);return hook_address;}
static int native_read(SceCtrlData *p,int n){native_calls++;if(original_result>0&&n>0){memset(p,0,sizeof(*p));p->Buttons=physical;p->Lx=128;p->Ly=128;}return original_result;}
uintptr_t sctrlHENFindFunction(const char *module,const char *library,uint32_t nid){
 if(!strcmp(module,"sceVshBridge_Driver")){assert(!strcmp(library,"sceVshBridge")&&nid==0xc6395c03);return vb+0x200;}
 assert(!strcmp(module,"sceController_Service"));
 const unsigned public_nids[]={0x3a622550,0xc152080a,0x1f803938,0x60b81f86},offsets[]={0x1ae0,0x1b00,0x1b20,0x1b40};
 for(unsigned i=0;i<4;i++)if(!strcmp(library,"sceCtrl")&&nid==public_nids[i])return ctrl_module.text_addr+offsets[i];
 assert(!strcmp(library,"sceCtrl_driver")&&nid==0xbe30ced0);return ctrl_read+(alias_bad?4:0);
}
void sctrlHENPatchSyscall(void *from,void *to){
 const unsigned offsets[]={0x1ae0,0x1b00,0x1b20,0x1b40};Read h[]={peek_pos,peek_neg,read_pos,read_neg};
 assert(syscall_patches<4&&(uintptr_t)from==ctrl_module.text_addr+offsets[syscall_patches]&&to==(void *)h[syscall_patches]);syscall_patches++;
}
uint32_t sctrlFindImportByNID(SceModule *m,char *lib,unsigned nid){assert(m==&impose_module&&!strcmp(lib,"sceCtrl_driver")&&nid==0x2ba616af);return impose_bad_stub?impose_module.text_addr+impose_module.text_size:0x8838b290;}
static void setup(void){
 overlay_installed=overlay_attempts=0;impose_present=impose_bad_stub=0;installed=vsh_installed=0;install_attempts=0;app_mode=0x100;syscall_patches=code_writes=0;ark_present=0;alias_bad=missing_bridge=mutate_on_irq=0;firmware=0x06060010;
 memset(bridge_words,0,sizeof(bridge_words));uint32_t c[34],s[2];fixture(c,s);memcpy(&bridge_words[0x200/4],c,sizeof(c));memcpy(&bridge_words[0x5608/4],s,sizeof(s));
}
#ifndef ZEE_INPUT_TEST_ENTRY
#define ZEE_INPUT_TEST_ENTRY main
#endif
int ZEE_INPUT_TEST_ENTRY(void){
 setup();uint32_t saved_stub[2];memcpy(saved_stub,&bridge_words[0x5608/4],sizeof(saved_stub));
 assert(zee_input_install()==0&&syscall_patches==4&&code_writes==1&&vsh_installed);
 assert(zee_input_install()==0&&syscall_patches==4&&code_writes==1);
 assert((uintptr_t)originals[4]==vb+0x5608&&!memcmp(saved_stub,&bridge_words[0x5608/4],sizeof(saved_stub)));
 for(unsigned i=0;i<34;i++){setup();bridge_words[0x200/4+i]^=1;assert(zee_input_install()<0&&syscall_patches==0&&code_writes==0);assert(install_attempts==4);}
 setup();alias_bad=1;assert(zee_input_install()<0&&!syscall_patches&&!code_writes);
 setup();firmware=0x06050010;assert(zee_input_install()<0&&!syscall_patches&&!code_writes);
 setup();mutate_on_irq=1;assert(zee_input_install()<0&&!syscall_patches&&!code_writes);
 setup();missing_bridge=1;for(unsigned i=0;i<6;i++)assert(zee_input_install()<0);assert(install_attempts==4&&!syscall_patches&&!code_writes);
 /* Real ARK topology: the unchanged import already jumps to VshCtrl,
  * not Ctrl. Accept only that resident-module range, preserve its words. */
 setup();ark_present=1;uint32_t ark_jump;
 assert(!vsh_input_jump(vb+0x5608,ark_module.text_addr+0x100,&ark_jump));
 bridge_words[0x5608/4]=ark_jump;
 assert(zee_input_install()==0&&code_writes==1&&syscall_patches==4);
 assert(bridge_words[0x5608/4]==ark_jump&&(uintptr_t)originals[4]==vb+0x5608);
 setup();bridge_words[0x5608/4]=ark_jump;assert(zee_input_install()<0&&!code_writes&&!syscall_patches);
 setup();ark_present=1;bridge_words[0x5608/4]=ark_jump;bridge_words[0x560c/4]=1;
 assert(zee_input_install()<0&&!code_writes&&!syscall_patches);
 setup();assert(zee_input_install()==0);
 /* GAME: no VSH bridge, no VshCtrl, no VSH instructions read or written.
  * Same four bounded controller aliases; preserve original return/merging. */
 setup();app_mode=0x200;missing_bridge=1;
 memset(bridge_words,0xa5,sizeof(bridge_words));
 assert(zee_input_install()==0&&installed&&!vsh_installed&&syscall_patches==4&&!code_writes);
 assert(zee_input_install()==0&&syscall_patches==4);
 for(unsigned i=0;i<4;i++)originals[i]=native_read;
 ZeeState game_state={.buttons=0x4000,.lx=0,.ly=255};zee_input_update(&game_state);
 SceCtrlData game_out;physical=8;native_calls=0;
 assert(read_pos(&game_out,1)==1&&native_calls==1&&game_out.Buttons==0x4008&&game_out.Lx==0&&game_out.Ly==255);
 physical=~8u;native_calls=0;
 assert(read_neg(&game_out,1)==1&&native_calls==1&&game_out.Buttons==~0x4008u);
 physical=8;native_calls=0;assert(peek_pos(&game_out,1)==1&&native_calls==1&&game_out.Buttons==0x4008);
 physical=~8u;native_calls=0;assert(peek_neg(&game_out,1)==1&&native_calls==1&&game_out.Buttons==~0x4008u);
 original_result=-7;native_calls=0;assert(read_pos(&game_out,1)==-7&&native_calls==1);original_result=1;
 setup();app_mode=0x200;missing_bridge=1;alias_bad=1;assert(zee_input_install()<0&&!syscall_patches&&!code_writes);
 setup();app_mode=0x300;assert(zee_input_install()<0&&install_error==7&&!syscall_patches&&!code_writes);
 setup();assert(zee_input_install()==0);
 setup();app_mode=0x200;missing_bridge=1;impose_present=1;assert(zee_input_install()==0);
 assert(!vsh_input_jump(0x8838b290,(uint32_t)(uintptr_t)originals[0],&impose_words[0]));impose_words[1]=0;
 uint32_t saved_entry=(uint32_t)(uintptr_t)originals[0];
 assert(zee_input_install_overlay()==0&&overlay_installed&&code_writes==1);
 assert(zee_input_install_overlay()==0&&code_writes==1&&(uint32_t)(uintptr_t)originals[0]==saved_entry);
 originals[0]=native_read;ZeeState menu_state={.buttons=0x4020,.lx=128,.ly=128};zee_input_update(&menu_state);
 SceCtrlData menu_out;physical=8;native_calls=0;
 assert(peek_pos(&menu_out,1)==1&&native_calls==1&&menu_out.Buttons==0x4028);
 setup();app_mode=0x200;impose_present=1;assert(zee_input_install()==0);
 assert(!vsh_input_jump(0x8838b290,(uint32_t)(uintptr_t)originals[0],&impose_words[0]));impose_words[1]=1;
 assert(zee_input_install_overlay()<0&&!code_writes&&overlay_attempts==4);
 setup();app_mode=0x200;assert(zee_input_install()==0);
 for(unsigned tries=0;tries<8;tries++)assert(zee_input_install_overlay()<0);
 assert(overlay_attempts==4&&!code_writes);
 setup();app_mode=0x200;impose_present=1;assert(zee_input_install()==0);impose_words[0]=0;impose_words[1]=0;
 assert(zee_input_install_overlay()<0&&!code_writes&&overlay_attempts==4);
 setup();app_mode=0x200;impose_present=1;impose_bad_stub=1;assert(zee_input_install()==0);
 assert(zee_input_install_overlay()<0&&!code_writes);
 setup();assert(zee_input_install()==0);
 /* Host-native mock stands in for unchanged target functions/import stub. */
 for(unsigned i=0;i<5;i++)originals[i]=native_read;
 ZeeState state={.buttons=0x4000,.lx=0,.ly=255};zee_input_update(&state);SceCtrlData out;
 physical=8;native_calls=0;assert(vsh_read_pos(&out,1)==1&&native_calls==1&&out.Buttons==0x4008&&out.Lx==0&&out.Ly==255);
 physical=~8u;native_calls=0;assert(read_neg(&out,1)==1&&native_calls==1&&out.Buttons==~0x4008u);
 state.lx=state.ly=128;zee_input_update(&state);physical=8;assert(read_pos(&out,1)==1&&out.Lx==128&&out.Ly==128);
 time_us=1000000;zee_input_keepalive();time_us=2000000;assert(vsh_read_pos(&out,1)==1&&out.Buttons==0x4008);
 time_us=2500000;assert(vsh_read_pos(&out,1)==1&&out.Buttons==8);time_us=0;zee_input_update(&state);zee_input_clear();assert(peek_pos(&out,1)==1&&out.Buttons==8);
 /* Low32 timer wrap, expiry and aggregate read-age integration. */
 time_us=0xfffffff0u;zee_input_update(&state);time_us=0x100000020ULL;
 assert(peek_pos(&out,1)==1&&out.Buttons==0x4008);
 ZeeInputTiming first;zee_input_timing(&first);assert(first.reports&&first.samples&&first.age_max_us>=48);
 ZeeInputTiming empty;zee_input_timing(&empty);assert(!empty.reports&&!empty.samples);
 time_us+=1000;zee_input_update(&state);time_us+=250;assert(peek_pos(&out,1)==1);
 ZeeInputTiming measured;zee_input_timing(&measured);assert(measured.reports==1&&measured.gaps==1&&measured.gap_max_us==1048&&measured.samples==1&&measured.age_max_us==250);
 time_us+=1600000;assert(peek_pos(&out,1)==1&&out.Buttons==8);
 zee_input_timing(&measured);assert(measured.stale==1&&!measured.samples);
 zee_input_clear();time_us+=5000000;zee_input_update(&state);zee_input_timing(&measured);assert(measured.reports==1&&!measured.gaps);
 original_result=-7;native_calls=0;assert(vsh_read_pos(&out,1)==-7&&native_calls==1);original_result=0;assert(read_pos(&out,1)==0);
 ZeeInputStats stats;zee_input_stats(&stats);assert(stats.vsh_installed&&stats.vsh_reads>=4);
 /* Real input publication/merge: only DS4 GAME dispatch consumes HOME. */
 original_result=1;physical=0;zee_input_home_enable(1);zee_input_clear();
 ZeeState ps={.buttons=0x10000,.lx=128,.ly=128};zee_input_update_kind(&ps,1);
 assert(!zee_input_home_take());assert(read_pos(&out,1)==1&&!(out.Buttons&0x10000));
 ps.buttons=0;zee_input_update_kind(&ps,1);ps.buttons=0x10008;zee_input_update_kind(&ps,1);
 assert(zee_input_home_take()&&!zee_input_home_take());assert(read_pos(&out,1)==1&&out.Buttons==8);
 zee_input_update_kind(&ps,1);assert(!zee_input_home_take());
 ps.buttons=0;zee_input_update_kind(&ps,1);ps.buttons=0x10000;zee_input_update_kind(&ps,1);zee_input_clear();assert(!zee_input_home_take());
 zee_input_update(&ps);assert(!zee_input_home_take());assert(read_pos(&out,1)==1&&(out.Buttons&0x10000)); /* other controllers unchanged */
 zee_input_home_enable(0);zee_input_update_kind(&ps,1);assert(!zee_input_home_take());assert(read_pos(&out,1)==1&&(out.Buttons&0x10000));
 ZeeState small_axis={.lx=123,.ly=130};zee_input_update_kind(&small_axis,1);assert(read_pos(&out,1)==1&&out.Lx==123&&out.Ly==130);
 zee_input_update(&small_axis);assert(read_pos(&out,1)==1&&out.Lx==128&&out.Ly==128); /* generic threshold unchanged */
 puts("PASS: guarded Impose direct positive-peek import, menu merge, once-only original, unchanged Ctrl chain, delay/target/bounds refusal and bounded absence retries; GAME positive/negative read/peek and original errors with all VSH memory accesses forbidden; VSH four syscall-table redirects plus one verified call write only; unchanged import/current-export chain, all-context/alias/firmware/race fail-closed guards, bounded attempts, fifth-route positive merge, once-only original errors and stale/disconnect neutral");
 return 0;
}
