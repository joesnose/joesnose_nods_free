#!/usr/bin/env python3
"""Verify targeted completion wrappers and Allegrex S2-to-t1 forwarding bridge."""
import json,sys
from pathlib import Path
from check_hid_stack import functions
ns={};exec(Path(__file__).with_name('check_lifecycle_compile.py').read_text().split('for n in names:')[0],ns)
f=functions(sys.argv[1]);allowed={f[n][0] for n in ['completion_begin','completion_end']};frames={}
for n in ['completion_status_receive','completion_disconnect_receive']:
 a,w,frame=f[n];frames[n]=frame
 assert frame<=64 and ns['original_bounds'](a,w)==(1,1),(n,frame)
 assert all((((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in allowed for i,v in enumerate(w) if v>>26==3)
for n in ['completion_begin','completion_end']:
 a,w,frame=f[n];frames[n]=frame;assert frame<=32
 irq={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']}
 assert all((((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in irq for i,v in enumerate(w) if v>>26==3)
a,w,frame=f['completion_disconnect_bridge'];assert frame==0 and len(w)==4
# OR t1,s2,zero; OR t2,s4,zero; J C wrapper; NOP. RA preserved.
assert list(w)==[0x02404825,0x02805025,0x08000000|((f['completion_disconnect_receive'][0]>>2)&0x3ffffff),0]
assert f['logger_thread'][2]<=2048
assert 'event_wait_receive' not in f
print(json.dumps(dict(frames=frames,bridge_frame=frame,bridge_s2_to_sixth_arg=True,bridge_s4_to_seventh_arg=True,original_calls_per_path=[1,1],helpers_irq_only=True,logger_frame=f['logger_thread'][2],pass_check=True)))
