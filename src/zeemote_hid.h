#ifndef ZEEMOTE_HID_H
#define ZEEMOTE_HID_H
#include <stdint.h>
#include <stddef.h>
#define ZEE_MAX_FIELDS 32
/* No fixed Zeemote wire offsets. Layout comes exclusively from a validated HID
 * descriptor. This adapter deliberately supports one complete gamepad report. */
typedef struct {uint16_t offset;uint8_t bits,kind;int32_t minimum,maximum;uint8_t null_state;} ZeeField;
typedef struct {ZeeField fields[ZEE_MAX_FIELDS];uint16_t payload_bytes;uint8_t count,report_id,numbered,mapping;} ZeeLayout;
typedef struct {uint32_t buttons;uint8_t lx,ly,rx,ry;} ZeeState;
enum {ZEE_OK=0,ZEE_INVALID=-1,ZEE_UNSUPPORTED=-2};
int zee_parse_descriptor(const uint8_t *descriptor,size_t length,ZeeLayout *layout);
/* Includes report ID byte iff descriptor uses report IDs. Native callback
 * strips numbered IDs; its adapter must restore/validate the node's ID. */
int zee_decode_report(const ZeeLayout *layout,const uint8_t *report,size_t length,ZeeState *state);
#endif
