/* Guarded PSP Go native HID profile selection; no firmware binaries included. */
#include <pspkernel.h>
#include <psploadcore.h>
#include <pspsysmem_kernel.h>
#include <pspsysclib.h>
#include <pspsdk.h>
#include <pspinit.h>
#include <psppower.h>
#include <systemctrl.h>
#include <stdint.h>
#include <string.h>
#include "patch_plan.h"
#include "profile_policy.h"
#include "registration_policy.h"
#include "zeemote_hooks.h"
#include "zeemote_input.h"
#include "game_reconnect.h"
#include "pair_trace.h"
#include "lifecycle_trace.h"
#include "status_timeout.h"
#include "sdp_wait.h"
#include "completion_wait.h"
#include "nes30_override.h"
#include "ds4_basic.h"
#include "ds4_country.h"
#include "ds4_local.h"
#include "target_handover.h"
#include "sony_restore.h"
#include "ds4_link_setup.h"
#include "home_button.h"
#include "sony_controls.h"
#include "ds4_transition.h"
#include "service_trace.h"
#include "zeemote_input.h"
PSP_MODULE_INFO("joesnose_nods_free",PSP_MODULE_KERNEL,0,65);
/* Release build: no diagnostic file output or formatting. Keep the worker
 * because it also owns reconnect, persistence and controller keepalive. */
static STMOD_HANDLER previous_handler;
static uint32_t trampoline[4][4] __attribute__((aligned(16)));
static int (*original_register)(const void *,const void *);
static int (*original_no_pin)(const void *);
static int (*original_profile)(int,void *);
static int (*original_hid_connect)(int,int);
static int resident;
static volatile unsigned native_hid_seen,pending_hid_slot;
static int current_profile_query(int,void *);
static ZeePreferred preferred_target;
static unsigned preferred_loaded,preferred_dirty,target_selection_serial;
static Ds4LocalScope local_scope;
static uint32_t local_bsman,local_hid;
static unsigned local_ready;
static Ds4LinkSetup link_setup;
static int link_setup_ready;
static SceUID link_setup_thread;
static volatile unsigned link_setup_command;
static void ds4_observe_profile(unsigned,const uint8_t *,int);
static void prepare_sony_target(unsigned);
static int ds4_selected(unsigned);
static int ds4_should_defer(unsigned);

#define TARGET_PATH "ef0:/seplugins/pspgo_hid_target.dat"
typedef struct {uint32_t base;SceUID block;uint32_t *code;} UiStub;
static UiStub ui_stubs[4];
static unsigned ui_stub_count;
static UiStub pin_stubs[4];
static unsigned pin_stub_count;
static unsigned profile_lines;
typedef struct {int seen,status,valid;unsigned flags,id;} ProfileState;
static ProfileState profile_states[8];
/* Callback-compatible sink; no formatter, storage, K1 changes or side effects. */
static void log_line(const char *fmt,...){(void)fmt;}
static void reset_profile_cache(void){
 int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<8;++i)profile_states[i].seen=0;
 sceKernelCpuResumeIntr(irq);
}
/* Arguments are forwarded unchanged. Never read/log PINs, link keys or device records. */
static int register_trace(const void *device,const void *pin){
 log_line("PIN_REGISTER begin\n");
 int result=original_register(device,pin);
 if(result>=1&&result<=8)prepare_sony_target((unsigned)result);
 reset_profile_cache();
 log_line("PIN_REGISTER result=%08x\n",(unsigned)result);
 return result;
}
static void flush_code(void){sceKernelDcacheWritebackInvalidateAll();sceKernelIcacheInvalidateAll();}
static int no_pin_trace(const void *device_address){
 log_line("NO_PIN_REGISTER begin\n");
 int result=original_no_pin(device_address);
 if(result>=1&&result<=8)prepare_sony_target((unsigned)result);
 reset_profile_cache();
 log_line("NO_PIN_REGISTER result=%08x\n",(unsigned)result);
 return result;
}
/* The native event worker only copies known numeric fields into bounded RAM.
 * UI profile queries/connection requests drain it; no event-thread file I/O. */
static int (*original_slc_event)(const uint16_t *);
typedef struct {unsigned slot,state,flags,length,outgoing,sequence;} SlcEvent;
typedef struct {unsigned slot,flags,allow,sequence,finished;int result;} IncomingEvent;
static SlcEvent slc_events[16];
static IncomingEvent incoming_events[16];
static unsigned slc_count,slc_dropped,incoming_count,incoming_dropped,event_sequence;
static int active_slot;
static int (*original_incoming)(unsigned,unsigned,int);
static void record_incoming(unsigned slot,unsigned flags,int allow,int finished,int result){
 if(slot<1 || slot>8 || !(flags&2))return;
 int irq=sceKernelCpuSuspendIntr();unsigned sequence=++event_sequence;
 if(incoming_count<16){IncomingEvent *e=&incoming_events[incoming_count++];
  e->slot=slot;e->flags=flags;e->allow=(unsigned)allow;e->finished=(unsigned)finished;
  e->result=result;e->sequence=sequence;
 }else ++incoming_dropped;
 sceKernelCpuResumeIntr(irq);
}
static int incoming_trace(unsigned slot,unsigned flags,int allow){
 record_incoming(slot,flags,allow,0,0);
 int result=original_incoming(slot,flags,allow);
 record_incoming(slot,flags,allow,1,result);
 return result;
}
static int slc_event_trace(const uint16_t *body){
 uintptr_t a=(uintptr_t)body,phys=a&0x1fffffffu;
 unsigned alias=a&0xe0000000u;
 int irq=sceKernelCpuSuspendIntr();
 if(!(a&1) && (alias==0||alias==0x80000000u||alias==0xa0000000u) &&
    phys>=0x08000004u && phys<=0x0c000000u-6 && body[-2]==0x263 &&
    slc_payload_length(body[-1])>=6 && body[0]>=1 && body[0]<=8 && (body[2]&2)){
  if(body[1]==0){
   native_hid_seen=1;
   pending_hid_slot=body[0];
   /* Clear the merged state on a fresh confirmed selected-controller link. */
  }
  int fresh=ds4_setup_event(&link_setup,body[0],body[1]);
  if(local_ready){
   unsigned slot=body[0],id=_lw(local_bsman+0xdd20+slot*16);
   if(ds4_scope_selected(&local_scope,slot,id&65535,id>>16)&&(fresh||body[1]==2||body[1]==3))zee_input_clear();
   ds4_scope_link(&local_scope,slot,body[1]);
  }
  unsigned sequence=++event_sequence;
  if(slc_count<16){SlcEvent *e=&slc_events[slc_count++];
   e->slot=body[0];e->state=body[1];e->flags=body[2];e->length=body[-1];
   e->outgoing=active_slot==body[0];e->sequence=sequence;

  }else ++slc_dropped;
 }
 sceKernelCpuResumeIntr(irq);
 return original_slc_event(body);
}
/* UI threads and the dedicated logger drain RAM; no native event-thread I/O.
 * A drain lock preserves log sequence order when the UI and worker race. */
static int drain_busy;
static void drain_slc_events(void){
 SlcEvent events[16];IncomingEvent incoming[16];unsigned count,dropped,n,missed;
 int irq=sceKernelCpuSuspendIntr();
 if(drain_busy){sceKernelCpuResumeIntr(irq);return;}
 drain_busy=1;count=slc_count;dropped=slc_dropped;n=incoming_count;missed=incoming_dropped;
 memcpy(events,slc_events,count*sizeof(events[0]));slc_count=0;slc_dropped=0;
 memcpy(incoming,incoming_events,n*sizeof(incoming[0]));incoming_count=0;incoming_dropped=0;
 sceKernelCpuResumeIntr(irq);
 unsigned i=0,j=0;
 while(i<count || j<n){
  if(i<count && (j==n || (int)(events[i].sequence-incoming[j].sequence)<0)){
   SlcEvent *e=&events[i++];
   log_line("SLC_EVENT seq=%u slot=%u state=%u flags=%04x packet_length=%u payload_length=%u during_request=%u\n",
    e->sequence,e->slot,e->state,e->flags,e->length,e->length-4,e->outgoing);
  }else{
   IncomingEvent *e=&incoming[j++];
   if(e->finished)log_line("HID_INCOMING_END seq=%u slot=%u flags=%04x allow=%u result=%08x\n",
    e->sequence,e->slot,e->flags,e->allow,(unsigned)e->result);
   else log_line("HID_INCOMING_BEGIN seq=%u slot=%u flags=%04x allow=%u\n",e->sequence,e->slot,e->flags,e->allow);
  }
 }
 if(dropped||missed)log_line("TRACE_DROPPED slc=%u incoming=%u\n",dropped,missed);
 irq=sceKernelCpuSuspendIntr();drain_busy=0;sceKernelCpuResumeIntr(irq);
}
static PairReceive original_pair_receive;
static PairEvent pair_events[32];static unsigned pair_count,pair_dropped;
static void pair_record(const PairEvent *event){
 int irq=sceKernelCpuSuspendIntr();
 if(pair_count<32)pair_events[pair_count++]=*event;else ++pair_dropped;
 sceKernelCpuResumeIntr(irq);
}
static int pair_receive_trace(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int callbacks){
 return pair_trace_forward(sel,count,packet,cap,timeout,callbacks,original_pair_receive,pair_record);
}
static void drain_pair_events(void){
 PairEvent e[32];unsigned count,dropped;int irq=sceKernelCpuSuspendIntr();
 count=pair_count;dropped=pair_dropped;memcpy(e,pair_events,count*sizeof(e[0]));pair_count=pair_dropped=0;
 sceKernelCpuResumeIntr(irq);
 for(unsigned i=0;i<count;i++)log_line("PAIR_EVENT id=%04x length=%u slot=%u word1=%04x\n",e[i].id,e[i].length,e[i].slot,e[i].word1);
 if(dropped)log_line("PAIR_EVENT dropped=%u\n",dropped);
}
static void install_pair_trace(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x7484){log_line("PAIR_TRACE skipped: module unavailable\n");return;}
 uint32_t b=m->text_addr,c[PAIR_TRACE_WORDS],patched;
 int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<PAIR_TRACE_WORDS;i++)c[i]=_lw(b+0x9d0+i*4);
 int valid=pair_trace_plan(b,m->text_size,(uint32_t)(uintptr_t)pair_receive_trace,c,&patched)==0;
 if(valid){original_pair_receive=(PairReceive)(uintptr_t)(b+0x7480);_sw(patched,b+0x9e4);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"PAIR_TRACE passive numeric event observer installed\n":"PAIR_TRACE skipped: receive context mismatch\n");
}
static StatusReceive status_original;
static SceUID status_worker,status_main;
static unsigned status_reply,status_role;
static volatile unsigned status_calls,status_errors;
static int status_last;
typedef struct {unsigned serial,reply,role,target,has_timeout;int callback,bounded,done,result;} StatusFlight;
static StatusFlight status_flights[2];static unsigned status_serial;
static unsigned __attribute__((noinline)) status_flight_begin(int role,unsigned response,int target,void *timeout,int cb,int bounded){
 if(!role)return 0;
 int irq=sceKernelCpuSuspendIntr();unsigned serial=++status_serial;StatusFlight *f=&status_flights[role-1];
 f->serial=serial;f->reply=response;f->role=role;f->target=target;f->has_timeout=timeout!=0;f->callback=cb;f->bounded=bounded;f->done=0;f->result=0;
 sceKernelCpuResumeIntr(irq);return serial;
}
static void __attribute__((noinline)) status_flight_end(int role,unsigned serial,int result,int bounded,unsigned response){
 if(!role)return;
 int irq=sceKernelCpuSuspendIntr();StatusFlight *f=&status_flights[role-1];if(f->serial==serial){f->done=1;f->result=result;}
 if(bounded){++status_calls;if(result<0)++status_errors;status_last=result;status_reply=response;status_role=role;}
 sceKernelCpuResumeIntr(irq);
}
static int status_receive(uint16_t *reply,uint32_t *timeout,int cb){
 unsigned response=reply?reply[0]:0;
 SceUID caller=sceKernelGetThreadId();
 if(ds4_setup_wait_bound(caller==link_setup_thread,link_setup_command,response,timeout!=NULL,cb)){
  uint32_t budget=2000000;return status_original(reply,&budget,cb);
 }
 int role=caller==status_worker?1:caller==status_main?2:0;
 int target=role&&reply&&response>=0x100&&response<=0x1ff&&!timeout&&(cb==0||cb==1);
 int bounded=status_bound(response,timeout!=0,cb,target,role);
 unsigned serial=status_flight_begin(role,response,target,timeout,cb,bounded);
 int result=status_receive_forward(reply,timeout,cb,target,role,status_original,&bounded);
 status_flight_end(role,serial,result,bounded,response);
 return result;
}
static void status_log_tick(void){
 static unsigned flight_serial[2];static int flight_done[2];
 for(unsigned i=0;i<2;i++){
  int irq=sceKernelCpuSuspendIntr();StatusFlight f=status_flights[i];sceKernelCpuResumeIntr(irq);
  if(f.serial&&(f.serial!=flight_serial[i]||f.done!=flight_done[i])){
   flight_serial[i]=f.serial;flight_done[i]=f.done;
   log_line("STATUS_CALL serial=%u role=%u reply=%04x cb=%d target=%u prior_timeout=%u bounded=%d done=%d result=%08x\n",f.serial,f.role,f.reply,f.callback,f.target,f.has_timeout,f.bounded,f.done,(unsigned)f.result);
  }
 }
 static unsigned last;
 int irq=sceKernelCpuSuspendIntr();unsigned calls=status_calls,errors=status_errors;int result=status_last;unsigned reply=status_reply,role=status_role;sceKernelCpuResumeIntr(irq);
 if(calls!=last){last=calls;log_line("STATUS_WAIT bounded=%u errors=%u last=%08x budget_us=%u reply=%04x role=%u\n",calls,errors,(unsigned)result,STATUS_TIMEOUT_US,reply,role);}
}
static void install_status_timeout(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x7348||m->text_size+m->data_size+m->bss_size<0xdd0c){log_line("STATUS_WAIT skipped: module bounds\n");return;}
 uint32_t b=m->text_addr,words[STATUS_GUARDS];int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<STATUS_GUARDS;i++)words[i]=_lw(b+status_offsets[i]);
 int valid=status_context(b,m->text_size,words)&&(((b+0x37ec)^(uint32_t)(uintptr_t)status_receive)&0xf0000000u)==0;
 if(valid){
  status_worker=(SceUID)_lw(b+0xdd08);status_main=(SceUID)_lw(b+0xdd00);
  valid=status_worker>0&&status_main>0&&status_main!=status_worker;
  if(valid){status_original=(void *)(uintptr_t)(b+0x7340);_sw(0x0c000000|(((uint32_t)(uintptr_t)status_receive>>2)&0x03ffffff),b+0x37e8);flush_code();resident=1;}
 }
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"STATUS_WAIT installed: GENERIC worker0185/main ACK incl CB native timeout2s\n":"STATUS_WAIT skipped: native context mismatch\n");
}
/* Completion phase is separate from the immediate command ACK. Metadata only
 * on native stacks; no logging, allocation or forced resource cleanup here. */
static CompletionReceive completion_status_original;
static CompletionWait completion_plain_original,completion_cb_original;
typedef struct {unsigned serial,bounded,done;int result;} CompletionFlight;
static CompletionFlight completion_flights[2];static unsigned completion_serial;
static unsigned __attribute__((noinline)) completion_begin(unsigned kind,int bounded){
 int irq=sceKernelCpuSuspendIntr();unsigned serial=++completion_serial;
 CompletionFlight *f=&completion_flights[kind];f->serial=serial;f->bounded=(unsigned)bounded;f->done=0;f->result=0;
 sceKernelCpuResumeIntr(irq);return serial;
}
static void __attribute__((noinline)) completion_end(unsigned kind,unsigned serial,int result){
 int irq=sceKernelCpuSuspendIntr();CompletionFlight *f=&completion_flights[kind];if(f->serial==serial){f->done=1;f->result=result;}sceKernelCpuResumeIntr(irq);
}
static int __attribute__((noinline)) completion_status_receive(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb){
 unsigned serial=completion_begin(0,completion_status_bound(sel,count,cap,timeout,cb));
 int result=completion_status_forward(sel,count,packet,cap,timeout,cb,completion_status_original);
 completion_end(0,serial,result);return result;
}
/* Called only by the assembly bridge, which forwards S2 unchanged in t1 and S4 profile bits in t2. */
int __attribute__((noinline)) completion_disconnect_receive(int id,unsigned bits,unsigned mode,unsigned *out,unsigned *timeout,CompletionWait original,unsigned profiles){
 int known=original==completion_plain_original||original==completion_cb_original;
 unsigned serial=completion_begin(1,completion_disconnect_bound(id,bits,mode,out,timeout,known,profiles));
 int result=completion_disconnect_forward(id,bits,mode,out,timeout,original,known,profiles);
 completion_end(1,serial,result);return result;
}
extern int completion_disconnect_bridge(int,unsigned,unsigned,unsigned *,unsigned *);
static void install_completion_wait(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m){log_line("COMPLETION_WAIT skipped: no native module\n");return;}
 uint32_t b=m->text_addr,w[COMPLETION_GUARDS],patch[2];
 if(m->text_size<0xcbc4||b>0xffffffffu-m->text_size){log_line("COMPLETION_WAIT skipped: module bounds\n");return;}
 int irq=sceKernelCpuSuspendIntr();for(unsigned i=0;i<COMPLETION_GUARDS;i++)w[i]=_lw(b+completion_offsets[i]);
 int valid=completion_plan(b,m->text_size,(uint32_t)(uintptr_t)completion_status_receive,(uint32_t)(uintptr_t)completion_disconnect_bridge,w,patch);
 if(valid){
  completion_status_original=(void *)(uintptr_t)(b+0x7480);completion_plain_original=(void *)(uintptr_t)(b+0xcad0);completion_cb_original=(void *)(uintptr_t)(b+0xcac0);
  _sw(patch[0],b+0x3414);_sw(patch[1],b+0x2ccc);flush_code();resident=1;
 }
 sceKernelCpuResumeIntr(irq);log_line(valid?"COMPLETION_WAIT installed: status0275 and pending disconnect native timeout5s\n":"COMPLETION_WAIT skipped: native context mismatch\n");
}
static void completion_tick(void){
 static unsigned last_serial[2],last_done[2];
 for(unsigned i=0;i<2;i++){
  int irq=sceKernelCpuSuspendIntr();CompletionFlight f=completion_flights[i];sceKernelCpuResumeIntr(irq);
  if(f.serial&&(f.serial!=last_serial[i]||f.done!=last_done[i])){last_serial[i]=f.serial;last_done[i]=f.done;
   log_line("COMPLETION_CALL kind=%u serial=%u bounded=%u done=%u result=%08x budget_us=%u\n",i+1,f.serial,f.bounded,f.done,(unsigned)f.result,COMPLETION_WAIT_US);}
 }
}
typedef struct {unsigned kind,a,b;int result;} ServiceEvent;
static ServiceEvent service_events[32];static unsigned service_count,service_lost;
static void __attribute__((noinline)) service_record(unsigned kind,unsigned a,unsigned b,int result){
 int irq=sceKernelCpuSuspendIntr();
 if(service_count<32){ServiceEvent *e=&service_events[service_count++];e->kind=kind;e->a=a;e->b=b;e->result=result;}else ++service_lost;
 sceKernelCpuResumeIntr(irq);
}
static void __attribute__((noinline)) service_record_fragment(const void *packet,unsigned capacity,int result){
 unsigned status,more,length;
 if(result>0&&service_fragment(packet,capacity,&status,&more,&length))service_record(3,status,more,(int)length);
}
static int (*service_command_original)(const void *,void *,int);
static int service_command(const void *request,void *reply,int cb){
 unsigned service=0,attribute=0;const uint16_t *h=request;
 if(h&&h[0]==0x78&&h[1]==6){const uint8_t *payload=*(const uint8_t *const *)((const uint8_t *)request+4);
  if(payload){service=trace_u16(payload+2);attribute=trace_u16(payload+4);service_record(1,service,attribute,cb);}}
 int result=service_command_original(request,reply,cb);
 if(service)service_record(2,service,attribute,result);
 return result;
}
typedef int (*ServiceQuery)(unsigned,unsigned,unsigned,void *,unsigned,void *,int);
static ServiceQuery descriptor_query_original;static uint32_t descriptor_module_base;
static int descriptor_query(unsigned slot,unsigned service,unsigned attribute,void *buffer,unsigned capacity,void *timeout,int cb){
 unsigned identity=0;
 if(slot>=1&&slot<=8){int irq=sceKernelCpuSuspendIntr();identity=_lw(descriptor_module_base+0xdd20+slot*16);sceKernelCpuResumeIntr(irq);}
 int size=nes30_descriptor_attribute(slot,identity&65535,identity>>16,service,attribute,buffer,capacity,cb);
 if(size){service_record(4,service,attribute,size);return size;}
 size=ds4_basic_descriptor_attribute(slot,identity&65535,identity>>16,service,attribute,buffer,capacity,cb);
 if(size){service_record(5,service,attribute,size);return size;}
 return descriptor_query_original(slot,service,attribute,buffer,capacity,timeout,cb);
}
static ServiceQuery country_query_original;static uint32_t country_module_base;
static int country_query(unsigned slot,unsigned service,unsigned attribute,void *buffer,unsigned capacity,void *timeout,int cb){
 unsigned identity=0;
 if(slot>=1&&slot<=8){int irq=sceKernelCpuSuspendIntr();identity=_lw(country_module_base+0xdd20+slot*16);sceKernelCpuResumeIntr(irq);}
 if(slot>=1&&slot<=8&&service==0x1124&&attribute==0x203)service_record(6,identity&65535,identity>>16,(int)slot);
 int size=ds4_country_attribute(slot,identity&65535,identity>>16,service,attribute,buffer,capacity,cb,timeout!=NULL);
 if(size){service_record(7,service,attribute,size);return size;}
 return country_query_original(slot,service,attribute,buffer,capacity,timeout,cb);
}
static void install_ds4_country(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x9d88||m->text_size+m->data_size+m->bss_size<0xdda4){log_line("DS4_COUNTRY skipped: module bounds\n");return;}
 uint32_t b=m->text_addr,w[DS4_COUNTRY_GUARDS];int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<DS4_COUNTRY_GUARDS;i++)w[i]=_lw(b+ds4_country_offsets[i]);
 int valid=ds4_country_context(b,m->text_size,(uint32_t)(uintptr_t)country_query,w);
 if(valid){country_module_base=b;country_query_original=(void *)(uintptr_t)(b+0x55e0);_sw(0x0c000000|(((uint32_t)(uintptr_t)country_query>>2)&0x03ffffff),b+0x9d78);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);log_line(valid?"DS4_COUNTRY installed: supported DS4 HID0203 uses local country0; native query skipped\n":"DS4_COUNTRY skipped: native context mismatch\n");
}
/* All selection records are queried by existing logger/UI paths, never from
 * a native report/metadata callback. Native SLC events alone set linked bits. */
static void ds4_observe_profile(unsigned slot,const uint8_t *record,int status){
 if(!local_ready)return;
 int irq=sceKernelCpuSuspendIntr();unsigned old=local_scope.selected;
 /* Public API is transiently unavailable during native warm-start. Preserve
  * only an already verified live scope; never create one from this error. */
 if(sony_restore_keep_scope(status,*(volatile uint8_t *)(uintptr_t)(local_bsman+0xde5b),local_scope.selected,local_scope.linked,slot)){sceKernelCpuResumeIntr(irq);return;}
 ds4_scope_observe(&local_scope,slot,record,status,&preferred_target);
 unsigned now=local_scope.selected;sceKernelCpuResumeIntr(irq);
 if(old!=now)service_record(8,slot,now!=0,0);
}
/* UI/registration thread only, before native connection starts. Native SLC
 * and report callbacks never query profiles or persist the target. */
static void prepare_sony_target(unsigned slot){
 if(slot<1||slot>8||!original_profile)return;
 uint8_t record[80] __attribute__((aligned(16)))={0};record[0]=80;
 int old=pspSdkSetK1(0),status=original_profile((int)slot,record),prepared=0;
 if(status>=0){int irq=sceKernelCpuSuspendIntr();prepared=sony_target_prepare(&local_scope,&preferred_target,slot,record);
  if(prepared){preferred_loaded=1;++target_selection_serial;if(prepared==2){preferred_dirty=1;pending_hid_slot=0;}}
  sceKernelCpuResumeIntr(irq);
 }
 memset(record,0,sizeof(record));pspSdkSetK1(old);
 if(prepared)log_line("SONY TARGET prepared slot=%u before_native_connect=1 changed=%u\n",slot,prepared==2);
}
static int ds4_is_active(unsigned slot,unsigned *identity){
 unsigned id=0;int irq=sceKernelCpuSuspendIntr();
 if(local_ready&&ds4_slot_bit(slot))id=_lw(local_bsman+0xdd20+slot*16);
 int active=local_ready&&ds4_scope_active(&local_scope,slot,id&65535,id>>16);
 sceKernelCpuResumeIntr(irq);if(identity)*identity=id;return active;
}
static int ds4_selected(unsigned slot){
 int irq=sceKernelCpuSuspendIntr();unsigned id=local_ready&&ds4_slot_bit(slot)?_lw(local_bsman+0xdd20+slot*16):0;
 int selected=local_ready&&ds4_scope_selected(&local_scope,slot,id&65535,id>>16);
 sceKernelCpuResumeIntr(irq);return selected;
}
static int ds4_should_defer(unsigned slot){
 int selected=ds4_selected(slot),linked=ds4_is_active(slot,NULL);
 return ds4_transition_defer(sceKernelInitKeyConfig(),selected,linked);
}
static int ds4_context_authorized(const void *context){
 uintptr_t c=(uintptr_t)context;int irq=sceKernelCpuSuspendIntr();int active=0;
 if(local_ready&&ds4_hid_context_index(local_hid,c)&&_lw((uint32_t)c-4)==1){
  uint32_t backend=_lw((uint32_t)c+8);int at=ds4_backend_index(local_bsman,backend);
  if(at&&_lw(backend)==local_bsman+(at==1?0x981c:0x9a80)&&_lw(backend+12)==local_bsman+(at==1?0x9a0c:0x9c70)){
   unsigned slot=_lw(backend-4)&65535,id=ds4_slot_bit(slot)?_lw(local_bsman+0xdd20+slot*16):0;
   active=ds4_scope_active(&local_scope,slot,id&65535,id>>16);
  }
 }
 sceKernelCpuResumeIntr(irq);return active;
}
/* Only the reboot entry may preserve our live Sony HID channel. The same
 * walker is also used for suspend: normal suspend/disconnect stays native. */
#include "transition_preserve.h"
static void install_transition_preserve(void){
 if(!local_ready)return;
 uint32_t b=local_hid,walk=(uint32_t)(uintptr_t)transition_reboot_walk,bridge=(uint32_t)(uintptr_t)transition_control_bridge;
 /* Guard both callsites before either write; pool relocation is checked too. */
 const unsigned offsets[]={0xc04,0xc08,0xc0c,0xc10,0xc14,0xd60,0xd68,0xd84,0xd8c,0xd94,0xd98,0xdb0,0xdb4,0xdb8,0xdcc,0xdd0,0xdd4,0xdd8,0xddc,0xde0,0xde4,0xde8};
 const uint32_t words[]={0x27bdfff0,0xafbf0000,0x0c000000|((b+0xd60)>>2&0x03ffffff),0,0x24020001,0x27bdffe0,0xafbf0014,0x24130004,0x26320004,0x00911821,0x8c650000,0x90a60000,0x10d30019,0x00922021,0x8c62000c,0x24040006,0x00002821,0x8c47000c,0x00e0f809,0x00003021,0x03801021,0x0200e021};
 int valid=!(walk&3)&&!(bridge&3)&&((walk^b)&0xf0000000u)==0&&((bridge^b)&0xf0000000u)==0;
 int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++)if(_lw(b+offsets[i])!=words[i])valid=0;
 uint32_t pool=b+0x4f30;
 if(_lw(b+0xd64)!=(0x3c020000u|((pool+0x8000u)>>16))||_lw(b+0xd74)!=(0x24510000u|(pool&65535)))valid=0;
 if(valid){transition_walk_original=(void (*)(void))(uintptr_t)(b+0xd60);_sw(0x0c000000u|((walk>>2)&0x03ffffff),b+0xc0c);_sw(0x0c000000u|((bridge>>2)&0x03ffffff),b+0xddc);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line("SONY TRANSITION reboot_only_preserve=%u old_reader_quiesce=1 suspend_usb_native=1\n",valid);
}
static int (*local_pnp_original)(unsigned,uint16_t *,uint16_t *,void *);
static int local_pnp_query(unsigned slot,uint16_t *vendor,uint16_t *product,void *timeout){
 unsigned identity=0;
 if(vendor&&product&&!timeout&&ds4_is_active(slot,&identity)){
  int verified=sony_basic_identity(identity&65535,identity>>16);
  *vendor=verified?(uint16_t)identity:0;*product=verified?(uint16_t)(identity>>16):0;
  service_record(9,*vendor,*product,(int)slot);return 0;
 }
 return local_pnp_original(slot,vendor,product,timeout);
}
static ServiceQuery local_attribute_original;
static int local_attribute_query(unsigned slot,unsigned service,unsigned attribute,void *buffer,unsigned capacity,void *timeout,int cb){
 int size=ds4_local_attribute(slot,ds4_is_active(slot,NULL),service,attribute,buffer,capacity,timeout,cb);
 if(size){service_record(11,service,attribute,size);return size;}
 ServiceQuery next=attribute==0x203?country_query:attribute==0x206?descriptor_query:local_attribute_original;
 return next(slot,service,attribute,buffer,capacity,timeout,cb);
}
static int (*local_read_original)(unsigned,void *,unsigned,void *,void *);
static int local_read_start(unsigned channel,void *buffer,unsigned capacity,void *callback,void *context){
 int result=local_read_original(channel,buffer,capacity,callback,context);
 uintptr_t c=(uintptr_t)context;
 if(local_ready&&(c==local_bsman+0xde78||c==local_bsman+0xdea8)&&ds4_is_active(_lw((uint32_t)c+4)&65535,NULL))service_record(10,channel,capacity,result);
 return result;
}
/* Run blocking native setup only on our owned worker, never input/SLC stacks. */
static void ds4_link_setup_tick(void){
 if(!link_setup_ready)return;
 ZeeInputStats input;zee_input_stats(&input);if(!input.active)return;
 for(unsigned slot=1;slot<=8;slot++){
  int eligible=ds4_is_active(slot,NULL);
  int irq=sceKernelCpuSuspendIntr();unsigned epoch=ds4_setup_take(&link_setup,slot,eligible);sceKernelCpuResumeIntr(irq);
  if(!epoch)continue;
  int old=pspSdkSetK1(0);
  link_setup_command=0x17a;
  int result=((int (*)(unsigned,unsigned))(uintptr_t)(local_bsman+0x34f8))(slot,0);
  link_setup_command=0;
  irq=sceKernelCpuSuspendIntr();int same=ds4_setup_current(&link_setup,slot,epoch);sceKernelCpuResumeIntr(irq);
  int issued=0;
  if(result>=0&&same&&ds4_is_active(slot,NULL)){
   link_setup_command=0x124;
   ((void (*)(unsigned))(uintptr_t)(local_bsman+0xab8c))(slot);link_setup_command=0;issued=1;
  }
  pspSdkSetK1(old);
  log_line("DS4 LINK_SETUP slot=%u epoch=%u command79_result=%08x command24_issued=%u command24_status_unavailable=1\n",slot,epoch,(unsigned)result,issued);
 }
}
static void install_ds4_local(void){
 SceModule *bsm=sceKernelFindModuleByName("sceBSMan"),*hid=sceKernelFindModuleByName("sceHIDSvc");
 if(!bsm||!hid||bsm->text_size<0xccc8||bsm->text_size+bsm->data_size+bsm->bss_size<0xdec0||hid->text_size+hid->data_size+hid->bss_size<0x4f50||hid->text_addr>0xffffffffu-0x4f50){log_line("DS4_LOCAL skipped: module bounds\n");return;}
 uint32_t b=bsm->text_addr,w[LOCAL_GUARDS],pnp,attr,read;
 int irq=sceKernelCpuSuspendIntr();for(unsigned i=0;i<LOCAL_GUARDS;i++)w[i]=_lw(b+local_offsets[i]);
 int valid=ds4_local_context(b,bsm->text_size,(uint32_t)(uintptr_t)country_query,(uint32_t)(uintptr_t)descriptor_query,w)&&
  _lw(b+0xde80)==b+0x981c&&_lw(b+0xde8c)==b+0x9a0c&&_lw(b+0xdeb0)==b+0x9a80&&_lw(b+0xdebc)==b+0x9c70&&
  !mips_jump(b+0xa1e4,(uint32_t)(uintptr_t)local_pnp_query,&pnp)&&!mips_jump(b+0x9d28,(uint32_t)(uintptr_t)local_attribute_query,&attr)&&!mips_jump(b+0xa22c,(uint32_t)(uintptr_t)local_read_start,&read);
 const unsigned pnps[3]={0xa1e4,0x9d54,0xac24},attrs[3]={0x9d28,0x9d78,0x9da4};
 for(unsigned i=0;valid&&i<3;i++){uint32_t j;valid=!mips_jump(b+pnps[i],(uint32_t)(uintptr_t)local_pnp_query,&j)&&j==pnp&&!mips_jump(b+attrs[i],(uint32_t)(uintptr_t)local_attribute_query,&j)&&j==attr;}
 if(valid){
  local_bsman=b;local_hid=hid->text_addr;local_pnp_original=(void *)(uintptr_t)(b+0x58d8);local_attribute_original=(void *)(uintptr_t)(b+0x55e0);local_read_original=(void *)(uintptr_t)(b+0xccc0);
  for(unsigned i=0;i<3;i++)_sw(pnp|0x04000000u,b+(unsigned[]){0xa1e4,0x9d54,0xac24}[i]);
  for(unsigned i=0;i<3;i++)_sw(attr|0x04000000u,b+(unsigned[]){0x9d28,0x9d78,0x9da4}[i]);
  _sw(read|0x04000000u,b+0xa22c);zeemote_ds4_set_authorizer(ds4_context_authorized);local_ready=1;
  uint32_t setup_words[DS4_SETUP_GUARDS];for(unsigned i=0;i<DS4_SETUP_GUARDS;i++)setup_words[i]=_lw(b+ds4_setup_offsets[i]);
  link_setup_ready=ds4_setup_context(b,bsm->text_size,setup_words);
  flush_code();resident=1;
 }
 sceKernelCpuResumeIntr(irq);log_line(valid?"DS4_LOCAL installed: preferred Wireless Controller + real link required; local metadata, unknown IDs kept unknown; VSH/GAME once-only saved-profile reconnect\n":"DS4_LOCAL skipped: native call/interface guards mismatch\n");
}
static void install_nes30_override(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x9db0||m->text_size+m->data_size+m->bss_size<0xdda4){log_line("NES_DESCRIPTOR skipped: module bounds\n");return;}
 uint32_t b=m->text_addr,w[NES30_GUARDS];int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<NES30_GUARDS;i++)w[i]=_lw(b+nes30_offsets[i]);
 int valid=nes30_context(b,m->text_size,(uint32_t)(uintptr_t)descriptor_query,w);
 if(valid){descriptor_module_base=b;descriptor_query_original=(void *)(uintptr_t)(b+0x55e0);_sw(0x0c000000|(((uint32_t)(uintptr_t)descriptor_query>>2)&0x03ffffff),b+0x9da4);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"HID_DESCRIPTOR installed: captured NES30 + basic DS4 054c:05c4/09cc overrides for HID0206 only\n":"NES_DESCRIPTOR skipped: native context mismatch\n");
}
static void install_service_trace(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x568c){log_line("SERVICE_TRACE skipped: module bounds\n");return;}
 uint32_t b=m->text_addr;int irq=sceKernelCpuSuspendIntr();
 int valid=_lw(b+0x5678)==0xa7a20012&&_lw(b+0x567c)==0xafac0018&&_lw(b+0x5680)==(0x0c000000|(((b+0x3720)>>2)&0x03ffffff))&&_lw(b+0x5684)==0xa7b30030&&_lw(b+0x5688)==0x04400025&&(((b+0x5684)^(uint32_t)(uintptr_t)service_command)&0xf0000000u)==0;
 if(valid){service_command_original=(void *)(uintptr_t)(b+0x3720);_sw(0x0c000000|(((uint32_t)(uintptr_t)service_command>>2)&0x03ffffff),b+0x5680);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);log_line(valid?"SERVICE_TRACE request/ACK/fragment metadata enabled\n":"SERVICE_TRACE skipped: native context mismatch\n");
}
static void service_tick(void){
 ServiceEvent events[32];unsigned count,lost;int irq=sceKernelCpuSuspendIntr();count=service_count;lost=service_lost;memcpy(events,service_events,count*sizeof(events[0]));service_count=service_lost=0;sceKernelCpuResumeIntr(irq);
 for(unsigned i=0;i<count;i++){ServiceEvent *e=&events[i];
  if(e->kind==3)log_line("SDP_FRAGMENT status=%04x more=%u bytes=%d\n",e->a,e->b,e->result);
  else if(e->kind==4)log_line("NES_DESCRIPTOR source=Linux_capture service=%04x attribute=%04x encoded_bytes=%d descriptor_bytes=86 native_query_skipped=1\n",e->a,e->b,e->result);
  else if(e->kind==5)log_line("DS4_DESCRIPTOR source=basic_projection service=%04x attribute=%04x encoded_bytes=%d report_id=1 report_bytes=10 native_query_skipped=1\n",e->a,e->b,e->result);
  else if(e->kind==6)log_line("HID_PNP slot=%d vendor=%04x product=%04x\n",e->result,e->a,e->b);
  else if(e->kind==7)log_line("DS4_COUNTRY source=default_zero service=%04x attribute=%04x encoded_bytes=%d native_query_skipped=1\n",e->a,e->b,e->result);
  else if(e->kind==8)log_line("DS4_SCOPE slot=%u armed=%u selected_registration_only=1\n",e->a,e->b);
  else if(e->kind==9)log_line("DS4_LOCAL_ID slot=%d vendor=%04x product=%04x source=cached_or_unknown\n",e->result,e->a,e->b);
  else if(e->kind==10)log_line("DS4_READ_START channel=%u capacity=%u result=%08x\n",e->a,e->b,(unsigned)e->result);
  else if(e->kind==11)log_line("DS4_LOCAL_ATTRIBUTE service=%04x attribute=%04x bytes=%d native_query_skipped=1\n",e->a,e->b,e->result);
  else log_line("SDP_QUERY phase=%u service=%04x attribute=%04x result=%08x\n",e->kind,e->a,e->b,(unsigned)e->result);
 }
 if(lost)log_line("SERVICE_TRACE dropped=%u\n",lost);
}
static SdpReceive sdp_original;
static unsigned sdp_started,sdp_finished,sdp_errors,sdp_cb,sdp_bounded;static int sdp_result;
static void __attribute__((noinline)) sdp_begin(int bounded,int cb){
 int irq=sceKernelCpuSuspendIntr();++sdp_started;sdp_bounded=(unsigned)bounded;sdp_cb=(unsigned)cb;sceKernelCpuResumeIntr(irq);
}
static void __attribute__((noinline)) sdp_end(int result){
 int irq=sceKernelCpuSuspendIntr();++sdp_finished;if(result<0)++sdp_errors;sdp_result=result;sceKernelCpuResumeIntr(irq);
}
static int sdp_receive(const void *sel,unsigned count,void *packet,unsigned cap,void *timeout,int cb){
 sdp_begin(sdp_bound(sel,count,cap,timeout,cb),cb);
 int result=sdp_forward(sel,count,packet,cap,timeout,cb,sdp_original);
 service_record_fragment(packet,cap,result);sdp_end(result);return result;
}
static void sdp_tick(void){
 static unsigned last_started,last_finished;
 int irq=sceKernelCpuSuspendIntr();unsigned started=sdp_started,finished=sdp_finished,errors=sdp_errors,bounded=sdp_bounded,cb=sdp_cb;int result=sdp_result;sceKernelCpuResumeIntr(irq);
 if(started!=last_started||finished!=last_finished){last_started=started;last_finished=finished;
  log_line("SDP_WAIT started=%u finished=%u errors=%u bounded=%u cb=%u last=%08x budget_us=%u\n",started,finished,errors,bounded,cb,(unsigned)result,SDP_WAIT_US);}
}
static void install_sdp_wait(void){
 SceModule *m=sceKernelFindModuleByName("sceBSMan");
 if(!m||m->text_size<0x7488){log_line("SDP_WAIT skipped: module bounds\n");return;}
 uint32_t b=m->text_addr,w[SDP_GUARDS];int irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<SDP_GUARDS;i++)w[i]=_lw(b+sdp_offsets[i]);
 int valid=sdp_context(b,m->text_size,(uint32_t)(uintptr_t)sdp_receive,w);
 if(valid){sdp_original=(void *)(uintptr_t)(b+0x7480);_sw(0x0c000000|(((uint32_t)(uintptr_t)sdp_receive>>2)&0x03ffffff),b+0x56c8);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"SDP_WAIT installed: generic service-data event0272 native timeout5s\n":"SDP_WAIT skipped: native context mismatch\n");
}
/* All target-file and native-record work runs on the owned8192-byte worker. */
static void preferred_target_worker(void){
 static unsigned retry_slot,retries;
 if(!preferred_loaded){
  preferred_loaded=1;SceUID fd=sceIoOpen(TARGET_PATH,PSP_O_RDONLY,0);
  if(fd>=0){ZeePreferred p;unsigned char extra;int size=sceIoRead(fd,&p,sizeof(p)),tail=sceIoRead(fd,&extra,1);sceIoClose(fd);
   if(size==(int)sizeof(p)&&tail==0&&zee_preferred_valid(&p)){int target_irq=sceKernelCpuSuspendIntr();if(!preferred_dirty){preferred_target=p;local_scope.selected=0;}sceKernelCpuResumeIntr(target_irq);log_line("TARGET loaded slot=%u\n",p.slot);}
   else log_line("TARGET ignored: invalid record\n");}
 }
 int irq=sceKernelCpuSuspendIntr();unsigned slot=pending_hid_slot,selection_serial=target_selection_serial;sceKernelCpuResumeIntr(irq);
 if(slot<1||slot>8||!original_profile){retry_slot=retries=0;return;}
 if(retry_slot!=slot){retry_slot=slot;retries=0;}
 uint8_t record[80] __attribute__((aligned(16)))={0};record[0]=80;ZeePreferred p;
 int valid=current_profile_query((int)slot,record)>=0&&zee_preferred_make(record,slot,&p);
 if(valid){int irq=sceKernelCpuSuspendIntr();
  if(pending_hid_slot!=slot||target_selection_serial!=selection_serial){sceKernelCpuResumeIntr(irq);memset(record,0,sizeof(record));return;}
  int prepared=sony_target_prepare(&local_scope,&preferred_target,slot,record);
  if(prepared==2)preferred_dirty=1;
  if(!prepared&&(!zee_preferred_valid(&preferred_target)||memcmp(&preferred_target,&p,sizeof(p)))){preferred_target=p;local_scope.selected=0;preferred_dirty=1;}
  sceKernelCpuResumeIntr(irq);
 }
 memset(record,0,sizeof(record));
 if(!valid&&++retries<4)return;
 irq=sceKernelCpuSuspendIntr();if(target_selection_serial!=selection_serial){sceKernelCpuResumeIntr(irq);return;}if(pending_hid_slot==slot)pending_hid_slot=0;sceKernelCpuResumeIntr(irq);
 if(!valid){log_line("TARGET not saved: native profile unavailable slot=%u\n",slot);return;}
 retries=0;
 int check_irq=sceKernelCpuSuspendIntr();int dirty=preferred_dirty;sceKernelCpuResumeIntr(check_irq);if(!dirty)return;
 SceUID fd=sceIoOpen(TARGET_PATH,PSP_O_CREAT|PSP_O_WRONLY|PSP_O_TRUNC,0666);int wrote=fd>=0?sceIoWrite(fd,&p,sizeof(p)):-1;
 int closed=fd>=0?sceIoClose(fd):-1;
 if(wrote==(int)sizeof(p)&&closed>=0){int target_irq=sceKernelCpuSuspendIntr();if(!memcmp(&preferred_target,&p,sizeof(p)))preferred_dirty=0;sceKernelCpuResumeIntr(target_irq);log_line("TARGET saved slot=%u\n",slot);}else log_line("TARGET save failed slot=%u result=%08x\n",slot,(unsigned)wrote);
}
/* Recover warm-start links during startup AND after the public API is ready.
 * Native-known Sony input can work without our local scope; exit preservation
 * still needs that verified selected/live scope. Owned worker queries only. */
static int sony_restore_snapshot(unsigned slot,SonyRestoreState *out){
 if(!local_ready||!ds4_slot_bit(slot))return 0;
 SonyRestoreState v={0};
 for(unsigned i=0;i<6;i++){
  uint32_t p=local_bsman+0xdde4+i*8,w=_lw(p);
  if((w&65535)==slot&&((w>>16)&255)==2){v.slot=slot;v.flags=w>>16;v.event=(int)_lw(p+4);break;}
 }
 for(unsigned i=0;i<2;i++){
  uint32_t p=local_bsman+0xde78+i*48;
  if((_lw(p+4)&65535)==slot){v.backend_slot=slot;v.backend_mode=_lw(p);break;}
 }
 for(unsigned i=0;i<6;i++)v.address[i]=*(volatile uint8_t *)(uintptr_t)(local_bsman+0xdd1a+slot*16+i);
 *out=v;return sony_restore_live(&v,slot);
}
static void sony_restore_tick(void){
 static unsigned attempts;static int complete,last_ready=-1;
 if(complete||!local_ready||!preferred_loaded||!zee_preferred_valid(&preferred_target))return;
 int ready=*(volatile uint8_t *)(uintptr_t)(local_bsman+0xde5b);
 if(ready!=last_ready){attempts=0;last_ready=ready;}
 if(attempts>=3)return;
 if(ds4_is_active(preferred_target.slot,NULL)){complete=1;return;}
 uint32_t b=local_bsman;SonyRestoreState before={0},after;ZeePreferred preferred;
 int irq=sceKernelCpuSuspendIntr();preferred=preferred_target;int live=sony_restore_snapshot(preferred.slot,&before);unsigned sequence=event_sequence;sceKernelCpuResumeIntr(irq);
 if(!live){static int noted;if(!noted&&local_ready){noted=1;log_line("SONY RESTORE eligibility slot=%u startup_ready=%u flags=%04x backend_mode=%u backend_slot=%u\n",preferred.slot,*(volatile uint8_t *)(uintptr_t)(b+0xde5b),before.flags,before.backend_mode,before.backend_slot);}return;}
 /* Never send Sony recovery queries to generic-controller registrations. */
 if(!sony_restore_target_hint(&preferred,&before)){
  complete=1;log_line("SONY RESTORE skipped target_not_sony_or_address_changed=1 no_recovery_query=1\n");return;
 }
 /* Guard private read ABI and command/reply/buffer sizing before calling. */
 const unsigned off[]={0x4360,0x4368,0x436c,0x4378,0x4384,0x4390,0x43ac,0x43b0,0x43b4,0x43b8,0x43c4,0x43dc,0x43e0,0x43e4};
 const uint32_t words[]={0x27bdffa0,0xafb3004c,0x00043900,0x00f31821,0x00c09021,0xafbf0050,0x240c0063,0x240b0002,0x240a0163,0x24090050,0x02403021,0xafb00028,0x0c000000|(((b+0x3720)>>2)&0x03ffffff),0xafbd0014};
 for(unsigned i=0;i<sizeof(off)/sizeof(off[0]);i++)if(_lw(b+off[i])!=words[i]){complete=1;log_line("SONY RESTORE skipped native_read_abi=0\n");return;}
 uint32_t global=b+0xdd00;
 if(_lw(b+0x623c)!=(0x3c020000|((global+0x8000)>>16))||_lw(b+0x624c)!=(0x24500000|(global&65535))||_lw(b+0x6250)!=0x960800e4||_lw(b+0x6278)!=0x8e0300e8||_lw(b+0x6284)!=0xa60500e6){complete=1;return;}
 uint32_t backend_global=b+0xde70;
 if(_lw(b+0xa14c)!=(0x3c030000|((backend_global+0x8000)>>16))||_lw(b+0xa154)!=(0x24620000|(backend_global&65535))||_lw(b+0xa160)!=0x2447000c){complete=1;return;}
 if(_lw(b+0x4364)!=(0x3c020000|((global+0x8000)>>16))||_lw(b+0x4370)!=(0x24530000|(global&65535))){complete=1;return;}
 ++attempts;uint8_t raw[80] __attribute__((aligned(16)))={0},record[80]={0};
 int result=-1;int valid=sony_restore_read_record(ready,preferred.slot,original_profile,(SonyRestoreStartupRead)(uintptr_t)(b+0x4360),raw,record,&result);
 irq=sceKernelCpuSuspendIntr();
 valid=valid&&event_sequence==sequence&&!memcmp(&preferred,&preferred_target,sizeof(preferred))&&sony_restore_snapshot(preferred.slot,&after)&&sony_restore_accept(&preferred,record,&before,&after);
 if(valid){
  ds4_scope_observe(&local_scope,preferred.slot,record,0,&preferred_target);
  ds4_scope_link(&local_scope,preferred.slot,0);ds4_setup_event(&link_setup,preferred.slot,0);complete=1;
 }
 sceKernelCpuResumeIntr(irq);memset(raw,0,sizeof(raw));memset(record,0,sizeof(record));
 log_line("SONY RESTORE slot=%u attempt=%u native_result=%08x restored_live_verified=%u public_api=%u\n",preferred.slot,attempts,(unsigned)result,valid,ready);
}
static void preferred_target_tick(void){
 int old=pspSdkSetK1(0);preferred_target_worker();sony_restore_tick();pspSdkSetK1(old);
}
static void input_timing_tick(void){
 static unsigned ticks;if(++ticks%10)return;
 ZeeInputTiming t;zee_input_timing(&t);
 if(t.reports||t.samples)log_line("INPUT_TIMING reports=%u gaps=%u gap_max_us=%u gap_mean_us=%u samples=%u age_max_us=%u age_mean_us=%u stale=%u\n",t.reports,t.gaps,t.gap_max_us,t.gaps?(unsigned)(t.gap_total_us/t.gaps):0,t.samples,t.age_max_us,t.samples?(unsigned)(t.age_total_us/t.samples):0,t.stale);
}
static void auto_reconnect_tick(void);
static int logger_thread(SceSize args,void *argp){
 (void)args;(void)argp;link_setup_thread=sceKernelGetThreadId();
 for(;;){sceKernelDelayThread(500000);drain_slc_events();drain_pair_events();zeemote_input_log_tick(NULL);preferred_target_tick();if(zee_input_session_live())scePowerTick(PSP_POWER_TICK_ALL);auto_reconnect_tick();ds4_link_setup_tick();status_log_tick();completion_tick();sdp_tick();service_tick();input_timing_tick();}
 return 0;
}
static int current_profile_query(int index,void *record){
 int old=pspSdkSetK1(0);
 int result=original_profile(index,record);
 ds4_observe_profile((unsigned)index,record,result);
 pspSdkSetK1(old);return result;
}
static int selected_connect_forward(int index,int mode){
 log_line("HID_CONNECT native slot=%d selected_mode=%d flags=%04x\n",
          index,mode,mode?0x0402:0x0202);
 return original_hid_connect(index,mode);
}
static int hid_connect_trace(int index,int mode){
 drain_slc_events();
 prepare_sony_target((unsigned)index);
 log_line("HID_CONNECT begin slot=%d mode=%d\n",index,mode);
 int irq=sceKernelCpuSuspendIntr();
 int trace=index>=1 && index<=8 && !active_slot;
 if(trace)active_slot=index;
 sceKernelCpuResumeIntr(irq);
 NativeProfileDecision decision;
 int result=native_profile_connect(index,mode,original_profile?current_profile_query:NULL,
                                  selected_connect_forward,&decision);
 irq=sceKernelCpuSuspendIntr();if(trace)active_slot=0;
 if(result>=0&&index>=1&&index<=8){pending_hid_slot=(unsigned)index;native_hid_seen=1;}
 sceKernelCpuResumeIntr(irq);
 drain_slc_events();reset_profile_cache();
 log_line("PROFILE_SELECT queried=%d result=%08x valid=%d id=%u flags=%04x requested=%d selected=%d\n",
          decision.queried,(unsigned)decision.query_result,decision.valid,decision.id,
          decision.flags,decision.requested_mode,decision.selected_mode);
 log_line("HID_CONNECT result=%08x\n",(unsigned)result);
 return result;
}
static int profile_trace(int index,void *record){
 drain_slc_events();
 int result=original_profile(index,record);
 if(index>=1 && index<=8){
  unsigned flags=0,id=0;int valid=0;
  uintptr_t address=(uintptr_t)record,physical=address&0x1fffffffu;
  unsigned alias=address&0xe0000000u;
  int irq=sceKernelCpuSuspendIntr();
  if(result>=0 && !(address&3) &&
     (alias==0 || alias==0x80000000u || alias==0xa0000000u) &&
     physical>=0x08000000u && physical<=0x0c000000u-80 && _lw(address)==0x50){
   const uint8_t *p=record;id=p[4]|((unsigned)p[5]<<8);
   flags=p[72]|((unsigned)p[73]<<8);valid=1;
  }
  ProfileState *old=&profile_states[index-1];
  int changed=!old->seen || old->status!=result || old->valid!=valid || old->flags!=flags || old->id!=id;
  old->seen=1;old->status=result;old->valid=valid;old->flags=flags;old->id=id;
  int emit=changed && profile_lines<64;if(emit)++profile_lines;
  sceKernelCpuResumeIntr(irq);
  if(valid)ds4_observe_profile((unsigned)index,record,result);
  if(emit)log_line("PROFILE slot=%d result=%08x metadata=%d id=%u flags=%04x\n",index,(unsigned)result,valid,id,flags);
 }
 return result;
}
/* VSH/GAME reconnect uses the fresh saved profile, as manual connection does.
 * Only a genuine native successful link authorizes reports or local metadata. */
static int auto_connect_forward(int slot,int mode){
 int selected=ds4_selected((unsigned)slot);
 int chosen=ds4_transition_mode(sceKernelInitKeyConfig(),selected,mode);
 if(selected&&(sceKernelInitKeyConfig()==PSP_INIT_KEYCONFIG_GAME||sceKernelInitKeyConfig()==PSP_INIT_KEYCONFIG_VSH)){
  log_line("SONY PROFILE_RECONNECT slot=%d mode=%d native_result_follows=1\n",slot,chosen);
  int result=original_hid_connect(slot,chosen);
  log_line("SONY PROFILE_RECONNECT result=%08x\n",(unsigned)result);
  if(result>=0){int irq=sceKernelCpuSuspendIntr();pending_hid_slot=(unsigned)slot;native_hid_seen=1;sceKernelCpuResumeIntr(irq);}
  return result;
 }
 return hid_connect_trace(slot,mode);
}
/* Native Bluetooth startup precedes ordinary GAME plugins. Once our HID
 * hooks are ready in GAME or VSH, initiate the same saved-target connection as the menu.
 * One attempt per context only; no repeated connection storm on an asynchronous success. */
static void auto_reconnect_tick(void){
 static unsigned ticks,rounds,done;
 unsigned context=sceKernelInitKeyConfig();
 if(done||(context!=PSP_INIT_KEYCONFIG_GAME&&context!=PSP_INIT_KEYCONFIG_VSH))return;
 if(++ticks%4)return;
 ++rounds;ZeeInputStats stats;zee_input_stats(&stats);
 if(stats.active){done=1;return;}
 int restore_irq=sceKernelCpuSuspendIntr();int restoring=local_ready&&(local_scope.selected&local_scope.linked)&&!*(volatile uint8_t *)(uintptr_t)(local_bsman+0xde5b);sceKernelCpuResumeIntr(restore_irq);
 if(restoring)return;
 if(!zeemote_input_ready()||!sceKernelFindModuleByName("sceHIDSvc")){
  if(rounds>=4)done=1;
  log_line("AUTO_RECONNECT waiting for HID module round=%u\n",rounds);return;
 }
 ZeeReconnect d=zee_game_reconnect_policy(stats.active,&preferred_target,original_profile?current_profile_query:NULL,original_hid_connect?auto_connect_forward:NULL,ds4_should_defer);
 if(d.attempted||d.deferred||rounds>=4||(!zee_preferred_valid(&preferred_target)&&d.matches>1))done=1;
 log_line("AUTO_RECONNECT context=%08x round=%u queries=%u matches=%u preferred=%u slot=%u flags=%04x attempted=%u deferred=%u result=%08x\n",context,rounds,d.queries,d.matches,d.preferred,d.slot,d.flags,d.attempted,d.deferred,(unsigned)d.result);
}
static void install_trace(HookKind kind,uint32_t nid,uint32_t hook,const char *label){
 SceModule *mod=sceKernelFindModuleByName("sceBSMan");
 uint32_t entry=sctrlHENFindFunction("sceBSMan","sceBSMan",nid);
 if(!mod || !entry || mod->text_size<12 || entry<mod->text_addr ||
    entry-mod->text_addr>mod->text_size-12 || (entry&3)){
  log_line("%s hook skipped: export unavailable or outside module\n",label);return;
 }
 int irq=sceKernelCpuSuspendIntr();
 const uint32_t original[]={_lw(entry),_lw(entry+4)};HookPlan plan;
 int valid=native_hook_plan(entry,hook,(uint32_t)(uintptr_t)trampoline[kind],
                            original,kind,mod->text_addr,&plan)==0;
 if(valid){
  memcpy(trampoline[kind],plan.trampoline,sizeof(trampoline[kind]));
  if(kind==HOOK_PIN)original_register=(void *)trampoline[kind];
  else if(kind==HOOK_NO_PIN)original_no_pin=(void *)trampoline[kind];
  else if(kind==HOOK_PROFILE)original_profile=(void *)trampoline[kind];
  else original_hid_connect=(void *)trampoline[kind];
  _sw(plan.entry[0],entry);_sw(plan.entry[1],entry+4);flush_code();resident=1;
 }
 sceKernelCpuResumeIntr(irq);
 if(valid)log_line("%s hook installed at module+%08x\n",label,entry-mod->text_addr);
 else log_line("%s hook skipped: prologue mismatch %08x %08x\n",label,original[0],original[1]);
}
static void install_slc_trace(void){
 SceModule *mod=sceKernelFindModuleByName("sceBSMan");uint32_t word;
 if(!mod || mod->text_size<0x204c){log_line("SLC_TRACE skipped: module unavailable\n");return;}
 uint32_t base=mod->text_addr,site=base+0xa24;
 int irq=sceKernelCpuSuspendIntr();
 int valid=slc_call_plan(base,(uint32_t)(uintptr_t)slc_event_trace,
  _lw(site),_lw(site+4),_lw(site+8),_lw(base+0x2044),_lw(base+0x2048),&word)==0;
 if(valid){original_slc_event=(void *)(uintptr_t)(base+0x2044);
  _sw(word,site);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"SLC_TRACE event call installed\n":"SLC_TRACE skipped: firmware context mismatch\n");
}
static void install_incoming_trace(void){
 SceModule *mod=sceKernelFindModuleByName("sceBSMan");uint32_t word;
 if(!mod || mod->text_size<0x2f80){log_line("HID_INCOMING skipped: module unavailable\n");return;}
 uint32_t base=mod->text_addr,site=base+0xc90;
 int irq=sceKernelCpuSuspendIntr();
 uint32_t context[]={_lw(site-8),_lw(site-4),_lw(site),_lw(site+4),_lw(site+8),_lw(site+12),_lw(base+0x2f78),_lw(base+0x2f7c)};
 int valid=incoming_call_plan(base,(uint32_t)(uintptr_t)incoming_trace,context,&word)==0;
 if(valid){original_incoming=(void *)(uintptr_t)(base+0x2f78);
  _sw(word,site);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 log_line(valid?"HID_INCOMING decision call installed\n":"HID_INCOMING skipped: firmware context mismatch\n");
}
static void patch_discovery(SceModule *mod){
 const unsigned load_offset=0x13d40,compare_offset=0x13d6c;
 if(mod->text_size<compare_offset+4){log_line("DISCOVERY skipped: module too small\n");return;}
 int irq=sceKernelCpuSuspendIntr();
 uint32_t first=_lw(mod->text_addr+load_offset),second=_lw(mod->text_addr+compare_offset);
 int valid=discovery_patch_allowed(first,second);
 if(valid){
  _sw(0x240effffu,mod->text_addr+load_offset);
  _sw(0x144e0003u,mod->text_addr+compare_offset);flush_code();
 }
 sceKernelCpuResumeIntr(irq);
 if(valid)log_line("DISCOVERY filter lifted in RAM; select Zeemote: SteelSeries Free in Gamepad mode\n");
 else if(first==0x240effffu&&second==0x144e0003u)log_line("DISCOVERY filter already lifted\n");
 else log_line("DISCOVERY skipped: instruction mismatch %08x %08x\n",first,second);
}
static uint32_t jump_target(uint32_t pc,uint32_t word){
 return ((pc+4)&0xf0000000u)|((word&0x03ffffffu)<<2);
}
static int hid_context_matches(SceModule *mod){
 uint32_t base=mod->text_addr;
 if(mod->text_size<0x20c2c)return 0;
 uint32_t call=_lw(base+0x14ec0),hi=_lw(base+0x14ec8),lo=_lw(base+0x14ecc);
 uint32_t table=((hi&0xffffu)<<16)+(int16_t)lo;
 return _lw(base+0x14e18)==0x00a09021u && /* move s2,a1 */
        _lw(base+0x14e2c)==0x8e110004u && /* lw s1,4(s0) */
        _lw(base+0x14e38)==0x94a20048u && /* lhu v0,72(a1) */
        _lw(base+0x14e50)==0x24020001u &&
        call>>26==3 && jump_target(base+0x14ec0,call)==base+0x1abe0 &&
        _lw(base+0x14ec4)==0 && (hi&0xffff0000u)==0x3c030000u &&
        (lo&0xffff0000u)==0x24630000u && table==base+0x20c28;
}
static void patch_hid_menu(SceModule *mod){
 uint32_t base=mod->text_addr,site=base+0x14eb8;
 if(!hid_context_matches(mod)){log_line("HID_MENU skipped: firmware context mismatch\n");return;}
 UiStub *stub=NULL;
 for(unsigned i=0;i<ui_stub_count;++i)if(ui_stubs[i].base==base)stub=&ui_stubs[i];
 if(!stub){
  if(ui_stub_count>=4){log_line("HID_MENU skipped: user-stub limit reached\n");return;}
  int old_k1=pspSdkSetK1(0);
  SceUID block=sceKernelAllocPartitionMemory(2,"DS4HidMenu",PSP_SMEM_Low,56,NULL);
  uint32_t *code=block>=0?sceKernelGetBlockHeadAddr(block):NULL;
  uintptr_t address=(uintptr_t)code;
  int usable=code && !(address&3) && address>=0x08800000u && address<=0x0c000000u-56;
  if(block>=0 && !usable)sceKernelFreePartitionMemory(block);
  pspSdkSetK1(old_k1);
  if(block<0){log_line("HID_MENU skipped: user memory allocation %08x\n",block);return;}
  if(!usable){log_line("HID_MENU skipped: no valid user memory address\n");return;}
  stub=&ui_stubs[ui_stub_count++];stub->base=base;stub->block=block;stub->code=code;
 }
 int irq=sceKernelCpuSuspendIntr();
 uint32_t branch=_lw(site),delay=_lw(site+4);HidMenuPlan plan;
 int valid=hid_context_matches(mod)&&hid_menu_plan(site,(uint32_t)(uintptr_t)stub->code,branch,delay,&plan)==0;
 int already=branch>>26==2 && jump_target(site,branch)==(uint32_t)(uintptr_t)stub->code && delay==0x24040004u;
 if(valid){
  memcpy(stub->code,plan.code,sizeof(plan.code));_sw(plan.entry,site);flush_code();
 }
 sceKernelCpuResumeIntr(irq);
 if(valid)log_line("HID_MENU initial connection action enabled for type 2 with native HID flag\n");
 else if(already)log_line("HID_MENU already patched for this module\n");
 else log_line("HID_MENU skipped: branch/delay mismatch %08x %08x\n",branch,delay);
}
/* Only the default-PIN predicate at the real discovery-record call site is
 * extended. The stock allocator, object constructor and native registration
 * remain responsible for the selected record and PIN. No address-only hook
 * fabricates a discovery identifier. */
static void pin_context(SceModule *mod,uint32_t c[13]){
 uint32_t b=mod->text_addr;
 const uint32_t offsets[]={0x13f1c,0x13f20,0x13f24,0x13f28,0x13f2c,0x13f30,0x13f3c,0x13f40,0x13f44,0x13f48,0x13af8,0x13afc,0x20b70};
 for(unsigned i=0;i<13;i++)c[i]=_lw(b+offsets[i]);
}
static void patch_registration(SceModule *mod){
 if(mod->text_size<0x20ba0){log_line("PIN_ROUTE skipped: UI module too small\n");return;}
 uint32_t b=mod->text_addr,c[13],table[14],category[2],auto_words[AUTO_PIN_GUARDS];UiStub *stub=NULL;
 for(unsigned i=0;i<pin_stub_count;i++)if(pin_stubs[i].base==b)stub=&pin_stubs[i];
 if(stub){uint32_t predicate,constructor;
  if(!mips_jump(b+0x13f1c,(uint32_t)(uintptr_t)stub->code,&predicate)&&
     !mips_jump(b+0x13f3c,(uint32_t)(uintptr_t)stub->code+PIN_WRAPPER_OFFSET,&constructor)&&
     _lw(b+0x13f1c)==(predicate|0x04000000u)&&_lw(b+0x13f3c)==(constructor|0x04000000u)&&
     _lw(b+0x13f20)==0x02002821u&&_lw(b+0x13f40)==0x00408021u&&_lw(b+0x13afc)==0x24060020u){
   log_line("PIN_ROUTE hybrid already installed\n");return;
  }
 }
 pin_context(mod,c);for(unsigned i=0;i<14;i++)table[i]=_lw(b+0x20b68+i*4);
 category[0]=_lw(b+0x13950);category[1]=_lw(b+0x13954);NativePinPlan plan;
 for(unsigned i=0;i<AUTO_PIN_GUARDS;i++)auto_words[i]=_lw(b+auto_pin_offsets[i]);
 int automatic=native_auto_pin_context(auto_words);
 if(!automatic){uint32_t expected[AUTO_PIN_GUARDS];native_auto_pin_expected(expected);for(unsigned i=0;i<AUTO_PIN_GUARDS;i++)if(auto_words[i]!=expected[i])log_line("SONY PAIRING guard offset=%05x expected=%08x actual=%08x\n",auto_pin_offsets[i],expected[i],auto_words[i]);}
 if(native_pin_hybrid_plan(b,0x08800000u,c,table,category,&plan)){
  log_line("PIN_ROUTE skipped: constructor/category/vtable context mismatch\n");return;
 }
 if(!stub){
  if(pin_stub_count>=4){log_line("PIN_ROUTE skipped: resident stub limit\n");return;}
  int old=pspSdkSetK1(0);SceUID block=sceKernelAllocPartitionMemory(2,"DS4NativePin",PSP_SMEM_Low,PIN_STUB_BYTES,NULL);
  uint32_t *code=block>=0?sceKernelGetBlockHeadAddr(block):NULL;uintptr_t a=(uintptr_t)code;
  int usable=code&&!(a&3)&&a>=0x08800000u&&a<=0x0c000000u-PIN_STUB_BYTES;
  if(block>=0&&!usable){sceKernelFreePartitionMemory(block);}
  pspSdkSetK1(old);
  if(block<0||!usable){log_line("PIN_ROUTE skipped: no usable resident user memory\n");return;}
  stub=&pin_stubs[pin_stub_count++];stub->base=b;stub->block=block;stub->code=code;
 }
 int irq=sceKernelCpuSuspendIntr();pin_context(mod,c);for(unsigned i=0;i<14;i++)table[i]=_lw(b+0x20b68+i*4);
 category[0]=_lw(b+0x13950);category[1]=_lw(b+0x13954);
 for(unsigned i=0;i<AUTO_PIN_GUARDS;i++)auto_words[i]=_lw(b+auto_pin_offsets[i]);
 int valid=automatic?native_pin_autopin_plan(b,(uint32_t)(uintptr_t)stub->code,c,table,category,auto_words,&plan)==0:native_pin_hybrid_plan(b,(uint32_t)(uintptr_t)stub->code,c,table,category,&plan)==0;
 if(valid){memcpy(stub->code,plan.code,sizeof(plan.code));_sw(plan.pin_zero_delay,b+0x13afc);_sw(plan.constructor_entry,b+0x13f3c);_sw(plan.entry,b+0x13f1c);if(plan.selector_entry)_sw(plan.selector_entry,b+0x2594);flush_code();resident=1;}
 sceKernelCpuResumeIntr(irq);
 if(valid)log_line("SONY PAIRING automatic_default_pin=%u entry_screen_hook=%u native_registration_unchanged=1\n",automatic,plan.selector_entry!=0);
 log_line(valid?"PIN_ROUTE installed: Free + DS4/DualSense exact name/class rules use native PIN registration/private HID category; names/classes are not identity proof\n":"PIN_ROUTE skipped: context changed before installation\n");
}
static int module_started(SceModule *mod){
 if(mod&&!strcmp(mod->modname,"bluetooth_plugin_module")){patch_discovery(mod);patch_hid_menu(mod);patch_registration(mod);}
 zeemote_input_install_module(mod,log_line);
 zeemote_ds4_install_read_trace(mod,log_line);
 return previous_handler?previous_handler(mod):0;
}
/* Read once before installing report callbacks; the active profile is immutable
 * for this module generation. No storage work in the report path. */
static void __attribute__((noinline)) configure_sony_controls(void){
 static char text[8193];SonyControlFile config;sony_controls_defaults(&config);
 const char *path="ef0:/seplugins/pspgo_hid_controls.txt";int old=pspSdkSetK1(0),fd=sceIoOpen(path,PSP_O_RDONLY,0),ok=0;unsigned used=0,bad=0;int io=0;
 if(fd>=0){
  while(used<sizeof(text)){int n=sceIoRead(fd,text+used,sizeof(text)-used);if(n<=0){io=n;break;}used+=(unsigned)n;}
  int closed=sceIoClose(fd);
  if(io>=0&&closed>=0&&used<=8192)ok=sony_controls_parse(text,used,&config,&bad);
 }
 int game=sceKernelInitKeyConfig()==PSP_INIT_KEYCONFIG_GAME;
 sony_controls_active=game?config.game:config.vsh;
 pspSdkSetK1(old);
 if(fd<0)log_line("SONY CONTROLS defaults context=%s game_preset=fps\n",game?"GAME":"VSH");
 else log_line("SONY CONTROLS file_loaded=%u context=%s bytes=%u error_line=%u defaults_on_error=1\n",ok,game?"GAME":"VSH",used,bad);
}
/* HOME argument1 opens and0 closes, using the same native status bit.
 * Resolve/guard an export; do not patch Impose or invoke it from report IRQs. */
static int home_thread(SceSize args,void *argp){
 (void)args;(void)argp;uint32_t base=0,size=0,entry=0,status_entry=0;unsigned retries=0;
 for(;;){
  sceKernelDelayThread(10000);
  SceModule *m=sceKernelFindModuleByName("sceImpose_Driver");
  if(!entry){
   if(retries++%50)continue;
   if(!m)continue;
   uint32_t p=sctrlHENFindFunction("sceImpose_Driver","sceImpose_driver",0xefb54a28),w[HOME_GUARDS];
   if(!p)p=sctrlHENFindFunction("sceImpose_Driver","sceImpose",0x381bd9e7);
   base=m->text_addr;size=m->text_size;
   if(!p||(p&3)||size<0x4c||base>0xffffffffu-size||p<base||p-base>size-0x4c){if(retries==1)log_line("DS4 HOME waiting for native export\n");continue;}
   for(unsigned i=0;i<HOME_GUARDS;i++)w[i]=_lw(p+home_offsets[i]);
   if(!zee_home_context(base,size,p,w)){log_line("DS4 HOME skipped: native ABI guard mismatch\n");zee_input_home_enable(0);return 0;}
   uint32_t getter=sctrlHENFindFunction("sceImpose_Driver","sceImpose_driver",0xb3eae8f0),sw[4];
   if(!getter||(getter&3)||size<16||getter<base||getter-base>size-16){log_line("DS4 HOME skipped: status export unavailable\n");zee_input_home_enable(0);return 0;}
   for(unsigned i=0;i<4;i++)sw[i]=_lw(getter+i*4);
   if(!zee_home_status_context(base,size,getter,sw)){log_line("DS4 HOME skipped: status ABI mismatch\n");zee_input_home_enable(0);return 0;}
   entry=p;status_entry=getter;log_line("DS4 HOME ready: native menu toggle, dedicated worker\n");
  }
  if(!m||m->text_addr!=base||m->text_size!=size){zee_input_home_enable(0);log_line("DS4 HOME stopped: module changed\n");return 0;}
  if(zee_input_home_take()){
   int old=pspSdkSetK1(0);unsigned status=((unsigned (*)(void))(uintptr_t)status_entry)();
   int open=zee_home_open_argument(status);((void (*)(int))(uintptr_t)entry)(open);pspSdkSetK1(old);
   log_line("DS4 HOME native toggle dispatched open=%u\n",(unsigned)open);
  }
 }
 return 0;
}
int module_start(SceSize args,void *argp){
 (void)args;(void)argp;
 unsigned fw=sceKernelDevkitVersion();int model=sceKernelGetModel();
 log_line("\nPSPGo Classic HID clean-baseline route 0.64 model=%d devkit=%08x\n",model,fw);
 if(model!=4||(fw!=0x06060010&&fw!=0x06060110)){
  log_line("UNSUPPORTED model/firmware; no patches installed\n");return -1;
 }
 configure_sony_controls();
 log_line("BOOT context=%08x ctrl=%u bsman=%u hid=%u bridge=%u\n",
  (unsigned)sceKernelInitKeyConfig(),sceKernelFindModuleByName("sceController_Service")!=NULL,
  sceKernelFindModuleByName("sceBSMan")!=NULL,sceKernelFindModuleByName("sceHIDSvc")!=NULL,
  sceKernelFindModuleByName("sceVshBridge_Driver")!=NULL);
 install_trace(HOOK_PIN,0x8ac2f7b8u,(uint32_t)(uintptr_t)register_trace,"PIN_REGISTER");
 install_trace(HOOK_NO_PIN,0x2d649923u,(uint32_t)(uintptr_t)no_pin_trace,"NO_PIN_REGISTER");
 install_trace(HOOK_PROFILE,0xcbd232d1u,(uint32_t)(uintptr_t)profile_trace,"PROFILE");
 install_trace(HOOK_HID_CONNECT,0xf188a405u,(uint32_t)(uintptr_t)hid_connect_trace,"HID_CONNECT");
 install_slc_trace();
 install_incoming_trace();
 install_pair_trace();
 install_status_timeout();
 install_completion_wait();
 install_sdp_wait();
 install_service_trace();
 install_nes30_override();
 install_ds4_country();
 install_ds4_local();
 log_line("DS4 LINK_SETUP guarded_native_sequence=%u worker_only=1\n",link_setup_ready);
 SceUID logger=sceKernelCreateThread("JoesnoseHID",logger_thread,0x60,8192,0,NULL);
 int logger_result=logger>=0?sceKernelStartThread(logger,0,NULL):logger;
 if(logger_result<0){if(logger>=0)sceKernelDeleteThread(logger);log_line("LOGGER unavailable %08x; reopen device list to flush\n",(unsigned)logger_result);}
 else log_line("LOGGER automatic flush enabled\n");
 previous_handler=sctrlHENSetStartModuleHandler(module_started);resident=1;
 SceModule *mod=sceKernelFindModuleByName("bluetooth_plugin_module");
 if(mod){patch_discovery(mod);patch_hid_menu(mod);patch_registration(mod);}
 zeemote_input_install_module(sceKernelFindModuleByName("sceHIDSvc"),log_line);
 zeemote_ds4_install_read_trace(sceKernelFindModuleByName("sceHIDSvc"),log_line);
 install_transition_preserve();
 if(sceKernelInitKeyConfig()==PSP_INIT_KEYCONFIG_GAME){
  SceUID home=sceKernelCreateThread("DS4Home",home_thread,0x30,4096,0,NULL);
  int result=home>=0?sceKernelStartThread(home,0,NULL):home;
  if(result>=0)zee_input_home_enable(1);else {if(home>=0)sceKernelDeleteThread(home);log_line("DS4 HOME worker unavailable result=%08x\n",(unsigned)result);}
 }
 log_line("MODE_POLICY fresh Wireless Controller0602 uses generic HID mode1; other0602 keeps legacy mode0\n");
 log_line("READY: generic + DS4/DualSense basic01 input; native pairing/authentication; remembered HID reconnect; actual connection required\n");
 return 0;
}
int module_stop(SceSize args,void *argp){
 (void)args;(void)argp;
 /* Keep hook code resident. Disabling the entry and rebooting clears RAM patches. */
 if(resident)return -1;
 return 0;
}
