/* Selected DS4 transition policy; native authentication remains live. */
#ifndef DS4_TRANSITION_H
#define DS4_TRANSITION_H
static inline int ds4_transition_defer(unsigned context,int selected,int linked){(void)context;return selected&&linked;}
static inline int ds4_transition_mode(unsigned context,int selected,int mode){(void)context;(void)selected;return mode;}
#endif
