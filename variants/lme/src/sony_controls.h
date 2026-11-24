/* User-configurable basic DS4/DualSense controls. Original MIT. */
#ifndef SONY_CONTROLS_H
#define SONY_CONTROLS_H
#include <stdint.h>
#include <stddef.h>
#include "zeemote_hid.h"
#define SONY_BUTTONS 18
#define SONY_DIRECTIONS 8
#define SONY_ANALOG_UP (1u<<24)
#define SONY_ANALOG_DOWN (1u<<25)
#define SONY_ANALOG_LEFT (1u<<26)
#define SONY_ANALOG_RIGHT (1u<<27)
typedef struct {
 uint32_t buttons[SONY_BUTTONS],directions[SONY_DIRECTIONS];
 int8_t axes[4]; /*0 none, +/-1 PSP X, +/-2 PSP Y */
 uint8_t triggers[2]; /*0 none,1 left,2 right,3 up,4 down */
 uint8_t deadzone[2];
} SonyControls;
typedef struct {SonyControls vsh,game;} SonyControlFile;
extern SonyControls sony_controls_active;
void sony_controls_defaults(SonyControlFile *file);
/* Transactional parse: error leaves caller's file unchanged. */
int sony_controls_parse(const char *text,size_t size,SonyControlFile *file,unsigned *error_line);
int sony_controls_decode(const uint8_t *report,size_t size,ZeeState *state);
#endif
