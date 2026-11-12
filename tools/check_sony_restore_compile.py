#!/usr/bin/env python3
"""Check owned-worker recovery frame and emitted reboot callback bridge."""
import sys,json
from check_hid_stack import functions
f=functions(sys.argv[1]);frames={n:f[n][2] for n in ['sony_restore_tick','sony_restore_snapshot','transition_reboot_walk','transition_control_receive','transition_control_bridge','zeemote_sony_quiesce_context']}
assert frames['sony_restore_tick']<=768 and frames['sony_restore_snapshot']<=64
assert frames['zeemote_sony_quiesce_context']<=32
assert frames['transition_reboot_walk']<=32 and frames['transition_control_receive']<=64
assert list(f['transition_control_bridge'][1])==[0x24680004,0x08000000|((f['transition_control_receive'][0]>>2)&0x3ffffff),0]
assert f['transition_control_bridge'][2]==0
address=f['sony_restore_tick'][0]
callers=[]
for name,(a,words,frame) in f.items():
 for i,w in enumerate(words):
  if w>>26 in (2,3) and (((a+i*4+4)&0xf0000000)|((w&0x3ffffff)<<2))==address:callers.append(name)
assert callers and all(n in ['preferred_target_tick','logger_thread'] for n in callers),callers
assert f['logger_thread'][2]<=2048
print(json.dumps(dict(frames=frames,recovery_callers=callers,bridge_arguments_preserved=True,pass_check=True)))
