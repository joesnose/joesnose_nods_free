#include <assert.h>
#include <stdio.h>
#include "../src/ds4_basic.c"
#include "../src/sony_controls.c"
static void neutral(uint8_t r[10]){const uint8_t p[10]={1,128,128,128,128,8,0,0,0,0};memcpy(r,p,10);}
static ZeeState decode(uint8_t r[10]){ZeeState s;assert(!sony_controls_decode(r,10,&s));return s;}
int main(void){
 SonyControlFile f;sony_controls_defaults(&f);uint8_t r[10];neutral(r);sony_controls_active=f.game;ZeeState s=decode(r);assert(!s.buttons&&s.lx==128&&s.ly==128);
 r[3]=0;s=decode(r);assert(s.buttons==0x8000&&s.lx==128&&s.ly==128);r[3]=255;assert(decode(r).buttons==0x2000);
 r[4]=0;assert(decode(r).buttons==(0x2000|0x1000));neutral(r);r[4]=255;assert(decode(r).buttons==0x4000);
 neutral(r);r[6]=4;assert(decode(r).buttons==0x8000);r[6]=8;assert(decode(r).buttons==0x4000);
 sony_controls_active=f.vsh;r[6]=4;assert(decode(r).buttons==0x100);r[6]=8;assert(decode(r).buttons==0x200);
 const unsigned bytes[14]={5,5,5,5,6,6,6,6,6,6,6,6,7,7};
 const unsigned bits[14]={0x20,0x40,0x10,0x80,1,2,4,8,0x40,0x80,0x10,0x20,1,2};
 const uint32_t expected[14]={0x4000,0x2000,0x8000,0x1000,0x100,0x200,0x100,0x200,0,0,1,8,0x10000,0};
 for(unsigned i=0;i<14;i++){neutral(r);r[bytes[i]]|=(uint8_t)bits[i];assert(decode(r).buttons==expected[i]);}
 const char *custom="\xef\xbb\xbf[game]\r\npreset = standard\r\nleft_x=none\nright_x=-analog_x\nl3=home\nr3=start+select\nps=none\ntouchpad=circle\ndpad_up=analog_up\nl2_axis=analog_left\nright_deadzone=0\n";unsigned line;
 assert(sony_controls_parse(custom,strlen(custom),&f,&line));sony_controls_active=f.game;
 neutral(r);r[3]=0;assert(decode(r).lx==255);r[3]=128;r[6]=0x40;assert(decode(r).buttons==0x10000);r[6]=0x80;assert(decode(r).buttons==(8|1));
 neutral(r);r[7]=1;assert(!decode(r).buttons);r[7]=2;assert(decode(r).buttons==0x2000);r[5]=0;s=decode(r);assert(s.ly==0&&s.buttons==0x2000);
 neutral(r);r[8]=255;assert(decode(r).lx==0);r[8]=128;assert(decode(r).lx==64);
 const char both[]="[game]\npreset=standard\nright_x=analog_x\nleft_deadzone=0\nright_deadzone=0\n";
 assert(sony_controls_parse(both,sizeof(both)-1,&f,&line));sony_controls_active=f.game;
 neutral(r);r[1]=64;r[3]=192;assert(decode(r).lx==64);r[3]=255;assert(decode(r).lx==255);r[1]=0;assert(decode(r).lx==0);
 const char cancel[]="[game]\npreset=standard\ncross=analog_left\ncircle=analog_right\n";assert(sony_controls_parse(cancel,sizeof(cancel)-1,&f,&line));sony_controls_active=f.game;
 neutral(r);r[5]=0x68;assert(decode(r).lx==128);
 /* Every stick axis can target either PSP axis, with inversion. */
 for(unsigned axis=0;axis<4;axis++)for(int dest=-2;dest<=2;dest++){
  memset(&sony_controls_active,0,sizeof(sony_controls_active));sony_controls_active.axes[axis]=(int8_t)dest;
  neutral(r);r[axis+1]=0;s=decode(r);unsigned expected=dest<0?255:0;
  assert(s.lx==((dest==1||dest==-1)?expected:128)&&s.ly==((dest==2||dest==-2)?expected:128));
 }
 for(unsigned trigger=0;trigger<2;trigger++)for(unsigned dest=1;dest<=4;dest++){
  memset(&sony_controls_active,0,sizeof(sony_controls_active));sony_controls_active.triggers[trigger]=(uint8_t)dest;neutral(r);r[8+trigger]=255;s=decode(r);
  assert(s.lx==(dest==1?0:dest==2?255:128)&&s.ly==(dest==3?0:dest==4?255:128)&&!s.buttons);
 }
 SonyControlFile saved=f;assert(!sony_controls_parse("x",8193,&f,&line)&&!memcmp(&f,&saved,sizeof(f)));const char *bad[]={"[unknown]","cross=circle","[game]\nleft_deadzone=128","[game]\nleft_x=analog_z","[vsh]\nl2_axis=analog_x","[game]\nr2=cross+","[game]\nwrong=cross","[game]\npreset=nope","[game]\nright_deadzone=-1"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){assert(!sony_controls_parse(bad[i],strlen(bad[i]),&f,&line)&&line>0&&!memcmp(&f,&saved,sizeof(f)));}
 neutral(r);r[0]=0x31;s=(ZeeState){.buttons=~0u,.lx=0,.ly=0};assert(sony_controls_decode(r,10,&s)==ZEE_INVALID&&!s.buttons&&s.lx==128&&s.ly==128);
 FILE *config=fopen("pspgo_hid_controls.txt","rb");assert(config);char text[8193];size_t n=fread(text,1,sizeof(text),config);assert(!ferror(config)&&feof(config));fclose(config);
 SonyControlFile defaults;sony_controls_defaults(&defaults);assert(sony_controls_parse(text,n,&f,&line)&&!memcmp(&f,&defaults,sizeof(f)));
 puts("PASS: confirmed FPS defaults, all14 basic buttons, sticks/diagonals, trigger axes, combos, inversion, strongest-axis/cancellation, atomic config errors, CRLF/BOM, malformed reports and shipped full config");
}
