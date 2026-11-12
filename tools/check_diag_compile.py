#!/usr/bin/env python3
"""Release check: logging sink cannot call anything and has no file path."""
import json,sys
from pathlib import Path
from check_hid_stack import functions
f=functions(sys.argv[1]);a,w,frame=f['log_line']
assert frame<=64
assert not any(v>>26==3 or (v&0xfc1fffff)==0x0000f809 for v in w)
assert not any(n in f for n in ['diag_open','diag_write','diag_close','diag_enter','diag_leave','format_char','prnt'])
for artifact in [Path(sys.argv[1]),Path(sys.argv[1]).with_suffix('.prx')]:
 data=artifact.read_bytes();assert b'pspgo_hci_transport.log' not in data and b'.log\0' not in data
write=f['sceIoWrite'][0];writers=[]
for name,(base,words,_) in f.items():
 for i,v in enumerate(words):
  if v>>26==3 and (((base+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2))==write:writers.append(name)
assert len(writers)==1 and writers[0] in ['logger_thread','preferred_target_worker'],writers
assert f['logger_thread'][2]<=2048
print(json.dumps(dict(logging_disabled=True,sink_calls_none=True,formatter_absent=True,log_paths_absent=True,state_file_writer_only=writers,sink_frame=frame,pass_check=True)))
