#!/usr/bin/env python3
"""Check known PSP HID callback frames in a compiled ELF (not a hardware test).

Sony's inspected report thread has 1024 bytes. Its known caller frames use
128 bytes. This check reserves another 128 bytes for native time/IRQ/memory
helpers, and requires at least 256 bytes of remaining headroom. It also rejects
a direct installer call from receive_report. Indirect calls require source
review; this is a conservative regression check, not a complete stack proof.
"""
import json
import struct
import sys
from pathlib import Path

def functions(path):
    data = Path(path).read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('expected little-endian ELF32')
    offset = struct.unpack_from('<I', data, 32)[0]
    stride, count = struct.unpack_from('<HH', data, 46)
    sections = [struct.unpack_from('<10I', data, offset + i * stride) for i in range(count)]
    out = {}
    for section in sections:
        if section[1] != 2:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(section[4], section[4] + section[5], section[9] or 16):
            name, value, size, info, _, index = struct.unpack_from('<IIIBBH', data, at)
            if info & 15 != 2 or not size or not 0 < index < len(sections):
                continue
            label = names[name:].split(b'\0', 1)[0].decode()
            code_section = sections[index]
            start = code_section[4] + value - code_section[3]
            words = struct.unpack_from('<' + 'I' * (size // 4), data, start)
            frames = [(-((w & 65535) - 65536)) for w in words
                      if w >> 16 == 0x27bd and w & 0x8000]
            out[label] = (value, words, max(frames, default=0))
    return out

def main(path):
    funcs = functions(path)
    needed = ['receive_report', 'zee_decode_report', 'zee_input_update_kind', 'zee_input_install', 'sony_controls_decode', 'ds4_basic_decode']
    missing = [name for name in needed if name not in funcs]
    if missing:
        raise ValueError('missing symbols: ' + ', '.join(missing))
    frames = {name: funcs[name][2] for name in needed}
    callback_address, words, _ = funcs['receive_report']
    installer = funcs['zee_input_install'][0]
    calls_installer = any(w >> 26 == 3 and
        ((((callback_address + i * 4 + 4) & 0xf0000000) | ((w & 0x3ffffff) << 2)) == installer)
        for i, w in enumerate(words))
    # Decode and update occur sequentially; summing both deliberately overcounts.
    minimum_path = 128 + frames['receive_report'] + (frames['zee_input_install'] if calls_installer else max(frames['zee_decode_report'], frames['zee_input_update_kind']))
    budget = 128 + 128 + frames['receive_report'] + max(frames['zee_decode_report'],frames['sony_controls_decode']+frames['ds4_basic_decode']) + frames['zee_input_update_kind'] + (frames['zee_input_install'] if calls_installer else 0)
    ok = not calls_installer and frames['receive_report'] <= 256 and budget <= 768
    print(json.dumps({'frames': frames, 'direct_installer_call': calls_installer,
                      'known_direct_path_minimum': minimum_path,
                      'conservative_known_budget_with_allowance': budget,
                      'report_thread_bytes': 1024, 'minimum_remaining_bytes': 1024 - budget,
                      'pass': ok}, indent=2))
    return 0 if ok else 1

if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1]))
