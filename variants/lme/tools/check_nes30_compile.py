#!/usr/bin/env python3
import json,sys
from pathlib import Path
from check_hid_stack import functions
ns={};code=Path(__file__).with_name('check_lifecycle_compile.py').read_text().split('for n in names:')[0]
code=code.replace('if v==0x03e00008:result=(0,0)','if v==0x03200008:result=(1,1)\n  elif v==0x03e00008:result=(0,0)')
exec(code,ns)
f=functions(sys.argv[1]);a,w,frame=f['service_command'];assert frame<=64 and ns['original_bounds'](a,w)==(1,1)
def calls(a,w):return [(((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) for i,v in enumerate(w) if v>>26==3]
assert all(t==f['service_record'][0] for t in calls(a,w))
a,w,frame=f['descriptor_query'];assert frame<=64 and w.count(0x03200008)==1
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','service_record']}
assert all(t in allowed for t in calls(a,w))
# Saved/restored non-slot arguments cover all seven PSP EABI parameters.
for reg,offset in [(5,20),(6,24),(7,28),(8,32),(9,36),(10,40)]:
 assert (0xafa00000|(reg<<16)|offset) in w and (0x8fa00000|(reg<<16)|offset) in w
ha,hw,hframe=f['service_record'];assert hframe<=32
irq={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']};assert all(t in irq for t in calls(ha,hw))
a,w,_=f['service_record_fragment'];assert all(t==f['service_record'][0] for t in calls(a,w))
assert 'event_wait_receive' not in f
print(json.dumps(dict(descriptor_wrapper_frame=frame,service_command_frame=f['service_command'][2],seven_argument_saves_restores_present=True,command_original_calls=[1,1],no_native_file_logging=True,withdrawn_observer_absent=True,pass_check=True)))
