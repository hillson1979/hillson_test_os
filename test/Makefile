# 编译器设置
CC = gcc
AS = gcc
LD = gcc

# 目标架构
TARGET_ARCH = -m32

# 编译选项
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
ASFLAGS = $(TARGET_ARCH) -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ts.ld

# 目标文件
TARGET = shell_demo.elf

# 源文件
SOURCE = simple_shell.c
ASM_SOURCE = syscall_entry.S
OBJECT = $(SOURCE:.c=.o) $(ASM_SOURCE:.S=.o)

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJECT) ts.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECT)
	@echo "Built: $@"

# 编译C文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

# 编译汇编文件
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

clean:
	rm -f $(OBJECT) $(TARGET) *~
	@echo "Cleaned"

.PHONY: all clean
