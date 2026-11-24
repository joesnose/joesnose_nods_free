/* Original PSP controller merge adapter, MIT. Hooks remain resident until reboot. */
#ifdef ZEE_INPUT_HOST_TEST
#include "zeemote_input_mock.h"
typedef struct {unsigned TimeStamp,Buttons;unsigned char Lx,Ly,Rsrv[6];} SceCtrlData;
void sctrlHENPatchSyscall(void *,void *);
void sceKernelDcacheWritebackAll(void);
void sceKernelIcacheInvalidateAll(void);
#else
#include <pspkernel.h>
#include <pspctrl.h>
#include <psploadcore.h>
#include <pspsdk.h>
#include <pspinit.h>
#include <systemctrl.h>
#endif
#include <string.h>
#include "zeemote_input.h"
#include "vsh_input_plan.h"
#include "home_button.h"
static ZeeState remote;
static unsigned remote_sony;
static ZeeHomeButton home_button;static int home_enabled;
static unsigned last_us,last_report_us,has_report;
static ZeeInputTiming timing;
void zee_input_timing(ZeeInputTiming *out){if(!out)return;int irq=sceKernelCpuSuspendIntr();*out=timing;memset(&timing,0,sizeof(timing));sceKernelCpuResumeIntr(irq);}
static int active,installed;
static unsigned read_counts[5],merged_samples,install_attempts,vsh_installed;
static unsigned install_error,guard_index=~0u;
static unsigned overlay_installed,overlay_attempts;
static unsigned seen_buttons,motion_seen,min_lx=255,max_lx,min_ly=255,max_ly;
void zee_input_stats(ZeeInputStats *out){if(!out)return;int irq=sceKernelCpuSuspendIntr();out->overlay_installed=overlay_installed;out->installed=installed;out->vsh_installed=vsh_installed;out->vsh_reads=read_counts[4];out->active=active;memcpy(out->reads,read_counts,4*sizeof(read_counts[0]));out->merged_samples=merged_samples;out->install_error=install_error;out->guard_index=guard_index;out->seen_buttons=seen_buttons;out->motion_seen=motion_seen;out->last_buttons=remote.buttons;out->lx=remote.lx;out->ly=remote.ly;out->min_lx=min_lx;out->max_lx=max_lx;out->min_ly=min_ly;out->max_ly=max_ly;sceKernelCpuResumeIntr(irq);}
/* Installation errors:1 firmware,2 module,3 export bounds/alias,4 VSH ABI,
 * 5 module changed,6 context changed,7 unsupported application type. Installation is worker-only, bounded. */
static int install_fail(unsigned error,unsigned index){install_error=error;guard_index=index;return -1;}
typedef int (*Read)(SceCtrlData *,int);
static Read originals[5];
static uint32_t code_address(const void *p){
#ifdef ZEE_INPUT_HOST_TEST
 return zee_test_code_address(p);
#else
 return (uint32_t)(uintptr_t)p;
#endif
}
/* Low32 microseconds avoid a 64-bit division on every report and read.
 * Unsigned elapsed arithmetic remains valid across the timer's wrap. */
static unsigned now_us(void){return sceKernelGetSystemTimeLow();}
void zee_input_update_kind(const ZeeState *s,unsigned ds4){int irq=sceKernelCpuSuspendIntr();remote_sony=ds4;remote=*s;seen_buttons|=s->buttons;motion_seen|=(s->lx<112?1u:0u)|(s->lx>144?2u:0u)|(s->ly<112?4u:0u)|(s->ly>144?8u:0u);if(s->lx<min_lx)min_lx=s->lx;if(s->lx>max_lx)max_lx=s->lx;if(s->ly<min_ly)min_ly=s->ly;if(s->ly>max_ly)max_ly=s->ly;unsigned stamp=now_us();
 if(has_report){unsigned gap=stamp-last_report_us;if(gap>timing.gap_max_us)timing.gap_max_us=gap;if(gap<=1500000u){timing.gaps++;timing.gap_total_us+=gap;}}
 zee_home_report(&home_button,home_enabled&&ds4,!!(s->buttons&0x10000u),stamp);
 if(home_enabled&&ds4)remote.buttons&=~0x10000u;
 last_report_us=stamp;has_report=1;timing.reports++;last_us=stamp;active=1;sceKernelCpuResumeIntr(irq);}
void zee_input_update(const ZeeState *s){zee_input_update_kind(s,0);}
void zee_input_home_enable(int enabled){int irq=sceKernelCpuSuspendIntr();home_enabled=enabled;zee_home_reset(&home_button);sceKernelCpuResumeIntr(irq);}
int zee_input_home_take(void){int irq=sceKernelCpuSuspendIntr();unsigned now=now_us();int result=zee_home_take(&home_button,home_enabled,active&&(unsigned)(now-last_us)<1500000u,now);sceKernelCpuResumeIntr(irq);return result;}
void zee_input_keepalive(void){int irq=sceKernelCpuSuspendIntr();if(active)last_us=now_us();sceKernelCpuResumeIntr(irq);}
int zee_input_session_live(void){int irq=sceKernelCpuSuspendIntr();int live=active&&has_report&&(unsigned)(now_us()-last_us)<1500000u;sceKernelCpuResumeIntr(irq);return live;}
void zee_input_clear(void){int irq=sceKernelCpuSuspendIntr();active=0;has_report=0;remote_sony=0;zee_home_reset(&home_button);memset(&remote,0,sizeof(remote));sceKernelCpuResumeIntr(irq);}
static int merge(unsigned index,SceCtrlData *p,int count){
 int irq_count=sceKernelCpuSuspendIntr();read_counts[index]++;sceKernelCpuResumeIntr(irq_count);
 int result=originals[index](p,count);if(result<=0||result>count||result>64)return result;
 ZeeState s;int irq=sceKernelCpuSuspendIntr();unsigned stamp=now_us();int valid=active&&(unsigned)(stamp-last_us)<1500000u;s=remote;unsigned sony=remote_sony;
 if(active&&has_report){unsigned age=stamp-last_report_us;if(age>timing.age_max_us)timing.age_max_us=age;if(age<1500000u){timing.samples++;timing.age_total_us+=age;}else timing.stale++;}sceKernelCpuResumeIntr(irq);
 if(!valid)return result;
 irq=sceKernelCpuSuspendIntr();merged_samples+=(unsigned)result;sceKernelCpuResumeIntr(irq);
 for(int i=0;i<result;i++){
  if(index&1)p[i].Buttons&=~s.buttons;else p[i].Buttons|=s.buttons;
  if(s.lx<(sony?128u:112u)||s.lx>(sony?128u:144u))p[i].Lx=s.lx;
  if(s.ly<(sony?128u:112u)||s.ly>(sony?128u:144u))p[i].Ly=s.ly;
 }
 return result;
}
static int peek_pos(SceCtrlData *p,int n){return merge(0,p,n);}
static int peek_neg(SceCtrlData *p,int n){return merge(1,p,n);}
static int read_pos(SceCtrlData *p,int n){return merge(2,p,n);}
static int read_neg(SceCtrlData *p,int n){return merge(3,p,n);}
static int vsh_read_pos(SceCtrlData *p,int n){return merge(4,p,n);}
int zee_input_install(void){
 if(installed)return 0;
 if(install_attempts>=4)return -1;
 ++install_attempts;
 unsigned fw=sceKernelDevkitVersion();if(fw!=0x06060010u&&fw!=0x06060110u){install_attempts=4;return install_fail(1,~0u);}
 static const unsigned nids[]={0x3a622550,0xc152080a,0x1f803938,0x60b81f86};
 Read hooks[]={peek_pos,peek_neg,read_pos,read_neg};
 SceModule *ctrl=sceKernelFindModuleByName("sceController_Service");
 SceModule *bridge=sceKernelFindModuleByName("sceVshBridge_Driver");
 if(!ctrl)return install_fail(2,~0u);
 unsigned mode=sceKernelInitKeyConfig();
 if(mode!=0x100u&&mode!=0x200u){install_attempts=4;return install_fail(7,mode);}
 uint32_t cb=ctrl->text_addr,cs=ctrl->text_size;
 if(cb>0xffffffffu-cs||cs<8)return install_fail(3,~0u);
 Read targets[5];
 for(unsigned i=0;i<4;i++){
  uint32_t entry=(uint32_t)sctrlHENFindFunction("sceController_Service","sceCtrl",nids[i]);
  if((entry&3)||entry<cb||entry-cb>cs-8)return install_fail(3,i);
  targets[i]=(Read)(uintptr_t)entry;
 }
 uint32_t alias=(uint32_t)sctrlHENFindFunction("sceController_Service","sceCtrl_driver",0xbe30ced0);
 if(alias!=(uint32_t)(uintptr_t)targets[2])return install_fail(3,2);
 /* Games call the four public controller syscalls. VSH bridge and VshCtrl
  * are not prerequisites here. Preserve ARK's current Ctrl entry chain. */
 if(mode==0x200u){
  int irq=sceKernelCpuSuspendIntr();
  if(installed){sceKernelCpuResumeIntr(irq);return 0;}
  if(ctrl->text_addr!=cb||ctrl->text_size!=cs){sceKernelCpuResumeIntr(irq);return install_fail(5,~0u);}
  memcpy(originals,targets,4*sizeof(targets[0]));
  for(unsigned i=0;i<4;i++)sctrlHENPatchSyscall((void *)targets[i],(void *)hooks[i]);
  sceKernelDcacheWritebackAll();sceKernelIcacheInvalidateAll();
  installed=1;install_error=0;guard_index=~0u;
  sceKernelCpuResumeIntr(irq);return 0;
 }
 if(!bridge)return install_fail(2,~0u);
 uint32_t b=bridge->text_addr,size=bridge->text_size;
 if(b>0xffffffffu-size||size<0x5610u)return install_fail(3,~0u);
 uint32_t exported=(uint32_t)sctrlHENFindFunction("sceVshBridge_Driver","sceVshBridge",0xc6395c03);
 SceModule *ark=sceKernelFindModuleByName("VshCtrl");
 uint32_t ab=ark?ark->text_addr:0,as=ark?ark->text_size:0;
 uint32_t snapshot[VSH_INPUT_CONTEXT_WORDS],stub[2],patched,stub_target;
 for(unsigned i=0;i<VSH_INPUT_CONTEXT_WORDS;i++)snapshot[i]=_lw(b+0x200+i*4);
 stub[0]=_lw(b+0x5608);stub[1]=_lw(b+0x560c);
 if(vsh_input_stub_target(b,(uint32_t)(uintptr_t)targets[2],ab,as,stub,&stub_target)){
  install_attempts=4;return install_fail(4,5);
 }
 int plan=vsh_input_plan(b,size,exported,stub_target,code_address((void *)vsh_read_pos),snapshot,stub,&patched);
 if(plan){install_attempts=4;return install_fail(4,(unsigned)-plan);}
 targets[4]=(Read)(uintptr_t)(b+0x5608);
 int irq=sceKernelCpuSuspendIntr();
 if(installed){sceKernelCpuResumeIntr(irq);return 0;}
 if((ark&&(ark->text_addr!=ab||ark->text_size!=as))||ctrl->text_addr!=cb||ctrl->text_size!=cs||bridge->text_addr!=b||bridge->text_size!=size){sceKernelCpuResumeIntr(irq);return install_fail(5,~0u);}
 for(unsigned i=0;i<VSH_INPUT_CONTEXT_WORDS;i++)if(_lw(b+0x200+i*4)!=snapshot[i]){
  sceKernelCpuResumeIntr(irq);return install_fail(6,4);
 }
 if(_lw(b+0x5608)!=stub[0]||_lw(b+0x560c)!=stub[1]){sceKernelCpuResumeIntr(irq);return install_fail(6,4);}
 memcpy(originals,targets,sizeof(originals));
 /* These replace syscall-table references only. Current Ctrl entry detours
  * and the bridge's original direct import remain untouched. */
 for(unsigned i=0;i<4;i++)sctrlHENPatchSyscall((void *)targets[i],(void *)hooks[i]);
 _sw(patched,b+0x25c);
 sceKernelDcacheWritebackAll();sceKernelIcacheInvalidateAll();installed=vsh_installed=1;install_error=0;guard_index=~0u;
 sceKernelCpuResumeIntr(irq);return 0;
}

/* Impose uses a direct kernel import of ordinary two-argument positive peek.
 * Change only that validated J/NOP stub; keep the original Ctrl/ARK entry. */
int zee_input_install_overlay(void){
 if(overlay_installed)return 0;
 if(!installed||sceKernelInitKeyConfig()!=0x200u)return -1;
 if(overlay_attempts>=4)return -1;
 ++overlay_attempts;
 SceModule *m=sceKernelFindModuleByName("sceImpose_Driver");
 SceModule *ctrl=sceKernelFindModuleByName("sceController_Service");
 if(!m||!ctrl)return -1;
 uint32_t b=m->text_addr,size=m->text_size;
 if(size<8||b>0xffffffffu-size)return -1;
 uint32_t stub=sctrlFindImportByNID(m,"sceCtrl_driver",0x2ba616afu);
 uint32_t target=(uint32_t)(uintptr_t)originals[0],old_word,new_word;
 if(!stub||(stub&3)||stub<b||stub-b>size-8||
    target!=sctrlHENFindFunction("sceController_Service","sceCtrl",0x3a622550u)||
    vsh_input_jump(stub,target,&old_word)||
    vsh_input_jump(stub,code_address((void *)peek_pos),&new_word)){
  overlay_attempts=4;return -1;
 }
 int irq=sceKernelCpuSuspendIntr();
 if(m->text_addr!=b||m->text_size!=size||_lw(stub)!=old_word||_lw(stub+4)!=0){
  overlay_attempts=4;sceKernelCpuResumeIntr(irq);return -1;
 }
 _sw(new_word,stub);sceKernelDcacheWritebackAll();sceKernelIcacheInvalidateAll();
 overlay_installed=1;sceKernelCpuResumeIntr(irq);return 0;
}
