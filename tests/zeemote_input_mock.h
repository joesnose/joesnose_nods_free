#include <stdint.h>
unsigned sceKernelGetSystemTimeLow(void);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int);
uintptr_t sctrlHENFindFunction(const char *,const char *,uint32_t);
typedef struct {uint32_t text_addr,text_size;} SceModule;
SceModule *sceKernelFindModuleByName(const char *);
unsigned sceKernelDevkitVersion(void);
uint32_t zee_test_code_address(const void *);
uint32_t _lw(uint32_t);
void _sw(uint32_t,uint32_t);

unsigned sceKernelInitKeyConfig(void);

uint32_t sctrlFindImportByNID(SceModule *,char *,unsigned);
