#!/usr/bin/env python3
"""
Disassemble x86 instructions from entry point
"""
import struct

# Entry point bytes from ELF
entry_bytes = bytes([
    0x55,              # push ebp
    0x89, 0xe5,        # mov ebp, esp
    0x83, 0xec, 0x18,  # sub esp, 0x18
    0xe8, 0xd2, 0x1f, 0x00, 0x00,  # call <rel32>  (0x1fd2)
    0x89, 0x45, 0xf4,  # mov [ebp-0xc], eax
    0x83, 0xec, 0x0c,  # sub esp, 0xc
    0xff, 0x75, 0xf4,  # push [ebp-0xc]
])

print("Entry point at 0x08000024:")
print("=" * 60)

offset = 0
i = 0
while i < len(entry_bytes):
    b = entry_bytes[i]
    print(f"0x{0x80000024 + offset:08x}:  ", end="")

    if b == 0x55:
        print(f"55                      push ebp")
        i += 1
    elif b == 0x89 and entry_bytes[i+1] == 0xe5:
        print(f"89 e5                   mov ebp, esp")
        i += 2
    elif b == 0x83 and entry_bytes[i+1] == 0xec:
        disp = entry_bytes[i+2]
        print(f"83 ec {disp:02x}             sub esp, 0x{disp:x}")
        i += 3
    elif b == 0xe8:
        # call rel32
        rel = struct.unpack('<i', entry_bytes[i+1:i+5])[0]
        target = 0x80000024 + offset + 5 + rel
        print(f"e8 {rel:08x}            call 0x{target:08x}")
        i += 5
    elif b == 0x89 and entry_bytes[i+1] == 0x45:
        disp = entry_bytes[i+2]
        print(f"89 45 {disp:02x}             mov dword [ebp-0x{256-disp:x}], eax")
        i += 3
    elif b == 0xff and entry_bytes[i+1] == 0x75:
        disp = entry_bytes[i+2]
        print(f"ff 75 {disp:02x}             push dword [ebp-0x{256-disp:x}]")
        i += 3
    else:
        print(f"{b:02x}                     ???")
        i += 1

    offset = (0x80000024 + i) - 0x80000024

print("\n" + "=" * 60)
print("\nSummary:")
print("- Function prologue: push ebp; mov ebp, esp; sub esp, 0x18")
print("- Call to initialization function")
print("- Stack access at [ebp-0xc]")
print("- **All accesses are within the stack frame**")
print("\n**Page fault at 0x80000027 means:")
print("  - Trying to fetch instruction at offset 0x3 from entry")
print("  - This is the 'sub esp, 0x18' instruction (3 bytes: 83 ec 18)")
print("  - **PROBLEM: The page containing 0x80000000-0x80000fff is not mapped!**")

import struct
