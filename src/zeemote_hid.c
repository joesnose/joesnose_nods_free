/* Original bounded HID gamepad decoder. MIT. */
#include "zeemote_hid.h"
#include <string.h>
typedef struct {uint16_t page,size,count;uint8_t id;int32_t minimum,maximum;} Global;
typedef struct {uint32_t values[32],minimum,maximum;unsigned count;int range;} Local;
static uint32_t number(const uint8_t *p,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static int32_t signed_number(uint32_t v,unsigned n){if(n&&n<4&&(v&(1u<<(n*8-1))))v|=~((1u<<(n*8))-1);return(int32_t)v;}
static uint32_t usage(uint32_t v,unsigned n,uint16_t page){return n==4?v:((uint32_t)page<<16)|v;}
static int add_field(ZeeLayout *l,unsigned offset,unsigned bits,unsigned kind,const Global *g,unsigned null_state){
 if(l->count>=ZEE_MAX_FIELDS||!bits||bits>16||offset+bits>512)return ZEE_UNSUPPORTED;
 for(unsigned i=0;i<l->count;i++)if(l->fields[i].kind==kind)return ZEE_UNSUPPORTED;
 ZeeField *f=&l->fields[l->count++];f->offset=offset;f->bits=bits;f->kind=kind;f->minimum=g->minimum;f->maximum=g->maximum;f->null_state=null_state!=0;return 0;
}
int zee_parse_descriptor(const uint8_t *d,size_t n,ZeeLayout *out){
 if(!d||!out||!n||n>512)return ZEE_INVALID;
 ZeeLayout l;memset(&l,0,sizeof(l));Global g={0},stack[4];unsigned sp=0,depth=0;uint8_t gamepad[8]={0};Local local={0};uint16_t offsets[256]={0};int selected=-1,numbered=0;uint32_t kinds=0;
 for(size_t at=0;at<n;){
  uint8_t prefix=d[at++];if(prefix==0xfe)return ZEE_UNSUPPORTED;unsigned bytes=prefix&3;if(bytes==3)bytes=4;if(bytes>n-at)return ZEE_INVALID;
  uint32_t v=number(d+at,bytes);at+=bytes;unsigned type=(prefix>>2)&3,tag=prefix>>4;
  if(type==1){switch(tag){
   case 0:if(v>65535)return ZEE_UNSUPPORTED;g.page=v;break;
   case 1:g.minimum=signed_number(v,bytes);break;
   case 2:g.maximum=g.minimum<0?signed_number(v,bytes):(int32_t)v;if(g.minimum>=0&&v>0x7fffffff)return ZEE_UNSUPPORTED;break;
   case 3:case 4:case 5:case 6:break; /* physical ranges/units do not change bit layout */
   case 7:if(v>32)return ZEE_UNSUPPORTED;g.size=v;break;
   case 8:if(!v||v>255)return ZEE_INVALID;g.id=v;numbered=1;break;
   case 9:if(v>65535)return ZEE_UNSUPPORTED;g.count=v;break;
   case 10:if(bytes||sp==4)return ZEE_INVALID;stack[sp++]=g;break;
   case 11:if(bytes||!sp)return ZEE_INVALID;g=stack[--sp];break;
   default:return ZEE_UNSUPPORTED;
  }continue;}
  if(type==2){switch(tag){
   case 0:if(local.count==32)return ZEE_UNSUPPORTED;local.values[local.count++]=usage(v,bytes,g.page);break;
   case 1:local.minimum=usage(v,bytes,g.page);local.range|=1;break;
   case 2:local.maximum=usage(v,bytes,g.page);local.range|=2;break;
   default:return ZEE_UNSUPPORTED;
  }continue;}
  if(type!=0)return ZEE_UNSUPPORTED;
  if(tag==10){ /* Collection */
   if(!bytes||depth==8)return ZEE_INVALID;
   uint32_t u=local.count?local.values[0]:local.minimum;
   gamepad[depth]=(depth?gamepad[depth-1]:0)||(v==1&&(u==0x00010005||u==0x00010004));depth++;
  }else if(tag==12){if(bytes||!depth)return ZEE_INVALID;depth--;}
  else if(tag==8){
   if(!bytes||!depth||!g.count||!g.size)return ZEE_INVALID;
   unsigned total=g.count*g.size,start=offsets[g.id];if(total>512-start)return ZEE_UNSUPPORTED;offsets[g.id]=start+total;
   if(gamepad[depth-1]&&!(v&1)){
    if(!(v&2)||(v&4)||g.maximum<=g.minimum)return ZEE_UNSUPPORTED;
    if(local.range&&((local.range!=3)||local.maximum<local.minimum||(local.maximum>>16)!=(local.minimum>>16)))return ZEE_INVALID;
    for(unsigned i=0;i<g.count;i++){
     uint32_t u=0;if(local.count)u=local.values[i<local.count?i:local.count-1];else if(local.range==3&&i<=local.maximum-local.minimum)u=local.minimum+i;
     unsigned kind=0;
     if((u>>16)==9&&(u&65535)>=1&&(u&65535)<=16)kind=8+(u&65535);
     else if(u==0x10030)kind=1;else if(u==0x10031)kind=2;else if(u==0x10032||u==0x10033)kind=3;else if(u==0x10034||u==0x10035)kind=4;else if(u==0x10039)kind=5;else if(u>=0x10090&&u<=0x10093)kind=25+(u-0x10090);
     if(!kind)continue;
     if(selected>=0&&selected!=g.id)return ZEE_UNSUPPORTED;
     selected=g.id;
     if(g.size>16)return ZEE_UNSUPPORTED;
     if(kind<=4&&(g.minimum< -32768||g.maximum>65535))return ZEE_UNSUPPORTED;
     if(kind==5&&(int64_t)g.maximum-g.minimum!=7)return ZEE_UNSUPPORTED;
     if(g.minimum<0){int32_t limit=1<<(g.size-1);if(g.minimum< -limit||g.maximum>=limit)return ZEE_UNSUPPORTED;}
     else if((uint32_t)g.maximum>((1u<<g.size)-1))return ZEE_UNSUPPORTED;
     if(kind>=9&&(g.minimum!=0||g.maximum!=1))return ZEE_UNSUPPORTED;
     int result=add_field(&l,start+i*g.size,g.size,kind,&g,v&0x40);if(result)return result;
     if(kind<=5)kinds|=1u<<kind;else if(kind<=24)kinds|=1u<<8;else kinds|=1u<<(kind-16);
    }
   }
  }else if(tag!=9&&tag!=11)return ZEE_UNSUPPORTED; /* output/feature do not advance input offsets */
  memset(&local,0,sizeof(local));
 }
 if(depth||sp||selected<0||(!(kinds&0x100)||(!(kinds&0x20)&&(kinds&0x6)!=0x6&&(kinds&0x1e00)!=0x1e00)))return ZEE_UNSUPPORTED;
 if(numbered&&offsets[0])return ZEE_UNSUPPORTED;
 l.numbered=numbered;l.report_id=selected;l.payload_bytes=(offsets[selected]+7)/8;
 if(!l.payload_bytes||l.payload_bytes+numbered>64)return ZEE_UNSUPPORTED;
 *out=l;return 0;
}
static uint32_t bits_at(const uint8_t *p,unsigned offset,unsigned count){uint32_t v=0;for(unsigned i=0;i<count;i++)v|=((p[(offset+i)/8]>>((offset+i)%8))&1u)<<i;return v;}
int zee_decode_report(const ZeeLayout *l,const uint8_t *p,size_t n,ZeeState *out){
 if(!l||!p||!out||l->numbered>1||!l->count||l->count>ZEE_MAX_FIELDS||n!=(size_t)l->payload_bytes+l->numbered||n>64)return ZEE_INVALID;
 if(l->numbered){if(*p++!=l->report_id)return ZEE_UNSUPPORTED;}
 ZeeState s={0,128,128,128,128};
 /* HID button usages converted through Linux's gamepad button numbering and
  * AOSP's 1038:1412 key layout. Unmapped buttons intentionally do nothing. */
 static const uint32_t buttons[17]={0,0x4000,0x2000,0,0x8000,0x1000,0,0x100,0x200,0,0,0,8,1,0,0,0};
 static const uint32_t generic_buttons[17]={0,0x4000,0x2000,0x8000,0x1000,0x100,0x200,1,8,0,0,0x10000,0,0,0,0,0};
 /* Original NES30 Pro2dc8:3820 Bluetooth button indices match SDL's
  * device mapping: Guide3, face1/2/4/5, shoulders7/8, Select11, Start12.
  * Keep v0.42 Cross/Triangle and swap Square/Circle per hardware feedback;
  * leave extra buttons neutral. */
 static const uint32_t nes30_buttons[17]={0,0x2000,0x4000,0x10000,0x1000,0x8000,0,0x100,0x200,0,0,1,8,0,0,0,0};
 static const uint32_t dpad[4]={0x10,0x40,0x20,0x80};
 static const uint32_t hats[8]={0x10,0x30,0x20,0x60,0x40,0xc0,0x80,0x90};
 for(unsigned i=0;i<l->count;i++){
  const ZeeField *f=&l->fields[i];if(!f->bits||f->bits>16||f->offset+f->bits>l->payload_bytes*8u)return ZEE_INVALID;
  uint32_t raw=bits_at(p,f->offset,f->bits);int32_t value=(int32_t)raw;
  if(f->minimum<0&&(raw&(1u<<(f->bits-1))))value=(int32_t)(raw|(~0u<<f->bits));
  if(value<f->minimum||value>f->maximum){if(f->kind==5&&f->null_state)continue;return ZEE_INVALID;}
  if(f->kind>=9&&f->kind<=24){if(value)s.buttons|=(l->mapping==2?nes30_buttons:l->mapping?generic_buttons:buttons)[f->kind-8];}
  else if(f->kind>=25&&f->kind<=28){if(value)s.buttons|=dpad[f->kind-25];}
  else if(f->kind==5){if((int64_t)f->maximum-f->minimum!=7)return ZEE_INVALID;s.buttons|=hats[value-f->minimum];}
  else if(f->kind>=1&&f->kind<=4){if(f->maximum<=f->minimum)return ZEE_INVALID;uint8_t axis=(uint8_t)((((int64_t)value-f->minimum)*255+(f->maximum-f->minimum)/2)/(f->maximum-f->minimum));if(f->kind==1)s.lx=axis;else if(f->kind==2)s.ly=axis;else if(f->kind==3)s.rx=axis;else s.ry=axis;}
  else return ZEE_UNSUPPORTED;
 }
 *out=s;return 0;
}
