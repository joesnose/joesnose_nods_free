#ifndef TRANSITION_PRESERVE_H
#define TRANSITION_PRESERVE_H
typedef int (*TransitionControl)(unsigned,void *,int);
static int transition_thread=-1;
static void (*transition_walk_original)(void);
extern void transition_control_bridge(void);
static void transition_reboot_walk(void){
 int previous=transition_thread;
 transition_thread=sceKernelGetThreadId();
 transition_walk_original();
 transition_thread=previous;
}
int transition_control_receive(unsigned op,void *buffer,int cb,TransitionControl original,void *context){
 if(transition_thread>=0&&transition_thread==sceKernelGetThreadId()&&op==6&&!buffer&&!cb&&
    ds4_context_authorized(context)&&zeemote_sony_context_owned(context)&&
    (uintptr_t)original==_lw(_lw((uintptr_t)context+8)+12)&&
    zeemote_sony_quiesce_context(context))return 0;
 return original(op,buffer,cb);
}
#endif
