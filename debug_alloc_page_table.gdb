# GDB调试脚本 - 追踪alloc_page_table执行

# 连接到QEMU
target remote :1234

# 设置架构
set architecture i386

# 在关键点设置断点
break alloc_early_page_table
commands
  printf "=== alloc_early_page_table called ===\n"
  printf "  early_pt_alloc_addr=0x%x\n", early_pt_alloc_addr
  continue
end

# 在alloc_page_table函数设置断点
break alloc_page_table
commands
  printf "=== alloc_page_table called ===\n"
  printf "  virt_addr=0x%x\n", virt_addr
  printf "  phys_addr=0x%x\n", phys_addr
  printf "  pd_index=%u\n", (virt_addr >> 22) & 0x3FF
  printf "  pd[pd_index]=0x%x\n", pd[(virt_addr >> 22) & 0x3FF]
  continue
end

# 在phys_to_virt设置断点
break phys_to_virt
commands
  printf "=== phys_to_virt called ===\n"
  printf "  phys=0x%x\n", phys_addr
  continue
end

# 在页目录项写入后设置断点
break page.c:202
commands
  printf "=== After PDE write ===\n"
  printf "  pd[%u]=0x%x\n", pd_index, pd[pd_index]
  printf "  pt_virt=0x%x\n", pt_virt
  continue
end

# 在页表访问前设置断点
break page.c:207
commands
  printf "=== Before PT access ===\n"
  printf "  pd[%u]=0x%x\n", pd_index, pd[pd_index]
  printf "  pt_ calculated = 0x%x\n", pd[pd_index] & ~0xFFF
  continue
end

# 在Triple Fault或重启前捕获
break *0xC0108000
commands
  printf "=== Possible crash point ===\n"
  backtrace
  continue
end

# 继续执行
continue
