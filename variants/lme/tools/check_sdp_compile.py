#!/usr/bin/env python3
import sys,json
from pathlib import Path
from check_hid_stack import functions
ns={};exec(Path(__file__).with_name('check_lifecycle_compile.py').read_text().split('for n in names:')[0],ns)
f=functions(sys.argv[1]);a,w,frame=f['sdp_receive'];assert frame<=64 and ns['original_bounds'](a,w)==(1,1)
allowed={f[n][0] for n in ['sdp_begin','sdp_end','service_record_fragment']}
assert all((((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in allowed for i,v in enumerate(w) if v>>26==3)
for helper in ['sdp_begin','sdp_end']:
 ha,hw,hframe=f[helper];assert hframe<=32
 irq={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']}
 assert all((((ha+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in irq for i,v in enumerate(hw) if v>>26==3)
assert 'event_wait_receive' not in f
print(json.dumps(dict(wrapper_frame=frame,original_calls_per_path=[1,1],metadata_helpers_irq_only=True,withdrawn_entry_observer_absent=True,pass_check=True)))
