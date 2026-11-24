#!/usr/bin/env python3
"""No storage/parser/heap calls from the configurable report mapper."""
import json,sys
from check_hid_stack import functions
f=functions(sys.argv[1]);a,w,frame=f['sony_controls_decode']
assert frame<=128
calls=[((a+i*4+4)&0xf0000000)|((x&0x3ffffff)<<2) for i,x in enumerate(w) if x>>26==3]
assert calls==[f['ds4_basic_decode'][0]]
assert not any((x&0xfc1fffff)==0x0000f809 for x in w)
assert f['configure_sony_controls'][2]<=512 and f['sony_controls_parse'][2]<=768
print(json.dumps(dict(mapper_frame=frame,mapper_calls_basic_validation_only=True,config_loader_frame=f['configure_sony_controls'][2],parser_frame=f['sony_controls_parse'][2],no_file_or_parse_calls_on_report_stack=True,pass_check=True)))
