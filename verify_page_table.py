#!/usr/bin/env python3
"""
Verify page table indices for user virtual address 0x08000000
"""

# User virtual address that caused page fault
vaddr = 0x08000027

# x86 page table structure:
# - 10 bits for PD index (bits 31-22)
# - 10 bits for PT index (bits 21-12)
# - 12 bits for offset (bits 11-0)

pd_index = (vaddr >> 22) & 0x3FF
pt_index = (vaddr >> 12) & 0x3FF
offset = vaddr & 0xFFF

print(f"Virtual Address: 0x{vaddr:08x}")
print(f"  Page Directory Index: {pd_index} (0x{pd_index:03x})")
print(f"  Page Table Index: {pt_index} (0x{pt_index:03x})")
print(f"  Page Offset: {offset} (0x{offset:03x})")
print()

# Calculate the actual page base address
page_base = vaddr & ~0xFFF
print(f"Page Base Address: 0x{page_base:08x}")
print()

# CR3 from log
cr3 = 0x101000
print(f"CR3 (Page Directory Physical Address): 0x{cr3:08x}")
print()

# Page directory entry address
pd_entry_phys = cr3 + pd_index * 4
print(f"Page Directory Entry at: 0x{pd_entry_phys:08x} + 0x{pd_index * 4:x}")
print()

# Expected layout:
# pd[512] should point to a page table
# That page table's entry 0 should point to the physical page containing 0x08000000

print("Expected Page Table Structure:")
print(f"  pd[{pd_index}] -> Page Table Physical Address")
print(f"  Page Table[0] -> Physical Page containing 0x{page_base:08x}")
print()

print("CRITICAL CHECK:")
print(f"  The page directory index {pd_index} is in the USER SPACE range!")
print(f"  (Kernel space is 0xC0000000+, which would be PD index >= 768)")
print(f"  User space is 0x00000000+, which is PD index < 768")
print()

# Check if this PD index overlaps with kernel mappings
print("Potential Issue:")
print(f"  PD index {pd_index} might conflict with existing kernel mappings!")
print(f"  Need to check if pd[{pd_index}] is already used by kernel...")
print()

# Calculate virtual address range for this PD entry
va_start = pd_index << 22
va_end = va_start + (1 << 22) - 1
print(f"Virtual Address Range for pd[{pd_index}]:")
print(f"  0x{va_start:08x} - 0x{va_end:08x} (4 MB)")
print()

# Check if entry point falls in this range
entry = 0x08000024
if va_start <= entry <= va_end:
    print(f"  ✓ Entry point 0x{entry:08x} is in this range!")
    print(f"  So pd[{pd_index}] MUST be mapped correctly!")
else:
    print(f"  ✗ Entry point 0x{entry:08x} is NOT in this range!")
    print(f"  This would be a serious bug!")
