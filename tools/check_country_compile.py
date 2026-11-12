#!/usr/bin/env python3
"""Guard DS4 country shim ABI/stack and absence of native-callback file I/O."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1]);a,w,frame=f['country_query']
assert frame<=64 and w.count(0x03200008)==1
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','service_record']}
assert all((((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) in allowed for i,v in enumerate(w) if v>>26==3)
# a0/a1/a2 stay in s0/s1/s2 through helpers and return to original registers.
for v in [0x00808025,0x00a08825,0x00c09025,0x02002025,0x02202825,0x02403025,0x00e01825,0x00603825]:assert v in w
# a3 and arguments5/6/7 survive both IRQ and metadata calls.
for reg,offsets in [(8,[16,20]),(9,[20,24]),(10,[24,28])]:
 for offset in offsets:assert (0xafa00000|(reg<<16)|offset) in w and (0x8fa00000|(reg<<16)|offset) in w
assert 0xafa7000c in w and 0x8fa3000c in w and 0xafa30010 in w and 0x8fa30010 in w
assert 'event_wait_receive' not in f
print(json.dumps(dict(country_wrapper_frame=frame,original_tail_once=True,seven_arguments_preserved=True,direct_calls_irq_or_metadata_only=True,pass_check=True)))
