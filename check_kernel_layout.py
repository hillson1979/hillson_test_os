#!/usr/bin/env python3
"""
检查内核虚拟地址范围，确保所有段都在页表映射范围内
"""

import sys
import struct

def parse_elf(filename):
    """解析 ELF 文件，获取段信息"""
    try:
        with open(filename, 'rb') as f:
            # 读取 ELF header
            ident = f.read(16)

            if ident[:4] != b'\x7fELF':
                print(f"错误：{filename} 不是 ELF 文件")
                return None

            # 检查是 32-bit 还是 64-bit
            is_64bit = ident[4] == 2

            # 读取 program header 偏移和数量
            if is_64bit:
                f.seek(0x20)  # e_phoff for 64-bit
                phoff = struct.unpack('<Q', f.read(8))[0]
                f.seek(0x36)  # e_phnum for 64-bit
                phnum = struct.unpack('<H', f.read(2))[0]
            else:
                f.seek(0x1C)  # e_phoff for 32-bit
                phoff = struct.unpack('<I', f.read(4))[0]
                f.seek(0x2C)  # e_phnum for 32-bit
                phnum = struct.unpack('<H', f.read(2))[0]

            print(f"ELF: {filename}")
            print(f"架构: {'64-bit' if is_64bit else '32-bit'}")
            print(f"Program Headers: {phnum} 个，偏移: 0x{phoff:X}")
            print()

            # 读取所有 program headers
            f.seek(phoff)

            # 32-bit program header 大小是 32 字节
            ph_size = 56 if is_64bit else 32

            sections = []
            max_vaddr = 0

            for i in range(phnum):
                ph_data = f.read(ph_size)

                if is_64bit:
                    p_type, p_flags, p_offset, p_vaddr, p_paddr, \
                    p_filesz, p_memsz, p_align = struct.unpack('<IIQQQQQQ', ph_data)
                else:
                    p_type, p_offset, p_vaddr, p_paddr, \
                    p_filesz, p_memsz, p_flags = struct.unpack('<IIIIIIII', ph_data)

                if p_type == 1:  # PT_LOAD
                    section_end = p_vaddr + p_memsz
                    if section_end > max_vaddr:
                        max_vaddr = section_end

                    sections.append({
                        'vaddr': p_vaddr,
                        'vend': section_end,
                        'filesz': p_filesz,
                        'memsz': p_memsz,
                        'flags': p_flags
                    })

            # 打印段信息
            print("已加载的段 (PT_LOAD):")
            print("-" * 80)
            for sec in sections:
                print(f"  虚拟地址: 0x{sec['vaddr']:08X} - 0x{sec['vend']:08X}")
                print(f"  文件大小: {sec['filesz']:6d} 字节, 内存大小: {sec['memsz']:6d} 字节")
                print()

            print("-" * 80)
            print(f"最高虚拟地址: 0x{max_vaddr:08X} ({max_vaddr / 1024 / 1024:.2f} MB)")
            print()

            # 检查是否在映射范围内
            MAPPING_START = 0xC0000000
            MAPPING_END = 0xC0000000 + 16 * 1024 * 1024  # 16MB

            if max_vaddr > MAPPING_END:
                print(f"⚠️  警告：内核超出 16MB 映射范围！")
                print(f"   最高地址: 0x{max_vaddr:08X}")
                print(f"   映射范围: 0x{MAPPING_START:08X} - 0x{MAPPING_END:08X}")
                print(f"   超出: {max_vaddr - MAPPING_END} 字节")
                print()
                print("建议：")
                required_mb = (max_vaddr - MAPPING_START) / 1024 / 1024
                print(f"  1. 扩展页表映射到至少 {required_mb:.0f} MB")
                print(f"  2. 添加更多页表（pt5, pt6, ...）")
                return False
            else:
                print(f"✅ 内核在 16MB 映射范围内")
                return True

    except FileNotFoundError:
        print(f"错误：找不到文件 {filename}")
        return None
    except Exception as e:
        print(f"错误：{e}")
        return None

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("用法: python3 check_kernel_layout.py <kernel.bin>")
        sys.exit(1)

    kernel_file = sys.argv[1]
    result = parse_elf(kernel_file)

    if result == False:
        sys.exit(1)  # 超出映射范围
    elif result == True:
        sys.exit(0)  # 正常
    else:
        sys.exit(2)  # 错误
