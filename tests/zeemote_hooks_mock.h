#ifndef ZEE_HOOKS_MOCK_H
#define ZEE_HOOKS_MOCK_H
#include <stdint.h>
typedef struct {char modname[32];uint32_t text_addr,text_size;} SceModule;
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int);
uint32_t _lw(uint32_t);
void _sw(uint32_t,uint32_t);
void sceKernelDcacheWritebackInvalidateAll(void);
void sceKernelIcacheInvalidateAll(void);
SceModule *sceKernelFindModuleByName(const char *);
#endif
