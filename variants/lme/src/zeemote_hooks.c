/* Original PSP Go native HID observation/translation adapter. MIT.
 * No radio commands, DS3 feature requests or synthetic connection status. */
#ifdef ZEE_HOOK_HOST_TEST
#include "../tests/zeemote_hooks_mock.h"
#else
#include <pspkernel.h>
#include <psploadcore.h>
#include <pspsdk.h>
#endif
#include <stdint.h>
#include <string.h>
#include "zeemote_hid.h"
#include "zeemote_input.h"
#include "zeemote_hooks.h"
#include "lifecycle_trace.h"
#include "ds4_basic.h"
#include "sony_controls.h"
typedef int (*Parse)(void *,const uint8_t *,unsigned);
typedef int (*Init)(void *,void *,const uint16_t *,unsigned);
typedef int (*Report)(void *,void *,const uint8_t *,unsigned);
typedef int (*Term)(void *,void *);
static Parse original_parse;static Init original_init;static Report original_report;static Term original_term;
typedef struct {void *context,*node;unsigned generation;int present,valid,authorized,parse_status,init_result,native_called;uint8_t node_type,route_before,route_after;uint16_t vendor,product;unsigned init_serial,descriptor_length,first_length,frame_logged;uint8_t first_node_id,first_report_id;uint8_t descriptor[512];ZeeLayout layout;} Entry;
typedef struct {void *context,*node;} OwnedNode;
static OwnedNode owned_nodes[6];
static int owned_index(void *context,void *node){if(!node)return -1;for(unsigned i=0;i<6;i++)if(owned_nodes[i].node==node&&owned_nodes[i].context==context)return (int)i;return -1;}
static Entry entries[4];static unsigned generation,init_serial,reported_serial;
static uint32_t installed_base;
static ZeeDs4Authorize ds4_authorizer;
void zeemote_ds4_set_authorizer(ZeeDs4Authorize fn){int irq=sceKernelCpuSuspendIntr();ds4_authorizer=fn;sceKernelCpuResumeIntr(irq);}

static LifeRemove original_context_remove;
static LifeTerm original_keyboard_term,original_mouse_term;
typedef struct {unsigned phase,kind;int result;} LifeEvent;
static LifeEvent life_events[32];static unsigned life_count,life_dropped;
static void life_record(unsigned phase,unsigned kind,int result){
 int irq=sceKernelCpuSuspendIntr();
 if(life_count<32){LifeEvent *e=&life_events[life_count++];e->phase=phase;e->kind=kind;e->result=result;}else ++life_dropped;
 sceKernelCpuResumeIntr(irq);
}
static int context_remove_trace(void *e){return life_remove_forward(e,original_context_remove,life_record);}
static int keyboard_term_trace(void *c,void *n){return life_term_forward(1,c,n,original_keyboard_term,life_record);}
static int mouse_term_trace(void *c,void *n){return life_term_forward(2,c,n,original_mouse_term,life_record);}

typedef struct {unsigned descriptors,layouts,initialized,reports,invalid,unmatched,capacity,busy;} Counters;
static Counters counters,printed;
static Entry *find(void *context){for(unsigned i=0;i<4;i++)if(entries[i].present&&entries[i].context==context)return &entries[i];return NULL;}
int zeemote_gamestick_authorized(void){
 int found=0,irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<4;i++)if(entries[i].present&&entries[i].valid&&entries[i].authorized&&entries[i].vendor==0x0f0d&&entries[i].product==0x1011)found=1;
 sceKernelCpuResumeIntr(irq);return found;
}
int zeemote_sony_context_owned(const void *context){
 int found=0,irq=sceKernelCpuSuspendIntr();
 for(unsigned i=0;i<4;i++){Entry *e=&entries[i];
  if(e->context==context&&e->present&&e->valid&&e->authorized&&e->layout.mapping==3&&e->node&&*(const uint8_t *)e->node==3&&owned_index(e->context,e->node)>=0)found=1;
 }
 sceKernelCpuResumeIntr(irq);return found;
}
static void forget(Entry *e){if(e){e->valid=e->authorized=0;e->node=NULL;e->generation=++generation;}}
#ifdef ZEE_HOOK_HOST_TEST
#define ZEE_CODE_ALIGN __attribute__((aligned(4)))
#else
#define ZEE_CODE_ALIGN
#endif
static int ZEE_CODE_ALIGN descriptor_capture(void *context,const uint8_t *descriptor,unsigned length){
 int irq=sceKernelCpuSuspendIntr();counters.descriptors++;Entry *e=find(context);
 if(!e)for(unsigned i=0;i<4;i++)if(!entries[i].present){e=&entries[i];break;}
 if(!e)for(unsigned i=0;i<4;i++)if(!entries[i].authorized&&(!e||entries[i].generation<e->generation))e=&entries[i];
 if(e){e->present=1;e->context=context;}
 int clear=e&&e->authorized;unsigned serial=++generation;if(e){e->generation=serial;e->node=NULL;e->valid=e->authorized=0;e->vendor=e->product=0;e->init_serial=0;e->first_length=e->frame_logged=0;e->descriptor_length=descriptor&&length<=512?length:0;if(e->descriptor_length)memcpy(e->descriptor,descriptor,length);}else counters.capacity++;
 sceKernelCpuResumeIntr(irq);if(clear)zee_input_clear();
 ZeeLayout layout;int status=zee_parse_descriptor(descriptor,length,&layout);
 irq=sceKernelCpuSuspendIntr();if(e&&e->generation==serial){e->parse_status=status;if(!status){e->layout=layout;e->valid=1;counters.layouts++;}}sceKernelCpuResumeIntr(irq);
 int result=original_parse(context,descriptor,length);
 if(result<0){irq=sceKernelCpuSuspendIntr();if(e&&e->generation==serial)forget(e);sceKernelCpuResumeIntr(irq);}
 return result;
}
static int initialize(void *context,void *node,const uint16_t *identity,unsigned flags){
 uint16_t product=identity?identity[0]:0,vendor=identity?identity[1]:0;
 int irq=sceKernelCpuSuspendIntr();Entry *before=find(context);unsigned serial=before?before->generation:0;
 int owned=owned_index(context,node),free_owner=-1;
 if(owned>=0)free_owner=owned;else for(unsigned i=0;i<6;i++)if(!owned_nodes[i].node){free_owner=(int)i;break;}
 int projection=before&&ds4_basic_projection(before->descriptor,before->descriptor_length);
 int local=projection&&ds4_authorizer&&ds4_authorizer(context);
 int take=before&&before->valid&&node&&*(const uint8_t *)node==3&&!(vendor==0x054c&&product==0x0268)&&free_owner>=0&&
  *(const uint32_t *)((const uint8_t *)node+0x128)==0&&
  *(const uint32_t *)((const uint8_t *)node+0x150)==0&&
  *(const uint32_t *)((const uint8_t *)node+0x154)==0;
 if(take&&sony_basic_identity(vendor,product)&&!projection)take=0;
 if(take&&projection&&!sony_basic_identity(vendor,product)&&!local)take=0;
 if(take){
  before->layout.mapping=(local||sony_basic_identity(vendor,product))?3:(vendor==0x1038&&product==0x1412)?0:(vendor==0x2dc8&&product==0x3820)?2:1;
  before->route_before=((const uint8_t *)node)[1];
  /* Sony skips the generic gamepad collection without recording its Report
   * ID. Composite dispatch selects by node[1]; set only our owned type3 node.
   * Keep the confirmed Free route untouched. */
  if(before->layout.mapping&&before->layout.numbered)((uint8_t *)node)[1]=before->layout.report_id;
  before->route_after=((const uint8_t *)node)[1];
  /* The native parser owns the zero-initialized node and linked-list lifetime.
   * Our backend owns only translation; no native aux/SceInput allocation exists. */
  owned_nodes[free_owner].context=context;owned_nodes[free_owner].node=node;
  before->vendor=vendor;before->product=product;before->node=node;before->node_type=3;before->init_result=0;before->native_called=0;before->init_serial=++init_serial;before->authorized=1;counters.initialized++;
  sceKernelCpuResumeIntr(irq);return 0;
 }
 sceKernelCpuResumeIntr(irq);
 /* Never mix native resource allocation into a node already owned by us. */
 int result=owned>=0?-1:original_init(context,node,identity,flags);
 irq=sceKernelCpuSuspendIntr();Entry *e=find(context);
 if(e&&e->generation==serial){e->vendor=vendor;e->product=product;e->init_serial=++init_serial;e->node=node;e->node_type=node?*(const uint8_t *)node:0;e->init_result=result;e->native_called=owned<0;e->authorized=0;}
 sceKernelCpuResumeIntr(irq);return result;
}
/* Report thread has only1024 bytes. Large buffers live in resident storage.
 * A contending/reentrant callback skips work instead of waiting or sharing a
 * mutable layout. Generation checks still guard publication after decoding. */
typedef struct {int busy;ZeeLayout layout;uint8_t report[64];ZeeState state;} ReportScratch;
static ReportScratch report_scratch;
static int receive_report(void *context,void *node,const uint8_t *data,unsigned length){
 unsigned serial=0;int authorized=0;uint8_t node_report_id=0;Entry *owner=NULL;
 int irq=sceKernelCpuSuspendIntr();int was_owned=owned_index(context,node)>=0;Entry *e=find(context);
 if(e&&e->authorized&&e->node==node&&node&&*(const uint8_t *)node==3){
  if(report_scratch.busy)counters.busy++;
  else {report_scratch.busy=1;report_scratch.layout=e->layout;serial=e->generation;owner=e;node_report_id=((const uint8_t *)node)[1];if(!e->first_length){e->first_length=length;e->first_node_id=node_report_id;e->first_report_id=data&&length?data[0]:0;}authorized=1;}
 }else counters.unmatched++;
 sceKernelCpuResumeIntr(irq);
 if(authorized){
  ZeeLayout *layout=&report_scratch.layout;const uint8_t *full=data;unsigned size=length;int framing=0;
  if(data&&length<=64){
   if(!layout->numbered)framing=length==layout->payload_bytes;
   else if(length==(unsigned)layout->payload_bytes+1&&data[0]==layout->report_id)framing=1;
   else if(length==layout->payload_bytes&&node_report_id==layout->report_id&&length<64){report_scratch.report[0]=layout->report_id;memcpy(report_scratch.report+1,data,length);full=report_scratch.report;size=length+1;framing=1;}
  }
  int decoded=framing?(layout->mapping==3?sony_controls_decode(full,size,&report_scratch.state):zee_decode_report(layout,full,size,&report_scratch.state)):ZEE_INVALID;
  irq=sceKernelCpuSuspendIntr();
  if(owner->authorized&&owner->generation==serial&&owner->node==node){
   if(!decoded){zee_input_update_kind(&report_scratch.state,layout->mapping==3);counters.reports++;}
   else {counters.invalid++;zee_input_clear();}
  }
  report_scratch.busy=0;
  sceKernelCpuResumeIntr(irq);
 }
 return was_owned?0:original_report(context,node,data,length);
}
static int terminate(void *context,void *node){
 life_record(3,3,0);
 int clear=0,irq=sceKernelCpuSuspendIntr();Entry *e=find(context);int owned=owned_index(context,node);
 if(e&&e->node==node){clear=e->authorized;forget(e);e->present=0;}
 if(owned>=0){owned_nodes[owned].context=owned_nodes[owned].node=NULL;clear=1;}
 sceKernelCpuResumeIntr(irq);if(clear)zee_input_clear();
 /* Caller still releases its pool node after this callback. There are no
  * native aux/input resources to unregister for adapter-owned nodes. */
 int result=owned>=0?0:original_term(context,node);life_record(4,3,result);return result;
}
typedef int (*Ds4Drain)(void *,uint8_t *,unsigned);
static Ds4Drain ds4_drain_original;static uint32_t ds4_trace_base;
/* Terminal only for this module generation, after committed reboot entry.
 * A live native channel can keep the HID report loop from ever reading empty.
 * Do not disconnect RF or free/rewrite native queues; let the old reader return
 * to its event loop so native stop can join it. A new module starts unquiesced. */
static const void *volatile ds4_quiesced_context;
int zeemote_sony_quiesce_context(const void *context){
 int irq=sceKernelCpuSuspendIntr();
 int valid=ds4_trace_base&&context&&zeemote_sony_context_owned(context)&&
  (!ds4_quiesced_context||ds4_quiesced_context==context);
 if(valid)ds4_quiesced_context=context;
 sceKernelCpuResumeIntr(irq);return valid;
}
typedef struct {unsigned reads,nonempty,errors,length,id;int result;} Ds4ReadTrace;
static Ds4ReadTrace ds4_read_stats;
static int ZEE_CODE_ALIGN ds4_read_trace(void *context,uint8_t *data,unsigned capacity){
 if(context&&ds4_quiesced_context==context)return 0;
 int result=ds4_drain_original(context,data,capacity);
 if(ds4_authorizer&&ds4_authorizer(context)){
  int irq=sceKernelCpuSuspendIntr();ds4_read_stats.reads++;ds4_read_stats.result=result;
  if(result>0&&data&&(unsigned)result<=capacity){ds4_read_stats.nonempty++;ds4_read_stats.length=(unsigned)result;ds4_read_stats.id=data[0];}
  else if(result<0||(result>0&&(unsigned)result>capacity))ds4_read_stats.errors++;
  sceKernelCpuResumeIntr(irq);
 }
 return result;
}
static int jump(uint32_t pc,uint32_t target,uint32_t *out){if((pc|target)&3||((pc+4)&0xf0000000)!=(target&0xf0000000))return 0;*out=0x0c000000u|((target>>2)&0x3ffffff);return 1;}
int zeemote_input_install_module(SceModule *module,ZeeLogger log){
 if(!module||strcmp(module->modname,"sceHIDSvc"))return 0;
 uint32_t b=module->text_addr;if(installed_base){if(installed_base==b)return 1;if(log)log("ZEE INPUT skipped: another HID module generation is resident\n");return -1;}
 if(module->text_size<0x4e80||b>0xffffffffu-module->text_size)return -1;
 uint32_t expected,patched;if(!jump(b+0x908,b+0x1000,&expected)||!jump(b+0x908,(uint32_t)(uintptr_t)descriptor_capture,&patched))return -1;
 int irq=sceKernelCpuSuspendIntr();
 uint32_t life_c[LIFE_GUARDS];for(unsigned i=0;i<LIFE_GUARDS;i++)life_c[i]=_lw(b+life_offsets[i]);
 int valid=life_context_ok(b,module->text_size,life_c)&&_lw(b+0x908)==expected&&_lw(b+0x90c)==0x02402021u&&_lw(b+0x904)==0x27a50018u&&
  _lw(b+0x8f4)==0x8fa60014u&&_lw(b+0x8f8)==0x2cc70201u&&
  _lw(b+0x4e64)==b+0x3cc0&&_lw(b+0x4e68)==b+0x3cc0&&
  _lw(b+0x4e4c)==b+0x4054&&_lw(b+0x4e50)==b+0x4054&&
  _lw(b+0x4e78)==b+0x3fd0&&_lw(b+0x4e7c)==b+0x3fd0&&
  _lw(b+0x4054)==0x90a30000u&&_lw(b+0x4058)==0x27bdfff0u&&
  _lw(b+0x3cc0)==0x27bdffb0u&&_lw(b+0x3cc4)==0xafb40040u;
 if(valid){
  original_context_remove=(LifeRemove)(uintptr_t)(b+0xa44);original_keyboard_term=(LifeTerm)(uintptr_t)(b+0x2d28);original_mouse_term=(LifeTerm)(uintptr_t)(b+0x34d4);
  _sw((uint32_t)(uintptr_t)context_remove_trace,b+0x4e3c);_sw((uint32_t)(uintptr_t)keyboard_term_trace,b+0x4e70);_sw((uint32_t)(uintptr_t)mouse_term_trace,b+0x4e74);
  original_parse=(Parse)(uintptr_t)(b+0x1000);original_init=(Init)(uintptr_t)(b+0x3cc0);original_report=(Report)(uintptr_t)(b+0x4054);original_term=(Term)(uintptr_t)(b+0x3fd0);
  _sw((uint32_t)(uintptr_t)initialize,b+0x4e64);_sw((uint32_t)(uintptr_t)receive_report,b+0x4e4c);_sw((uint32_t)(uintptr_t)terminate,b+0x4e78);_sw(patched,b+0x908);
  sceKernelDcacheWritebackInvalidateAll();sceKernelIcacheInvalidateAll();installed_base=b;
 }
 sceKernelCpuResumeIntr(irq);
 if(log)log(valid?"ZEE INPUT installed: descriptor-derived type3 reports; validated generic gamepad descriptor; DS3 callbacks unchanged\n":"ZEE INPUT skipped: HID callback/descriptor ABI context mismatch\n");
 return valid?1:-1;
}
int zeemote_ds4_install_read_trace(SceModule *m,ZeeLogger log){
 if(!m||strcmp(m->modname,"sceHIDSvc"))return 0;
 uint32_t b=m->text_addr,original,patched;
 if(ds4_trace_base)return ds4_trace_base==b?1:-1;
 if(m->text_size<0xf60||b>0xffffffffu-m->text_size||!jump(b+0xe70,b+0xf58,&original)||!jump(b+0xe70,(uint32_t)(uintptr_t)ds4_read_trace,&patched))return -1;
 int irq=sceKernelCpuSuspendIntr();
 int valid=_lw(b+0xe6c)==0x03a02821&&_lw(b+0xe70)==original&&_lw(b+0xe74)==0x24060040&&_lw(b+0xe78)==0x18400025&&_lw(b+0xf58)==0x27bdffe0&&_lw(b+0xf5c)==0xafb10004;
 if(valid){ds4_drain_original=(Ds4Drain)(uintptr_t)(b+0xf58);_sw(patched,b+0xe70);sceKernelDcacheWritebackInvalidateAll();sceKernelIcacheInvalidateAll();ds4_trace_base=b;}
 sceKernelCpuResumeIntr(irq);if(log)log(valid?"DS4_READ_TRACE installed: metadata only before composite routing; original read unchanged\n":"DS4_READ_TRACE skipped: native context mismatch\n");return valid?1:-1;
}
int zeemote_input_ready(void){return installed_base!=0;}
void zeemote_diagnostic_state(unsigned out[9]){
 if(!out)return;
 int irq=sceKernelCpuSuspendIntr();
 out[0]=installed_base;out[1]=counters.descriptors;out[2]=counters.layouts;
 out[3]=counters.initialized;out[4]=counters.reports;out[5]=counters.invalid;
 out[6]=counters.unmatched;out[7]=counters.capacity;out[8]=counters.busy;
 sceKernelCpuResumeIntr(irq);
}

void zeemote_input_log_tick(ZeeLogger log){
 /* Native lifecycle monitoring, not an RF heartbeat: unchanged HID reports
  * may legitimately stop while a button remains held. Keepalive never creates
  * or reactivates input state; only a valid report can do that. */
 if(installed_base){
  SceModule *module=sceKernelFindModuleByName("sceHIDSvc");
  int alive=module&&module->text_addr==installed_base&&module->text_size>=0x4e80;
  int live_session=0,irq=sceKernelCpuSuspendIntr();
  for(unsigned i=0;i<4;i++){
   Entry *e=&entries[i];
   if(!alive){forget(e);e->present=0;}
   else if(e->present&&e->authorized&&e->node&&*(const uint8_t *)e->node==3)live_session=1;
  }
  if(!alive){memset(owned_nodes,0,sizeof(owned_nodes));zee_input_clear();}else if(live_session)zee_input_keepalive();
  sceKernelCpuResumeIntr(irq);
  if(alive&&live_session){ZeeInputStats input;zee_input_stats(&input);if(input.active&&!input.installed)zee_input_install();if(input.active)zee_input_install_overlay();}
 }
 if(log){
  static unsigned reported_nonempty,reported_errors,reported_length,reported_id;
  int irq=sceKernelCpuSuspendIntr();Ds4ReadTrace raw=ds4_read_stats;sceKernelCpuResumeIntr(irq);
  if((raw.nonempty&&!reported_nonempty)||raw.errors!=reported_errors||raw.length!=reported_length||raw.id!=reported_id){reported_nonempty=raw.nonempty;reported_errors=raw.errors;reported_length=raw.length;reported_id=raw.id;
   log("DS4_RAW reads=%u packets=%u errors=%u length=%u report_id=%02x last=%08x\n",raw.reads,raw.nonempty,raw.errors,raw.length,raw.id,(unsigned)raw.result);}
 }
 if(!log)return;
 LifeEvent life_copy[32];unsigned life_n,life_lost;
 int life_irq=sceKernelCpuSuspendIntr();life_n=life_count;life_lost=life_dropped;memcpy(life_copy,life_events,life_n*sizeof(life_copy[0]));life_count=life_dropped=0;sceKernelCpuResumeIntr(life_irq);
 for(unsigned i=0;i<life_n;i++)log("LIFE_EVENT phase=%u kind=%u result=%08x\n",life_copy[i].phase,life_copy[i].kind,(unsigned)life_copy[i].result);
 if(life_lost)log("LIFE_EVENT dropped=%u\n",life_lost);
 static unsigned stats_lines,seen_logged,motion_logged,error_logged,guard_logged=~0u;ZeeInputStats stats;zee_input_stats(&stats);
 if((stats.installed&&stats_lines<4)||stats.seen_buttons!=seen_logged||stats.motion_seen!=motion_logged||stats.install_error!=error_logged||stats.guard_index!=guard_logged){
  log("ZEE CTRL installed=%u active=%u peek_pos=%u peek_neg=%u read_pos=%u read_neg=%u merged=%u error=%u guard=%u vsh=%u vsh_reads=%u impose=%u\n",stats.installed,stats.active,stats.reads[0],stats.reads[1],stats.reads[2],stats.reads[3],stats.merged_samples,stats.install_error,stats.guard_index,stats.vsh_installed,stats.vsh_reads,stats.overlay_installed);
  log("ZEE STATE buttons=%08x seen=%08x motion=%x lx=%u range=%u..%u ly=%u range=%u..%u\n",stats.last_buttons,stats.seen_buttons,stats.motion_seen,stats.lx,stats.min_lx,stats.max_lx,stats.ly,stats.min_ly,stats.max_ly);
  stats_lines++;seen_logged=stats.seen_buttons;motion_logged=stats.motion_seen;error_logged=stats.install_error;guard_logged=stats.guard_index;
 }
 Counters now;int irq=sceKernelCpuSuspendIntr();now=counters;sceKernelCpuResumeIntr(irq);
 Counters changes=now,previous=printed;changes.reports=changes.unmatched=0;previous.reports=previous.unmatched=0;
 if(memcmp(&changes,&previous,sizeof(changes))||now.reports/512!=printed.reports/512||now.unmatched/512!=printed.unmatched/512||(now.reports<8&&now.reports!=printed.reports)||(now.unmatched<8&&now.unmatched!=printed.unmatched)){log("ZEE INPUT descriptors=%u layouts=%u initialized=%u reports=%u invalid=%u unmatched=%u capacity=%u busy=%u\n",now.descriptors,now.layouts,now.initialized,now.reports,now.invalid,now.unmatched,now.capacity,now.busy);printed=now;}
 unsigned highest=reported_serial;
 for(unsigned i=0;i<4;i++){
  Entry meta;irq=sceKernelCpuSuspendIntr();meta=entries[i];int snapshot_owned=owned_index(meta.context,meta.node)>=0;sceKernelCpuResumeIntr(irq);
  if(meta.authorized&&meta.first_length&&meta.frame_logged!=meta.generation){
   log("ZEE INPUT FIRST REPORT layout_id=%u numbered=%u payload=%u length=%u node_id=%u first_byte=%02x\n",meta.layout.report_id,meta.layout.numbered,meta.layout.payload_bytes,meta.first_length,meta.first_node_id,meta.first_report_id);
   irq=sceKernelCpuSuspendIntr();if(entries[i].generation==meta.generation)entries[i].frame_logged=meta.generation;sceKernelCpuResumeIntr(irq);
  }
  if(meta.init_serial<=reported_serial)continue;
  log("ZEE INPUT INIT vendor=%04x product=%04x descriptor_status=%d authorized=%d owned=%d native_called=%d %s_result=%08x node_type=%u descriptor_length=%u route_before=%u route_after=%u\n",meta.vendor,meta.product,meta.parse_status,meta.authorized,snapshot_owned,meta.native_called,meta.native_called?"native":"adapter",(unsigned)meta.init_result,meta.node_type,meta.descriptor_length,meta.route_before,meta.route_after);
  if(meta.authorized&&meta.layout.mapping==3)log("DS4 BASIC initialized: report01/10bytes, native authentication required; full78-byte mode not enabled\n");
  if(meta.descriptor_length&&meta.node_type==3){
   log("ZEE INPUT FRAME layout_id=%u numbered=%u payload=%u first_length=%u node_id=%u first_byte=%02x\n",meta.layout.report_id,meta.layout.numbered,meta.layout.payload_bytes,meta.first_length,meta.first_node_id,meta.first_report_id);
   static const char hex[]="0123456789abcdef";
   for(unsigned at=0;at<meta.descriptor_length;at+=16){char line[33];unsigned count=meta.descriptor_length-at;if(count>16)count=16;
    for(unsigned j=0;j<count;j++){uint8_t value=meta.descriptor[at+j];line[j*2]=hex[value>>4];line[j*2+1]=hex[value&15];}line[count*2]=0;log("ZEE HID DESCRIPTOR offset=%u bytes=%s\n",at,line);
   }
  }
  if(meta.init_serial>highest)highest=meta.init_serial;
 }
 reported_serial=highest;
}
