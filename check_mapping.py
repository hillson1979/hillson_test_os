#!/usr/bin/env python3
"""
Check page mapping for the ELF segment
"""

# From the output log:
# PT_LOAD: va=0x8000000, file_pa=0x424000, memsz=0x2CBD, filesz=0x2CBD

va = 0x8000000
memsz = 0x2cbd
entry = 0x08000024

print("ELF Segment Information:")
print(f"  Virtual Address: 0x{va:08x}")
print(f"  Memory Size: 0x{memsz:x} ({memsz} bytes)")
print(f"  Entry Point: 0x{entry:08x}")
print()

# Check which pages should be mapped
page_size = 0x1000
page0_start = 0x8000000
page0_end = 0x8001000 - 1
page1_start = 0x8001000
page1_end = 0x8002000 - 1

print(f"Page 0: 0x{page0_start:08x} - 0x{page0_end:08x}")
print(f"Page 1: 0x{page1_start:08x} - 0x{page1_end:08x}")
print()

print(f"Entry point 0x{entry:08x} is in:")
if page0_start <= entry <= page0_end:
    print(f"  [OK] Page 0 (0x{page0_start:08x} - 0x{page0_end:08x})")
    print(f"  ** PROBLEM: Page 0 is NOT mapped! **")
    print(f"  ** The loader only mapped from 0x{va:08x}, not from 0x{page0_start:08x} **")
elif page1_start <= entry <= page1_end:
    print(f"  [OK] Page 1 (0x{page1_start:08x} - 0x{page1_end:08x})")

print()
print("Segment end address: 0x{va + memsz:08x}")
print(f"Pages needed: {(memsz + page_size - 1) // page_size} pages")
print()

# The problem: va=0x8000000 is not page-aligned!
print("ROOT CAUSE:")
print("=" * 60)
print(f"Segment VA 0x{va:08x} is NOT page-aligned!")
print(f"Entry point 0x{entry:08x} needs page at 0x{page0_start:08x}")
print(f"But the segment starts at 0x{va:08x} (offset 0x{va % page_size:x} in page)")
print()
print("Expected behavior:")
print("  - Page 0 (0x8000000-0x8000fff) should be mapped")
print("  - But the code may have skipped it because the segment doesn't start at page boundary")
print()
print("From the log, the mapping loop seems to map pages starting from va,")
print("but if va=0x8000000, it should still map page 0!")
print()
print("Wait... let me check the actual page being mapped...")
