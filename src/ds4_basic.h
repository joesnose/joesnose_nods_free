/* Original host-side basic DS4 projection and exact identity policy. MIT.
 * This describes the documented 10-byte BT report, not a captured descriptor.
 * No pairing, feature requests or full-report mode switching. */
#ifndef DS4_BASIC_H
#define DS4_BASIC_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "zeemote_hid.h"
#define DS4_BASIC_REPORT_SIZE 10u
#define DS4_BASIC_DESCRIPTOR_SIZE 68u
static const uint8_t ds4_basic_descriptor[DS4_BASIC_DESCRIPTOR_SIZE]={
 0x05,0x01,0x09,0x05,0xa1,0x01,0x85,0x01,0x09,0x30,0x09,0x31,
 0x09,0x32,0x09,0x35,0x15,0x00,0x26,0xff,0x00,0x75,0x08,0x95,
 0x04,0x81,0x02,0x09,0x39,0x15,0x00,0x25,0x07,0x75,0x04,0x95,
 0x01,0x81,0x42,0x05,0x09,0x19,0x01,0x29,0x0e,0x15,0x00,0x25,
 0x01,0x75,0x01,0x95,0x0e,0x81,0x02,0x75,0x06,0x95,0x01,0x81,
 0x03,0x75,0x08,0x95,0x02,0x81,0x03,0xc0,
};
static inline int ds4_basic_identity(unsigned vendor,unsigned product){
 return vendor==0x054c&&(product==0x05c4||product==0x09cc);
}
/* DualSense standard model shares the initial 10-byte Bluetooth report01. */
static inline int sony_basic_identity(unsigned vendor,unsigned product){return ds4_basic_identity(vendor,product)||(vendor==0x054c&&product==0x0ce6);}
static inline int ds4_basic_projection(const uint8_t *descriptor,unsigned size){
 return descriptor&&size==DS4_BASIC_DESCRIPTOR_SIZE&&!memcmp(descriptor,ds4_basic_descriptor,size);
}
#define DS4_BASIC_ATTRIBUTE_SIZE (11u+DS4_BASIC_DESCRIPTOR_SIZE)
static inline int ds4_basic_projected_attribute(void *buffer,unsigned capacity){
 if(!buffer||capacity<DS4_BASIC_ATTRIBUTE_SIZE)return 0;
 const uint8_t prefix[11]={0x09,0x02,0x06,0x35,DS4_BASIC_DESCRIPTOR_SIZE+6,0x35,DS4_BASIC_DESCRIPTOR_SIZE+4,0x08,0x22,0x25,DS4_BASIC_DESCRIPTOR_SIZE};
 memcpy(buffer,prefix,sizeof(prefix));memcpy((uint8_t *)buffer+sizeof(prefix),ds4_basic_descriptor,DS4_BASIC_DESCRIPTOR_SIZE);return DS4_BASIC_ATTRIBUTE_SIZE;
}
static inline int ds4_basic_descriptor_attribute(unsigned slot,unsigned vendor,unsigned product,unsigned service,unsigned attribute,void *buffer,unsigned capacity,int callbacks){
 if(slot<1||slot>8||!sony_basic_identity(vendor,product)||service!=0x1124||attribute!=0x206||callbacks!=0)return 0;
 return ds4_basic_projected_attribute(buffer,capacity);
}
/* Exact report01 framing. Invalid input resets state to neutral; no input is
 * published until a valid real packet is received. */
int ds4_basic_decode(const uint8_t *report,size_t length,ZeeState *state);
#endif
