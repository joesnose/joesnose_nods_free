#!/usr/bin/env python3
"""Keep native HOME calls on the owned worker, away from HID report stacks."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1])
a,w,frame=f['home_thread'];assert frame<=1024
assert sum((x&0xfc1fffff)==0x0000f809 for x in w)==2 # status getter + native menu handler
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','sceKernelGetSystemTimeLow']}
for n in ['zee_input_home_take','zee_input_update_kind']:
 a,words,fr=f[n];assert fr<=64
 assert all((((a+i*4+4)&0xf0000000)|((x&0x3ffffff)<<2)) in allowed for i,x in enumerate(words) if x>>26==3)
print(json.dumps(dict(home_worker_frame=frame,home_worker_stack=4096,worker_native_indirect_calls=2,input_paths_irq_and_clock_only=True,pass_check=True)))
