import struct

# Read ELF file
with open('zhwh_os/test/net_shell.elf', 'rb') as f:
    # ELF header (first 52 bytes minimum)
    e_ident = f.read(16)
    e_type = struct.unpack('<H', f.read(2))[0]
    e_machine = struct.unpack('<H', f.read(2))[0]
    e_version = struct.unpack('<I', f.read(4))[0]
    e_entry = struct.unpack('<I', f.read(4))[0]
    e_phoff = struct.unpack('<I', f.read(4))[0]
    e_shoff = struct.unpack('<I', f.read(4))[0]
    e_flags = struct.unpack('<I', f.read(4))[0]
    e_ehsize = struct.unpack('<H', f.read(2))[0]
    e_phentsize = struct.unpack('<H', f.read(2))[0]
    e_phnum = struct.unpack('<H', f.read(2))[0]

    print(f'ELF Entry point: 0x{e_entry:08x}')
    print(f'Program header offset: 0x{e_phoff:08x} ({e_phoff})')
    print(f'Program header entry size: {e_phentsize}')
    print(f'Program header count: {e_phnum}')

    # Read program headers
    f.seek(e_phoff)
    for i in range(e_phnum):
        p_type = struct.unpack('<I', f.read(4))[0]
        p_offset = struct.unpack('<I', f.read(4))[0]
        p_vaddr = struct.unpack('<I', f.read(4))[0]
        p_paddr = struct.unpack('<I', f.read(4))[0]
        p_filesz = struct.unpack('<I', f.read(4))[0]
        p_memsz = struct.unpack('<I', f.read(4))[0]
        p_align = struct.unpack('<I', f.read(4))[0]

        type_name = "LOAD" if p_type == 1 else f"0x{p_type:x}"
        print(f'\nProgram Header {i}:')
        print(f'  Type: {type_name}')
        print(f'  Virtual address: 0x{p_vaddr:08x}')
        print(f'  Physical address: 0x{p_paddr:08x}')
        print(f'  File size: 0x{p_filesz:x} ({p_filesz} bytes)')
        print(f'  Memory size: 0x{p_memsz:x} ({p_memsz} bytes)')
        print(f'  Offset in file: 0x{p_offset:08x}')
        print(f'  Alignment: 0x{p_align:x}')

        # Read first few bytes of this segment
        if p_type == 1 and p_filesz > 0:
            saved_pos = f.tell()
            f.seek(p_offset)
            first_bytes = f.read(16)
            print(f'  First bytes: {" ".join(f"{b:02x}" for b in first_bytes)}')

            # Check if entry point is in this segment
            if p_vaddr <= e_entry < p_vaddr + p_memsz:
                entry_offset = e_entry - p_vaddr
                f.seek(p_offset + entry_offset)
                entry_bytes = f.read(20)
                print(f'  *** Entry point (0x{e_entry:08x}) is in this segment')
                print(f'  Entry offset in segment: 0x{entry_offset:x}')
                print(f'  Entry code bytes: {" ".join(f"{b:02x}" for b in entry_bytes)}')

            f.seek(saved_pos)

