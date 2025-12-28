# 编译器设置
CC = gcc
AS = gcc
LD = gcc  # 使用gcc进行链接
OBJCOPY = objcopy

# 目标架构
TARGET_ARCH = -m32

# 编译选项
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector
ASFLAGS = $(TARGET_ARCH)
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T linker/os.ld -Wl,-Map=kernel.map

# 包含目录
INCLUDES = -I./include

# 源文件
C_SOURCES = kernel.c printf.c vga.c pci.c kmalloc_early.c string.c highmem_mapping.c hardware_highmem.c madt_parser.c lapic.c ioapic.c page.c acpi.c mp.c segment.c interrupt.c mm.c task.c sched.c llist.c signal.c userboot.c syscall.c
C_SOURCES += mm/buddy.c  # 启用 Buddy System（减小了 MAX_BUDDY_BLOCKS）
C_SOURCES += mm/test_memory.c  # 启用内存测试
# C_SOURCES += mm/slab.c  # 暂时禁用 Slab 分配器
ASM_SOURCES = boot.s vectors.s task_impl.s interrupt_exit.s trap_entry.s #copy_user_32.s

# 目标文件
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# 最终目标
TARGET = kernel.bin
RAW_TARGET = kernel.raw

# 默认目标
all: $(TARGET) $(RAW_TARGET)

# 使用gcc链接（推荐）
$(TARGET): $(OBJECTS) linker/os.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "Linked: $@"

# 生成原始二进制文件
$(RAW_TARGET): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(RAW_TARGET)
	@echo "Generated raw binary: $@"

# 编译C文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiled: $<"

# 编译汇编文件
%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

clean:
	rm -f $(OBJECTS) $(TARGET) $(RAW_TARGET) *.o *~ *.map *.iso
	@echo "Cleaned object files and binaries"

help:
	@echo "可用目标:"
	@echo "  all       - 编译所有目标 (默认)"
	@echo "  kernel.bin - 编译ELF内核文件"
	@echo "  kernel.raw - 生成原始二进制文件"
	@echo "  clean     - 清理对象文件"
	@echo "  help      - 显示此帮助信息"

.PHONY: all clean help

