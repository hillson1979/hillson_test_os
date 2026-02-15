# 编译器设置
CC = gcc
AS = gcc
LD = gcc  # 使用gcc进行链接
OBJCOPY = objcopy

# 目标架构
TARGET_ARCH = -m32

# 编译选项
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-mmx -mno-3dnow -mno-red-zone \
         -mno-avx -mno-pclmul -mno-aes \
         -fno-math-errno -fno-rounding-math -fno-signaling-nans \
         -mgeneral-regs-only  # 强制只使用通用寄存器，禁用 XMM/YMM 寄存器
ASFLAGS = $(TARGET_ARCH)
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T linker/os.ld -Wl,-Map=kernel.map

# 包含目录
INCLUDES = -I./include

# 源文件
C_SOURCES = kernel.c printf.c vga.c pci.c kmalloc_early.c string.c highmem_mapping.c hardware_highmem.c madt_parser.c lapic.c ioapic.c page.c acpi.c mp.c segment.c interrupt.c mm.c task.c sched.c llist.c signal.c userboot.c syscall.c multiboot2.c pci_msi.c msi_test.c
C_SOURCES += driver/keyboard.c  # 添加键盘驱动
C_SOURCES += driver/vbe.c  # 添加 VBE 驱动
C_SOURCES += driver/uart.c  # 添加串口驱动
C_SOURCES += driver/netdebug.c  # 添加以太网调试接口
C_SOURCES += driver/usb_hcd.c  # 添加 USB 主机控制器驱动
C_SOURCES += driver/usb.c  # 添加 USB 核心协议栈
C_SOURCES += driver/usb_mouse.c  # 添加 USB 鼠标驱动
C_SOURCES += mm/buddy.c  # 启用 Buddy System（减小了 MAX_BUDDY_BLOCKS）
C_SOURCES += mm/test_memory.c  # 启用内存测试
C_SOURCES += fs/ramfs.c  # 添加 ramfs 文件系统
C_SOURCES += fs/vfs.c  # 添加 VFS 层
C_SOURCES += net/core.c  # 添加网络核心
C_SOURCES += net/loopback.c  # 添加回环设备
C_SOURCES += net/rtl8139.c  # 添加 RTL8139 网卡驱动
C_SOURCES += net/e1000.c  # 添加 E1000 网卡驱动
C_SOURCES += net/wifi/wifi.c  # 添加 WiFi 主模块
C_SOURCES += net/wifi/reg.c  # 添加寄存器操作
C_SOURCES += net/wifi/hw.c  # 添加硬件初始化
C_SOURCES += net/wifi/intel.c  # 添加 Intel WiFi 硬件初始化
C_SOURCES += net/wifi/intel_dma.c  # 添加 Intel DMA 实现
C_SOURCES += net/wifi/intel_fw.c  # 添加 Intel 固件通信
C_SOURCES += net/wifi/intel_fw_parser.c  # 添加 Intel 固件解析器
C_SOURCES += net/wifi/dma.c  # 添加 DMA 管理
C_SOURCES += net/wifi/firmware.c  # 添加固件加载
C_SOURCES += net/wifi/ieee80211.c  # 添加 802.11 帧处理
# Intel WiFi 固件（小型占位固件，真实固件由用户态加载）
C_SOURCES += net/wifi/firmware/intel/iwlwifi-6000g2a.c
# 真实固件文件放在用户空间，不在内核编译时包含
# Atheros WiFi 固件
C_SOURCES += net/wifi/firmware/atheros/ath10k_qca9377.c
# C_SOURCES += net/wifi/firmware/atheros/fw-5.c  # 暂时禁用大容量固件 (783KB)
# C_SOURCES += mm/slab.c  # 暂时禁用 Slab 分配器
# vbe_thunk.s 暂时禁用 - 实模式切换太复杂
ASM_SOURCES = boot.s vectors.s task_impl.s interrupt_exit.s trap_entry.s # vbe_thunk.s #copy_user_32.s

# 目标文件
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# 最终目标
TARGET = kernel.bin
RAW_TARGET = kernel.raw

# Offset 计算工具
OFFSETcalc = calc_task_offset

# 默认目标
all: $(TARGET) $(RAW_TARGET)

# 计算 task_t 结构体偏移量
calc-offset: calc_task_offset.c
	@echo "Compiling offset calculator..."
	$(CC) $(TARGET_ARCH) -o $(OFFSETcalc) calc_task_offset.c
	@echo ""
	./$(OFFSETcalc)
	@echo ""
	@echo "====================="
	@echo "Copy the TASK_IFRAME value above to task_impl.s line 18:"
	@echo "  .set TASK_IFRAME, <value>"
	@echo "====================="

# 使用gcc链接（推荐）
$(TARGET): $(OBJECTS) linker/os.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "Linked: $@"

# 生成原始二进制文件
$(RAW_TARGET): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(RAW_TARGET)
	@echo "Generated raw binary: $@"

# 明确排除大容量固件文件不被编译到内核
net/wifi/firmware/intel/iwlwifi-6000g2a-6-real.o:
	@echo "SKIPPED: $< (large firmware, userspace only)"

net/wifi/firmware/atheros/fw-5.o:
	@echo "SKIPPED: $< (large firmware, disabled)"

# 编译C文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiled: $<"

# 编译汇编文件
%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

clean:
	rm -f $(OBJECTS) $(TARGET) $(RAW_TARGET) *.o *~ *.map *.iso $(OFFSETcalc)
	@echo "Cleaned object files and binaries"

help:
	@echo "可用目标:"
	@echo "  all         - 编译所有目标 (默认)"
	@echo "  kernel.bin  - 编译ELF内核文件"
	@echo "  kernel.raw  - 生成原始二进制文件"
	@echo "  calc-offset - 计算 task_t 结构体偏移量"
	@echo "  clean       - 清理对象文件"
	@echo "  help        - 显示此帮助信息"

.PHONY: all clean help calc-offset

