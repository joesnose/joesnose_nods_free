#!/usr/bin/env python3
"""No 64-bit clock/division or file I/O in report-update/read-merge hot paths."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1]);allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','sceKernelGetSystemTimeLow']}
frames={}
for name in ['zee_input_update_kind','merge']:
 a,w,frame=f[name];frames[name]=frame;assert frame<=64
 calls=[((a+i*4+4)&0xf0000000)|((x&0x3ffffff)<<2) for i,x in enumerate(w) if x>>26==3]
 assert all(t in allowed for t in calls),(name,calls)
 assert f['sceKernelGetSystemTimeLow'][0] in calls
assert f['zee_input_timing'][2]<=32
print(json.dumps(dict(hot_path_frames=frames,low32_clock=True,no_division_or_file_calls_in_hot_paths=True,pass_check=True)))
