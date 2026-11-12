#!/usr/bin/env python3
"""Compiled local-metadata wrappers: bounded stacks and callback side effects."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1])
allowed={f[n][0] for n in ['ds4_is_active','service_record','sceKernelCpuSuspendIntr','sceKernelCpuResumeIntr']}
names=['local_pnp_query','local_attribute_query','local_read_start','ds4_context_authorized','ds4_read_trace','ds4_is_active']
frames={}
for n in names:
 a,w,frame=f[n];frames[n]=frame
 assert frame<=64
 calls=[((a+i*4+4)&0xf0000000)|((x&0x3ffffff)<<2) for i,x in enumerate(w) if x>>26==3]
 assert all(c in allowed for c in calls),(n,calls)
# Conservatively charge both observers and authorization on top of the old
# report path allowance, even though read observation returns before dispatch.
budget=392+frames['ds4_read_trace']+frames['ds4_context_authorized']+frames['ds4_is_active']
assert budget<=768
print(json.dumps(dict(frames=frames,direct_calls_irq_or_metadata_only=True,conservative_report_budget=budget,report_thread_bytes=1024,pass_check=True)))
