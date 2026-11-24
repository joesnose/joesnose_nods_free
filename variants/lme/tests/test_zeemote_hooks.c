#include "../src/zeemote_hid.c"
#include "../src/ds4_basic.c"
#define ZEE_HOOK_HOST_TEST
#include "../src/zeemote_hooks.c"
#define main descriptor_test_main
#include "zeemote_descriptor_fixture.h"
#undef main
#include "zeemote_free_protocol_fixture.h"
#include "gamestick_descriptor_fixture.h"
#include "nes30_descriptor_fixture.h"
static const uint8_t *active_descriptor=descriptor;static unsigned active_length=sizeof(descriptor);
#include <stdarg.h>
static unsigned updates,clears,parse_calls,init_calls,report_calls,term_calls,install_calls,logs;
static int parse_result,init_result,terminate_during_install,input_active,term_after_snapshot,reenter_after_snapshot,input_installed;static const uint8_t *nested_report;static unsigned keepalive_calls;static SceModule *live_module;static ZeeState injected;
static uint8_t node[0x160];static void *ctx=(void *)(uintptr_t)0x88001000;
static int local_authorized,raw_result,raw_calls;static uint8_t raw_id;
static int local_allow(const void *c){assert(c==ctx);return local_authorized;}
static int raw_read(void *c,uint8_t *data,unsigned capacity){assert(c==ctx&&capacity==64);raw_calls++;if(data&&raw_result>0&&(unsigned)raw_result<=capacity)data[0]=raw_id;return raw_result;}
int sceKernelCpuSuspendIntr(void){return 0;}
void sceKernelCpuResumeIntr(int n){(void)n;
 if(report_scratch.busy&&term_after_snapshot){term_after_snapshot=0;terminate(ctx,node);}
 if(report_scratch.busy&&reenter_after_snapshot){reenter_after_snapshot=0;assert(receive_report(ctx,node,nested_report,8)==0);}
}
static uint32_t memory[0x5000/4];static uint32_t base=0x8822f800;
uint32_t _lw(uint32_t address){assert(address>=base&&address<base+sizeof(memory));return memory[(address-base)/4];}
void _sw(uint32_t value,uint32_t address){assert(address>=base&&address<base+sizeof(memory));memory[(address-base)/4]=value;}
void sceKernelDcacheWritebackInvalidateAll(void){}void sceKernelIcacheInvalidateAll(void){}
void zee_input_stats(ZeeInputStats *out){memset(out,0,sizeof(*out));out->active=input_active;out->installed=input_installed;}
int zee_input_install_overlay(void){return 0;}
int zee_input_install(void){assert(!report_scratch.busy);install_calls++;input_installed=1;if(terminate_during_install){terminate_during_install=0;terminate(ctx,node);}return 0;}
#include "../src/sony_controls.c"
static unsigned injected_ds4;
void zee_input_update_kind(const ZeeState *s,unsigned ds4){injected_ds4=ds4;updates++;injected=*s;input_active=1;}void zee_input_clear(void){clears++;input_active=0;}
void zee_input_keepalive(void){keepalive_calls++;}
SceModule *sceKernelFindModuleByName(const char *name){assert(!strcmp(name,"sceHIDSvc"));return live_module;}
static int parse(void *c,const uint8_t *d,unsigned n){assert(c==ctx&&d==active_descriptor&&n==active_length);parse_calls++;return parse_result;}
static int init(void *c,void *p,const uint16_t *id,unsigned flags){assert(c==ctx&&p==node&&flags==123);assert(id);init_calls++;return init_result;}
static int report(void *c,void *p,const uint8_t *d,unsigned n){assert(c==ctx&&p==node&&d&&n<=65);report_calls++;return 77;}
static int term(void *c,void *p){assert(c==ctx&&p==node);term_calls++;return 88;}
static void log_mock(const char *fmt,...){assert(fmt);logs++;}
static void reset(void){SonyControlFile controls;sony_controls_defaults(&controls);sony_controls_active=controls.vsh;ds4_authorizer=NULL;active_descriptor=descriptor;active_length=sizeof(descriptor);life_count=life_dropped=0;memset(owned_nodes,0,sizeof(owned_nodes));memset(entries,0,sizeof(entries));memset(&counters,0,sizeof(counters));memset(&printed,0,sizeof(printed));generation=init_serial=reported_serial=installed_base=0;parse_result=init_result=input_active=term_after_snapshot=reenter_after_snapshot=input_installed=0;memset(&report_scratch,0,sizeof(report_scratch));keepalive_calls=0;live_module=NULL;updates=clears=parse_calls=init_calls=report_calls=term_calls=install_calls=logs=0;memset(node,0,sizeof(node));node[0]=3;original_parse=parse;original_init=init;original_report=report;original_term=term;}
static void authorize(uint16_t vendor,uint16_t product){uint16_t id[]={product,vendor};assert(!descriptor_capture(ctx,active_descriptor,active_length));assert(!initialize(ctx,node,id,123));}
int main(void){
 reset();assert(!zeemote_sony_context_owned(ctx));
 authorize(0x054c,0x09cc); /* Nonprojected descriptor stays native. */
 assert(!zeemote_sony_context_owned(ctx));
 reset();uint8_t r[]={7,1,0,0,255,128,128,8};assert(receive_report(ctx,node,r,8)==77&&!updates);
 authorize(0x1038,0x1412);assert(parse_calls==1&&init_calls==0&&entries[0].authorized);assert(receive_report(ctx,node,r,8)==0&&updates==1&&injected.buttons==0x4000&&injected.lx==0&&injected.ly==255);assert(!logs);
 node[1]=7;assert(receive_report(ctx,node,r+1,7)==0&&updates==2);node[1]=0;assert(receive_report(ctx,node,r+1,7)==0&&updates==2&&clears==1);
 assert(terminate(ctx,node)==0&&term_calls==0&&!entries[0].present);assert(receive_report(ctx,node,r,8)==77&&updates==2);
 reset();authorize(0x054c,0x0268);receive_report(ctx,node,r,8);assert(!updates&&!install_calls);
 reset();authorize(0x1038,0x1413);receive_report(ctx,node,r,8);assert(updates==1&&!install_calls&&entries[0].layout.mapping==1);
 reset();parse_result=-9;uint16_t ids[]={0x1412,0x1038};descriptor_capture(ctx,active_descriptor,active_length);initialize(ctx,node,ids,123);receive_report(ctx,node,r,8);assert(!updates);
 reset();authorize(0x1038,0x1412);term_after_snapshot=1;assert(receive_report(ctx,node,r,8)==0&&!report_calls);assert(!updates&&!entries[0].authorized); /* generation recheck after report snapshot */
 reset();authorize(0x1038,0x1412);node[0]=4;receive_report(ctx,node,r,8);assert(!updates); /* never steal DS3 reports */
 zeemote_input_log_tick(log_mock);assert(logs);unsigned prior=logs;zeemote_input_log_tick(log_mock);assert(logs==prior);
 reset();
 for(unsigned i=0;i<9;i++){ctx=(void *)(uintptr_t)(0x88001000+i*0x100);descriptor_capture(ctx,active_descriptor,active_length);assert(find(ctx));}
 assert(!counters.capacity);authorize(0x1038,0x1412);Entry *protected=find(ctx);unsigned protected_generation=protected->generation;
 for(unsigned i=0;i<9;i++){ctx=(void *)(uintptr_t)(0x88101000+i*0x100);descriptor_capture(ctx,active_descriptor,active_length);}
 assert(protected->authorized&&protected->generation==protected_generation&&!counters.capacity);
 reset();ctx=(void *)(uintptr_t)0x88001000;authorize(0x1038,0x1412);receive_report(ctx,node,r,8);
 SceModule lifecycle={"sceHIDSvc",base,0x4ec4};live_module=&lifecycle;installed_base=base;assert(input_active&&updates==1);
 zeemote_input_log_tick(log_mock);assert(keepalive_calls==1&&input_active&&updates==1&&install_calls==1); /* no synthetic input report */
 receive_report(ctx,node,r+1,7);assert(!input_active);zeemote_input_log_tick(log_mock);assert(!input_active&&updates==1);
 receive_report(ctx,node,r,8);assert(input_active&&updates==2);unsigned kept=keepalive_calls;live_module=NULL;zeemote_input_log_tick(log_mock);assert(!input_active&&!entries[0].authorized&&!entries[0].present&&keepalive_calls==kept);
 live_module=&lifecycle;zeemote_input_log_tick(log_mock);assert(!input_active&&updates==2&&keepalive_calls==kept); /* module reappearance does not recreate session */
 reset();authorize(0x1038,0x1412);receive_report(ctx,node,r,8);installed_base=base;live_module=&lifecycle;lifecycle.text_addr=base+4;zeemote_input_log_tick(log_mock);assert(!input_active&&!entries[0].authorized&&!keepalive_calls);
 /* Concurrent report callbacks use a try-lock, never mutable shared state
  * or waiting. Installation remains absent from the report call chain. */
 reset();authorize(0x1038,0x1412);nested_report=r;reenter_after_snapshot=1;receive_report(ctx,node,r,8);assert(updates==1&&counters.busy==1&&!report_scratch.busy&&!install_calls);
 report_scratch.busy=1;receive_report(ctx,node,r,8);assert(updates==1&&counters.busy==2&&report_scratch.busy);report_scratch.busy=0;
 receive_report(ctx,node,r+1,7);assert(!report_scratch.busy&&!input_active&&!install_calls);
 /* Native initializer failure cannot disable a validated adapter-owned node. */
 reset();init_result=-123;authorize(0x1038,0x1412);assert(!init_calls&&entries[0].authorized&&owned_index(ctx,node)>=0);receive_report(ctx,node,r,8);assert(updates==1);assert(!terminate(ctx,node)&&!term_calls);
 /* Ownership outlives descriptor-cache invalidation and never invokes cleanup
  * for native resources that were never allocated. */
 reset();authorize(0x1038,0x1412);descriptor_capture(ctx,active_descriptor,active_length);assert(!entries[0].authorized&&owned_index(ctx,node)>=0);assert(!terminate(ctx,node)&&!term_calls);
 reset();*(uint32_t *)(node+0x154)=1;authorize(0x1038,0x1412);assert(init_calls==1&&!entries[0].authorized&&owned_index(ctx,node)<0);assert(terminate(ctx,node)==88&&term_calls==1);
 /* Actual81-byte descriptor regression, with invented control states. */
 reset();active_descriptor=free_descriptor;active_length=sizeof(free_descriptor);authorize(0x1038,0x1412);assert(!init_calls&&entries[0].authorized);assert(entries[0].layout.payload_bytes==7&&entries[0].layout.report_id==1&&entries[0].layout.count==21);
 uint8_t actual[]={1,8,0,0,0,0,0,0};receive_report(ctx,node,actual,8);assert(updates==1&&!injected.buttons&&injected.lx==128&&injected.ly==128);
 actual[6]=1;actual[1]=2;actual[2]=0x80;actual[3]=0x7f;receive_report(ctx,node,actual,8);assert(updates==2&&injected.buttons==(0x4000|0x20)&&injected.lx==0&&injected.ly==255);
 node[1]=1;receive_report(ctx,node,actual+1,7);assert(updates==3);assert(!terminate(ctx,node)&&!term_calls&&!input_active);
 /* Native composite dispatch chooses the node by its Report ID, then
  * strips that byte. This reproduces the observed GameStick path. */
 reset();active_descriptor=gamestick_descriptor;active_length=sizeof(gamestick_descriptor);
 authorize(0x0f0d,0x1011);assert(entries[0].authorized&&entries[0].layout.report_id==3&&entries[0].layout.payload_bytes==9);
 assert(entries[0].route_before==0&&node[1]==3&&entries[0].route_after==3);
 uint8_t game_report[10]={3,0x0f,128,128,128,128,0,0,1,0};
 /* Keyboard1 and mouse2 remain different native nodes. */
 uint8_t routes[3]={1,2,node[1]};unsigned selected=99;
 for(unsigned i=0;i<3;i++)if(routes[i]==game_report[0]){selected=i;break;}
 assert(selected==2);assert(receive_report(ctx,node,game_report+1,9)==0&&updates==1&&injected.buttons==0x4000);
 assert(!terminate(ctx,node)&&!input_active&&!term_calls);
 /* Captured NES30 descriptor, with synthesized full/stripped reports. */
 reset();active_descriptor=nes30_descriptor;active_length=sizeof(nes30_descriptor);
 authorize(0x2dc8,0x3820);assert(entries[0].authorized&&entries[0].layout.mapping==2&&node[1]==3&&entries[0].layout.payload_bytes==9);
 assert(receive_report(ctx,node,game_report,10)==0&&updates==1&&injected.buttons==0x2000);
 assert(receive_report(ctx,node,game_report+1,9)==0&&updates==2&&injected.buttons==0x2000);
 assert(!terminate(ctx,node)&&!input_active&&!term_calls);
 /* DS4 basic projection is usable only with the matching native identity.
  * Both models share wire format. No real input is fabricated during init. */
 for(unsigned model=0;model<3;model++){
  reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;
  authorize(0x054c,model==2?0x0ce6:model?0x09cc:0x05c4);assert(entries[0].authorized&&entries[0].layout.mapping==3&&node[1]==1&&!updates&&!input_active);assert(zeemote_sony_context_owned(ctx));assert(!zeemote_sony_context_owned((void *)1));
  uint8_t ds4[10]={1,0,255,128,128,0x18,0x30,1,0,0};
  assert(!receive_report(ctx,node,ds4,10)&&updates==1&&injected_ds4==1&&injected.buttons==(0x8000|1|8|0x10000)&&injected.lx==0&&injected.ly==255);
  assert(!receive_report(ctx,node,ds4+1,9)&&updates==2&&injected.buttons==(0x8000|1|8|0x10000));
  ds4[0]=0x11;assert(!receive_report(ctx,node,ds4,10)&&updates==2&&!input_active);ds4[0]=1;
  assert(!receive_report(ctx,node,ds4+1,9)&&updates==3); /* nine bytes are explicitly native stripped framing */
  assert(!receive_report(ctx,node,ds4,8)&&updates==3&&!input_active);
  assert(!terminate(ctx,node)&&!input_active&&!term_calls);
 }
 reset();authorize(0x054c,0x05c4);assert(init_calls==1&&!entries[0].authorized); /* other descriptor is not silently decoded as DS4 basic */
 reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;authorize(0x054c,0x0268);assert(init_calls==1&&!entries[0].authorized); /* DS3 still native */
 reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;authorize(0x054c,0x05c4);
 uint8_t ds4_race[10]={1,128,128,128,128,0x28,0,0,0,0};term_after_snapshot=1;
 assert(!receive_report(ctx,node,ds4_race,10)&&!updates&&!entries[0].authorized&&!input_active);
 /* Explicit selected/live context may use the DS4 codec with unknown IDs;
  * a name or projected descriptor alone never authorizes that codec. */
 reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;
 authorize(0,0);assert(init_calls==1&&!entries[0].authorized);
 reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;
 local_authorized=1;zeemote_ds4_set_authorizer(local_allow);authorize(0,0);
 assert(!init_calls&&entries[0].authorized&&entries[0].layout.mapping==3&&node[1]==1&&!updates);
 uint8_t local_report[10]={1,128,128,128,128,0x18,0,0,0,0};
 assert(!receive_report(ctx,node,local_report,10)&&updates==1&&injected.buttons==0x8000);
 assert(!terminate(ctx,node)&&!term_calls&&!input_active);
 reset();active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;local_authorized=1;zeemote_ds4_set_authorizer(local_allow);authorize(0x054c,0x0268);assert(init_calls==1&&!entries[0].authorized);
 /* Reader observer preserves native data/results, including unhandled full
  * reports. It records only length/ID, never installs or publishes controls. */
 reset();memset(&ds4_read_stats,0,sizeof(ds4_read_stats));ds4_drain_original=raw_read;local_authorized=1;zeemote_ds4_set_authorizer(local_allow);
 uint8_t wire[64];memset(wire,0xa5,sizeof(wire));raw_calls=0;raw_result=10;raw_id=1;
 assert(ds4_read_trace(ctx,wire,64)==10&&raw_calls==1&&wire[0]==1&&wire[1]==0xa5&&ds4_read_stats.nonempty==1&&!updates&&!logs);
 raw_result=64;raw_id=0x11;assert(ds4_read_trace(ctx,wire,64)==64&&ds4_read_stats.id==0x11&&ds4_read_stats.length==64&&!updates);
 raw_result=0;assert(!ds4_read_trace(ctx,wire,64)&&ds4_read_stats.nonempty==2);
 raw_result=-123;assert(ds4_read_trace(ctx,wire,64)==-123&&ds4_read_stats.errors==1);
 raw_result=65;assert(ds4_read_trace(ctx,wire,64)==65&&ds4_read_stats.errors==2);
 local_authorized=0;unsigned raw_before=ds4_read_stats.reads;raw_result=10;assert(ds4_read_trace(ctx,wire,64)==10&&ds4_read_stats.reads==raw_before);
 reset();authorize(0x1038,0x1412);assert(node[1]==0); /* exact Free route kept */
 /* Installer context guards are exercised against a mock module image.
  * Host function alignment flag makes its synthetic JAL target representable. */
 base=((uint32_t)(uintptr_t)descriptor_capture&0xf0000000u)|0x0222f800u;
 const uint32_t offs[]={0x908,0x90c,0x904,0x8f4,0x8f8,0x4e64,0x4e68,0x4e4c,0x4e50,0x4e78,0x4e7c,0x4054,0x4058,0x3cc0,0x3cc4};uint32_t j;assert(jump(base+0x908,base+0x1000,&j));
 uint32_t words[]={j,0x02402021,0x27a50018,0x8fa60014,0x2cc70201,base+0x3cc0,base+0x3cc0,base+0x4054,base+0x4054,base+0x3fd0,base+0x3fd0,0x90a30000,0x27bdfff0,0x27bdffb0,0xafb40040};
 SceModule module={"sceHIDSvc",base,0x4ec4};
 for(unsigned bad=0;bad<sizeof(offs)/sizeof(offs[0]);bad++){
  installed_base=0;memset(memory,0,sizeof(memory));for(unsigned i=0;i<sizeof(offs)/sizeof(offs[0]);i++)_sw(words[i],base+offs[i]);_sw(words[bad]^1,base+offs[bad]);
  uint32_t lc[LIFE_GUARDS];life_expected(base,lc);for(unsigned i=0;i<LIFE_GUARDS;i++)_sw(lc[i],base+life_offsets[i]);
  uint32_t prior[sizeof(memory)/4];memcpy(prior,memory,sizeof(memory));assert(zeemote_input_install_module(&module,NULL)<0);assert(!memcmp(prior,memory,sizeof(memory)));
 }
 for(unsigned bad=0;bad<LIFE_GUARDS;bad++){
  installed_base=0;memset(memory,0,sizeof(memory));for(unsigned i=0;i<sizeof(offs)/sizeof(offs[0]);i++)_sw(words[i],base+offs[i]);
  uint32_t c[LIFE_GUARDS];life_expected(base,c);for(unsigned i=0;i<LIFE_GUARDS;i++)_sw(c[i],base+life_offsets[i]);
  _sw(c[bad]^1,base+life_offsets[bad]);uint32_t prior[sizeof(memory)/4];memcpy(prior,memory,sizeof(memory));
  assert(zeemote_input_install_module(&module,NULL)<0&&!memcmp(prior,memory,sizeof(memory)));
 }
 installed_base=0;memset(memory,0,sizeof(memory));for(unsigned i=0;i<sizeof(offs)/sizeof(offs[0]);i++)_sw(words[i],base+offs[i]);uint32_t lc[LIFE_GUARDS];life_expected(base,lc);for(unsigned i=0;i<LIFE_GUARDS;i++)_sw(lc[i],base+life_offsets[i]);assert(zeemote_input_install_module(&module,NULL)==1);
 assert(_lw(base+0x4e68)==base+0x3cc0&&_lw(base+0x4e50)==base+0x4054&&_lw(base+0x4e7c)==base+0x3fd0);assert(zeemote_input_install_module(&module,NULL)==1);
 ds4_trace_base=0;const unsigned ro[]={0xe6c,0xe70,0xe74,0xe78,0xf58,0xf5c};uint32_t rj;assert(jump(base+0xe70,base+0xf58,&rj));
 const uint32_t rw[]={0x03a02821,rj,0x24060040,0x18400025,0x27bdffe0,0xafb10004};
 for(unsigned bad=0;bad<6;bad++){
  ds4_trace_base=0;for(unsigned i=0;i<6;i++)_sw(rw[i],base+ro[i]);_sw(rw[bad]^1,base+ro[bad]);
  uint32_t prior[sizeof(memory)/4];memcpy(prior,memory,sizeof(memory));assert(zeemote_ds4_install_read_trace(&module,NULL)<0&&!memcmp(prior,memory,sizeof(memory)));
 }
 ds4_trace_base=0;for(unsigned i=0;i<6;i++)_sw(rw[i],base+ro[i]);assert(zeemote_ds4_install_read_trace(&module,NULL)==1);
 /* Reboot quiesce: old native input pump must observe empty even if its
  * backend would continuously return Sony reports. Do not consume/free queues. */
 reset();ds4_trace_base=base;ds4_drain_original=raw_read;ds4_quiesced_context=NULL;
 active_descriptor=ds4_basic_descriptor;active_length=DS4_BASIC_DESCRIPTOR_SIZE;
 authorize(0x054c,0x05c4);raw_result=10;raw_calls=0;raw_id=1;uint8_t stop_buffer[64]={0};
 assert(ds4_read_trace(ctx,stop_buffer,64)==10&&raw_calls==1);
 assert(!zeemote_sony_quiesce_context(NULL));assert(!zeemote_sony_quiesce_context((void *)1));
 ds4_trace_base=0;assert(!zeemote_sony_quiesce_context(ctx));ds4_trace_base=base;
 assert(zeemote_sony_quiesce_context(ctx));assert(zeemote_sony_quiesce_context(ctx));
 memset(stop_buffer,0xa5,sizeof(stop_buffer));
 for(unsigned n=0;n<128;n++)assert(ds4_read_trace(ctx,stop_buffer,64)==0);
 assert(raw_calls==1&&stop_buffer[0]==0xa5&&stop_buffer[63]==0xa5);
 assert(!terminate(ctx,node));assert(ds4_read_trace(ctx,stop_buffer,64)==0&&raw_calls==1);
 reset();active_descriptor=gamestick_descriptor;active_length=sizeof(gamestick_descriptor);authorize(0x0f0d,0x1011);ds4_quiesced_context=NULL;
 assert(!zeemote_sony_quiesce_context(ctx));assert(ds4_read_trace(ctx,stop_buffer,64)==10&&raw_calls==2);
 puts("PASS: real descriptor parser plus mocked native init/report/term callbacks, generic descriptor ownership with native DS3 exclusion and Zeemote-specific mapping, full/stripped IDs, malformed neutral, disconnect invalidation, generation race, untouched DS3 path and deferred logging");
}
