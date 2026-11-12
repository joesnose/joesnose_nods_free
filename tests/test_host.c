#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "patch_plan.h"
int main(void){
 char text[20];memset(text,'!',sizeof(text));
 uint8_t a[]={0x02,0x17,0x3b,0x80,0xa5,0xfe};
 assert(!host_address_line(a,text));assert(!memcmp(text,"02:17:3B:80:A5:FE\n",18));assert(text[18]=='!');
 uint8_t zero[6]={0},ones[6]={255,255,255,255,255,255};
 assert(host_address_line(zero,text));assert(host_address_line(ones,text));
 assert(host_address_line(0,text));assert(host_address_line(a,0));
 assert(host_query_allowed(0x88239200,0x3c028824,0x9045705b));
 assert(!host_query_allowed(0x88239200,0x3c038824,0x9045705b));
 assert(!host_query_allowed(0x88239200,0x3c028824,0x9045705a));
 assert(host_query_allowed(0x8823a200,0x3c028825,0x9045805b));
 puts("PASS: Bluetooth address output bounds, byte order and relocated read-query guard");
}
