# GDB调试脚本 - 4GB内存配置

# 连接到QEMU
target remote :1234

# 设置架构
set architecture i386

# 关键断点
# 在 alloc_page_table 函数设置断点
break alloc_page_table
commands
  printf "=== alloc_page_table called ===\n"
  printf "virt_addr=0x%x\n", virt_addr
  printf "phys_addr=0x%x\n", phys_addr
  printf "pd_index=%u\n", (virt_addr >> 22) & 0x3FF
  printf "pd[pd_index]=0x%x\n", pd[(virt_addr >> 22) & 0x3FF]
  continue
end

# 在 lapicinit 函数设置断点
break lapicinit
commands
  printf "=== lapicinit called ===\n"
  printf "lapic (before)=0x%x\n", lapic
  continue
end

# 在 map_highmem_physical 函数设置断点（针对LAPIC）
break map_highmem_physical
commands
  printf "=== map_highmem_physical called ===\n"
  printf "phys_addr=0x%x\n", phys_addr
  printf "size=%u\n", size
  continue
end

# 在页错误处理函数设置断点
break handle_page_fault
commands
  printf "=== Page Fault ===\n"
  printf "cr2=0x%x\n", $eax  # 假设cr2在eax中
  printf "eip=0x%x\n", $eip
  backtrace
  continue
end

# 继续执行
continue
