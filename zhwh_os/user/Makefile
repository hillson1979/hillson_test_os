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
TARGETS = shell_demo.elf gui.elf
# 源文件
SHELL_SOURCE = simple_interactive_shell.c
GUI_SOURCE = gui.c

ASM_SOURCE = syscall_entry.S

# 对象文件
SHELL_OBJ = $(SHELL_SOURCE:.c=.o) $(ASM_SOURCE:.S=.o)
GUI_OBJ = $(GUI_SOURCE:.c=.o) libuser.o

# 默认目标
all: $(TARGETS)

# 链接 Shell
shell_demo.elf: $(SHELL_OBJ) ts.ld
	$(LD) $(LDFLAGS) -o $@ $(SHELL_OBJ)
	@echo "Built: $@"

# 链接 GUI
gui.elf: $(GUI_OBJ) ts.ld
	$(LD) $(LDFLAGS) -o $@ $(GUI_OBJ)
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
	rm -f *.o $(TARGETS) *~
	@echo "Cleaned"

.PHONY: all clean
