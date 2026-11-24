#!/usr/bin/env python3
"""DS4 basic callback-stack/side-effect regression check, not hardware proof."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1])
a,w,frame=f['ds4_basic_decode']
assert frame<=32 and not any(v>>26==3 or ((v&0xfc1fffff)==0x0000f809) for v in w)
ca,cw,callback_frame=f['receive_report']
def targets(a,w):return [(((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) for i,v in enumerate(w) if v>>26==3]
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr','memcpy','zee_decode_report','sony_controls_decode','zee_input_update_kind','zee_input_clear']}
calls=targets(ca,cw);assert all(t in allowed for t in calls) and calls.count(f['sony_controls_decode'][0])==1
budget=128+128+callback_frame+frame+f['sony_controls_decode'][2]+f['zee_input_update_kind'][2]
assert callback_frame<=64 and budget<=768
print(json.dumps(dict(ds4_decoder_frame=frame,decoder_calls_none=True,report_callback_frame=callback_frame,callback_direct_calls_whitelisted=True,ds4_known_budget_with_native_allowance=budget,report_thread_bytes=1024,pass_check=True)))
