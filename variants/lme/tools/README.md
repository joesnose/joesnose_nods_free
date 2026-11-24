# Build Validation Tools

The `check_*.py` scripts inspect a compiled PSP ELF for callback behavior, firmware ABI assumptions, stack limits and logging removal.

Build the plugin first, then run a check from the repository root. For example:

```sh
python tools/check_diag_compile.py joesnose_nods_free.elf
python tools/check_hid_stack.py joesnose_nods_free.elf
```

`check_hid_stack.py` also provides the ELF inspection helper used by other checks. These tools inspect compiled code; they do not establish hardware compatibility by themselves.
