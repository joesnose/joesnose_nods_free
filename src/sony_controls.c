#include "sony_controls.h"
#include "ds4_basic.h"
#include <string.h>
SonyControls sony_controls_active;
static const char *const button_names[SONY_BUTTONS]={"cross","circle","square","triangle","l1","r1","l2","r2","l3","r3","create","options","ps","touchpad","dpad_up","dpad_down","dpad_left","dpad_right"};
static const char *const direction_names[SONY_DIRECTIONS]={"left_up","left_down","left_left","left_right","right_up","right_down","right_left","right_right"};
static const char *const axis_names[4]={"left_x","left_y","right_x","right_y"};
static void standard(SonyControls *c){
 memset(c,0,sizeof(*c));
 const uint32_t buttons[SONY_BUTTONS]={0x4000,0x2000,0x8000,0x1000,0x100,0x200,0x100,0x200,0,0,1,8,0x10000,0,0x10,0x40,0x80,0x20};
 memcpy(c->buttons,buttons,sizeof(buttons));c->axes[0]=1;c->axes[1]=2;c->deadzone[0]=16;c->deadzone[1]=32;
}
static void fps(SonyControls *c){
 standard(c);c->buttons[6]=0x8000;c->buttons[7]=0x4000;
 c->directions[4]=0x1000;c->directions[5]=0x4000;c->directions[6]=0x8000;c->directions[7]=0x2000;
}
void sony_controls_defaults(SonyControlFile *f){standard(&f->vsh);fps(&f->game);}
static int equal(const char *a,const char *b){
 while(*a&&*b){unsigned x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}return !*a&&!*b;
}
static char *trim(char *p){while(*p==' '||*p=='\t'||*p=='\r')p++;char *end=p+strlen(p);while(end>p&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'))*--end=0;return p;}
static int targets(char *text,uint32_t *result){
 static const char *const names[]={"none","cross","x","circle","square","triangle","l","r","select","start","home","up","down","left","right","analog_up","analog_down","analog_left","analog_right"};
 static const uint32_t values[]={0,0x4000,0x4000,0x2000,0x8000,0x1000,0x100,0x200,1,8,0x10000,0x10,0x40,0x80,0x20,SONY_ANALOG_UP,SONY_ANALOG_DOWN,SONY_ANALOG_LEFT,SONY_ANALOG_RIGHT};
 uint32_t out=0;char *p=text;
 for(;;){char *next=p;while(*next&&*next!='+')next++;int more=*next!=0;if(more)*next=0;char *token=trim(p);unsigned i;
  for(i=0;i<sizeof(values)/sizeof(values[0]);i++)if(equal(token,names[i]))break;
  if(i==sizeof(values)/sizeof(values[0]))return 0;
  out|=values[i];if(!more)break;p=next+1;
 }
 *result=out;return 1;
}
static int setting(SonyControls *c,const char *key,char *value){
 if(equal(key,"preset")){if(equal(value,"standard")){standard(c);return 1;}if(equal(value,"fps")){fps(c);return 1;}return 0;}
 for(unsigned i=0;i<SONY_BUTTONS;i++)if(equal(key,button_names[i])||(i==10&&equal(key,"share")))return targets(value,&c->buttons[i]);
 for(unsigned i=0;i<SONY_DIRECTIONS;i++)if(equal(key,direction_names[i]))return targets(value,&c->directions[i]);
 for(unsigned i=0;i<4;i++)if(equal(key,axis_names[i])){
  int sign=1;if(*value=='-'){sign=-1;value++;}
  if(equal(value,"none")){c->axes[i]=0;return 1;}
  if(equal(value,"analog_x")){c->axes[i]=(int8_t)sign;return 1;}
  if(equal(value,"analog_y")){c->axes[i]=(int8_t)(sign*2);return 1;}
  return 0;
 }
 for(unsigned i=0;i<2;i++)if(equal(key,i?"r2_axis":"l2_axis")){
  const char *const names[]={"none","analog_left","analog_right","analog_up","analog_down"};
  for(unsigned j=0;j<5;j++)if(equal(value,names[j])){c->triggers[i]=(uint8_t)j;return 1;}
  return 0;
 }
 for(unsigned i=0;i<2;i++)if(equal(key,i?"right_deadzone":"left_deadzone")){
  if(!*value)return 0;
  unsigned n=0;for(char *p=value;*p;p++){if(*p<'0'||*p>'9'||n>127)return 0;n=n*10+(unsigned)(*p-'0');}if(n>127)return 0;c->deadzone[i]=(uint8_t)n;return 1;
 }
 return 0;
}
int sony_controls_parse(const char *text,size_t size,SonyControlFile *file,unsigned *error_line){
 SonyControlFile next;sony_controls_defaults(&next);SonyControls *section=NULL;unsigned line=0;size_t at=0;
 if(error_line)*error_line=0;
 if(!text||!file||size>8192)return 0;
 if(size>=3&&(unsigned char)text[0]==0xef&&(unsigned char)text[1]==0xbb&&(unsigned char)text[2]==0xbf)at=3;
 while(at<size){char buf[160];unsigned n=0;line++;
  while(at<size&&text[at]!='\n'){unsigned ch=(unsigned char)text[at++];if(!ch||n>=sizeof(buf)-1)goto bad;buf[n++]=(char)ch;}
  if(at<size)at++;
  buf[n]=0;
  for(unsigned i=0;i<n;i++)if(buf[i]=='#'||buf[i]==';'){buf[i]=0;break;}
  char *p=trim(buf);if(!*p)continue;
  if(*p=='['){char *end=p+strlen(p)-1;if(*end!=']')goto bad;*end=0;p=trim(p+1);if(equal(p,"vsh"))section=&next.vsh;else if(equal(p,"game"))section=&next.game;else goto bad;continue;}
  char *eq=p;while(*eq&&*eq!='=')eq++;if(!*eq||!section)goto bad;*eq=0;if(!setting(section,trim(p),trim(eq+1)))goto bad;
 }
 *file=next;return 1;
 bad:if(error_line)*error_line=line;return 0;
}
static unsigned distance(unsigned x){return x<128?128-x:x-128;}
static void axis_candidate(unsigned *out,unsigned v){if(distance(v)>distance(*out))*out=v;}
int sony_controls_decode(const uint8_t *r,size_t size,ZeeState *out){
 ZeeState base;if(ds4_basic_decode(r,size,&base)){if(out)*out=base;return ZEE_INVALID;}if(!out)return ZEE_INVALID;
 const SonyControls *c=&sony_controls_active;uint32_t action=0;
 const uint8_t pressed[SONY_BUTTONS]={!!(r[5]&0x20),!!(r[5]&0x40),!!(r[5]&0x10),!!(r[5]&0x80),!!(r[6]&1),!!(r[6]&2),!!(r[6]&4),!!(r[6]&8),!!(r[6]&0x40),!!(r[6]&0x80),!!(r[6]&0x10),!!(r[6]&0x20),!!(r[7]&1),!!(r[7]&2),!!(base.buttons&0x10),!!(base.buttons&0x40),!!(base.buttons&0x80),!!(base.buttons&0x20)};
 for(unsigned i=0;i<SONY_BUTTONS;i++)if(pressed[i])action|=c->buttons[i];
 unsigned x=128,y=128;
 for(unsigned i=0;i<4;i++){
  unsigned v=r[i+1],dz=c->deadzone[i/2];int low=v<128-dz,high=v>128+dz;
  if(low)action|=c->directions[(i/2)*4+(i&1?0:2)];
  if(high)action|=c->directions[(i/2)*4+(i&1?1:3)];
  if(!low&&!high)continue;
  int dest=c->axes[i];if(dest<0){dest=-dest;v=255-v;}
  if(dest==1)axis_candidate(&x,v);else if(dest==2)axis_candidate(&y,v);
 }
 for(unsigned i=0;i<2;i++){
  unsigned v=r[8+i],kind=c->triggers[i];if(!v||!kind)continue;
  unsigned delta=(v*(kind==1||kind==3?128u:127u)+127u)/255u;
  if(kind==1)axis_candidate(&x,128-delta);else if(kind==2)axis_candidate(&x,128+delta);
  else if(kind==3)axis_candidate(&y,128-delta);else if(kind==4)axis_candidate(&y,128+delta);
 }
 unsigned left=!!(action&SONY_ANALOG_LEFT),right=!!(action&SONY_ANALOG_RIGHT),up=!!(action&SONY_ANALOG_UP),down=!!(action&SONY_ANALOG_DOWN);
 if(left||right)x=left==right?128:left?0:255;
 if(up||down)y=up==down?128:up?0:255;
 *out=(ZeeState){action&0x1ffffu,(uint8_t)x,(uint8_t)y,base.rx,base.ry};return ZEE_OK;
}
