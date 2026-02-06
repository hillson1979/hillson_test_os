# Makefile for scheduler test programs

CC = gcc
AS = gcc
LD = gcc

TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
ASFLAGS = $(TARGET_ARCH) -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ts.ld

TARGETS = sched_test5.elf shell_demo.elf fs_test.elf net_test.elf net_shell.elf

# 测试程序源文件
SCHED5_SOURCE = sched_test5.c
SHELL_SOURCE = simple_interactive_shell.c
SYSCALL_TEST_SOURCE = simple_syscall_test.c
FS_TEST_SOURCE = fs_test.c
NET_TEST_SOURCE = net_test.c
NET_SHELL_SOURCE = net_shell.c

# 目标文件
SCHED5_OBJ = $(SCHED5_SOURCE:.c=.o)
SHELL_OBJ = shell_demo.o
FS_TEST_OBJ = $(FS_TEST_SOURCE:.c=.o)

SYSCALL_TEST_OBJ = simple_syscall_test.o


SHELL_DEP = $(SHELL_SOURCE)

all: $(TARGETS)

libuser.o: ../user/libuser.c
	$(CC) $(CFLAGS) -I../user -c ../user/libuser.c -o libuser.o
	@echo "Compiled: libuser.c"

sched_test5.elf: $(SCHED5_OBJ) syscalls.o libuser.o ts.ld
	$(LD) $(LDFLAGS) -o $@ $(SCHED5_OBJ) syscalls.o libuser.o
	@echo "Built: $@"

shell_demo.o: $(SHELL_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(SHELL_SOURCE) -o shell_demo.o
	@echo "Compiled: $(SHELL_SOURCE)"

shell_demo.elf: shell_demo.o syscalls.o libuser.o ts.ld
	$(LD) $(LDFLAGS) -o $@ shell_demo.o syscalls.o libuser.o
	@echo "Built: $@"



simple_syscall_test.o: $(SYSCALL_TEST_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(SYSCALL_TEST_SOURCE) -o simple_syscall_test.o
	@echo "Compiled: $(SYSCALL_TEST_SOURCE)"

syscalls.o: syscalls.S
	$(AS) $(ASFLAGS) -c syscalls.S -o syscalls.o
	@echo "Assembled: syscalls.S"

simple_syscall_test.elf: simple_syscall_test.o syscalls.o
	$(LD) $(LDFLAGS) -o $@ simple_syscall_test.o syscalls.o
	@echo "Built: $@"

# fs_test 编译规则
fs_test.o: $(FS_TEST_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(FS_TEST_SOURCE) -o fs_test.o
	@echo "Compiled: $(FS_TEST_SOURCE)"

fs_test.elf: fs_test.o syscalls.o libuser.o ts.ld
	$(LD) $(LDFLAGS) -o $@ fs_test.o syscalls.o libuser.o
	@echo "Built: $@"

# net_test 编译规则
net_test.o: $(NET_TEST_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(NET_TEST_SOURCE) -o net_test.o
	@echo "Compiled: $(NET_TEST_SOURCE)"

net_test.elf: net_test.o syscalls.o libuser.o ts.ld
	$(LD) $(LDFLAGS) -o $@ net_test.o syscalls.o libuser.o
	@echo "Built: $@"

# net_shell 编译规则
net_shell.o: $(NET_SHELL_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -I../include -c $(NET_SHELL_SOURCE) -o net_shell.o
	@echo "Compiled: $(NET_SHELL_SOURCE)"

# WiFi 固件对象文件
../driver/net/wifi/intel/iwlwifi_6000g2a_fw.o:
	$(CC) $(CFLAGS) -I../include -c ../driver/net/wifi/intel/iwlwifi_6000g2a_fw.c -o ../driver/net/wifi/intel/iwlwifi_6000g2a_fw.o
	@echo "Compiled: iwlwifi_6000g2a_fw.c"

net_shell.elf: net_shell.o syscalls.o libuser.o ../driver/net/wifi/intel/iwlwifi_6000g2a_fw.o ts.ld
	$(LD) $(LDFLAGS) -o $@ net_shell.o syscalls.o libuser.o ../driver/net/wifi/intel/iwlwifi_6000g2a_fw.o
	@echo "Built: $@ (with embedded 677KB firmware)"

%.o: %.c
	$(CC) $(CFLAGS) -I../user -I../include -c $< -o $@
	@echo "Compiled: $<"

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

clean:
	rm -f *.o $(TARGETS) libuser.o *~ ../driver/net/wifi/intel/iwlwifi_6000g2a_fw.o
	@echo "Cleaned"

.PHONY: all clean
