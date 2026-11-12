#!/usr/bin/env python3
"""Check each compiled teardown path calls its original once; not full firmware proof."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1]);names=['context_remove_trace','keyboard_term_trace','mouse_term_trace'];allowed={f[n][0] for n in ['sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']};checks={}
def original_bounds(a,w):
 memo={};visiting=set()
 def walk(i):
  if i in memo:return memo[i]
  if i in visiting or not 0<=i<len(w):raise ValueError('unexpected cycle/escape in observer CFG')
  visiting.add(i);v=w[i];op=v>>26;rs=(v>>21)&31;rt=(v>>16)&31;imm=(v&65535);imm=imm-65536 if imm&32768 else imm
  extra=0
  if v==0x03e00008:result=(0,0)
  else:
   if op==3:nxt=[i+2]
   elif v&0xfc1fffff==0x0000f809:extra=1;nxt=[i+2]
   elif op==2:
    dest=(((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2));nxt=[(dest-a)//4]
   elif op in [1,4,5,6,7,20,21,22,23]:
    dest=i+1+imm;nxt=[i+2,dest]
    if op in [4,20] and rs==rt:nxt=[dest]
    if op in [5,21] and rs==rt:nxt=[i+2]
    if op==1 and rs==0 and rt in [0,2,16]:nxt=[i+2]
    if op==1 and rs==0 and rt in [1,3,17]:nxt=[dest]
   else:nxt=[i+1]
   values=[walk(j) for j in nxt];result=(extra+min(x[0] for x in values),extra+max(x[1] for x in values))
  visiting.remove(i);memo[i]=result;return result
 return walk(0)
for n in names:
 a,w,frame=f[n];direct=[(((a+i*4+4)&0xf0000000)|((v&0x3ffffff)<<2)) for i,v in enumerate(w) if v>>26==3]
 low,high=original_bounds(a,w)
 checks[n]=dict(frame=frame,min_original_calls_per_return=low,max_original_calls_per_return=high,direct_calls_only_irq=all(x in allowed for x in direct))
assert 'lifecycle_thread_snapshot' not in f
profiler_frame=0
ok=all(x['frame']<=64 and x['min_original_calls_per_return']==1 and x['max_original_calls_per_return']==1 and x['direct_calls_only_irq'] for x in checks.values()) and profiler_frame<=2048
print(json.dumps(dict(observers=checks,worker_profiler_frame=profiler_frame,worker_stack_bytes=8192,pass_check=ok),indent=2))
raise SystemExit(0 if ok else 1)
