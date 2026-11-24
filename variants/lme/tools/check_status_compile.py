#!/usr/bin/env python3
"""Bounded status wrapper checks; native indirect stack is not fully proven."""
import sys,json
from pathlib import Path
from check_hid_stack import functions
ns={};exec(Path(__file__).with_name('check_lifecycle_compile.py').read_text().split('for n in names:')[0],ns)
f=functions(sys.argv[1]);a,w,frame=f['status_receive'];bounds=ns['original_bounds'](a,w)
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','sceKernelGetThreadId','status_flight_begin','status_flight_end']}
direct=[(((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) for i,v in enumerate(w) if v>>26==3]
assert bounds==(1,1) and frame<=64 and all(x in allowed for x in direct)
for helper in ['status_flight_begin','status_flight_end']:
 ha,hw,hframe=f[helper];assert hframe<=32
 irq={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']}
 assert all((((ha+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in irq for i,v in enumerate(hw) if v>>26==3)
print(json.dumps(dict(wrapper_frame=frame,original_calls_per_path=bounds,no_file_logging=True,pass_check=True)))
