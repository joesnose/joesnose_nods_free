#!/usr/bin/env python3
"""Power keep-awake belongs to the resident worker, never a HID callback."""
import json,sys
from pathlib import Path
from check_hid_stack import functions
f=functions(sys.argv[1]);target=f['scePowerTick'][0];callers=[]
for name,(base,words,frame) in f.items():
 for i,w in enumerate(words):
  if w>>26==3 and (((base+i*4+4)&0xf0000000)|((w&0x3ffffff)<<2))==target:callers.append(name)
assert callers==['logger_thread'],callers
assert f['zee_input_session_live'][2]<=64
assert b'scePower_driver\0' in Path(sys.argv[1]).read_bytes()
print(json.dumps({'power_tick_worker_only':True,'power_tick_callers':callers,'live_check_frame':f['zee_input_session_live'][2],'kernel_power_driver_import':True,'hardware_test':False}))
