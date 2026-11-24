#!/usr/bin/env python3
"""Bound owned registration-planning frames on the observed4KB loader worker."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1]);frames={n:f[n][2] for n in ['module_started','patch_registration','native_pin_hybrid_plan','log_line']}
assert frames['patch_registration']<=1600 and frames['native_pin_hybrid_plan']<=128
owned=frames['module_started']+frames['patch_registration']+max(frames['native_pin_hybrid_plan'],frames['log_line'])
assert owned<=2048 and owned+1536<=4096
print(json.dumps(dict(frames=frames,known_owned_path=owned,native_allowance=1536,loader_stack=4096,full_native_stack_unproven=True,pass_check=True)))
