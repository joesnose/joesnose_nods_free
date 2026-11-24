#!/usr/bin/env python3
"""Check the compiled observer ABI; not a complete native stack proof."""
import sys,json
from check_hid_stack import functions
f=functions(sys.argv[1]);address,words,frame=f['pair_receive_trace']
indirect=[i for i,w in enumerate(words) if w&0xfc1fffff==0x0000f809]
first=indirect[0] if indirect else -1

def writes_argument(w):
 op=w>>26;rt=(w>>16)&31;rd=(w>>11)&31;fn=w&63
 return ((op in list(range(8,16))+[32,33,35,36,37,39] and rt in range(4,10)) or
         (op==0 and rd in range(4,10) and fn in [0,2,3,4,6,7,16,18,32,33,34,35,36,37,38,39,42,43]))
args_untouched=first>=0 and not any(writes_argument(w) for w in words[:first+2])
allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']}
calls=[(((address+i*4+4)&0xf0000000)|((w&0x3ffffff)<<2)) for i,w in enumerate(words) if w>>26==3]
only_irq_calls=all(a in allowed for a in calls)
ok=frame<=64 and len(indirect)==1 and args_untouched and only_irq_calls
print(json.dumps(dict(frame=frame,indirect_calls=len(indirect),six_arguments_untouched_through_native_call=args_untouched,
                     observer_direct_calls_only_irq_helpers=only_irq_calls,pass_check=ok),indent=2))
raise SystemExit(0 if ok else 1)
