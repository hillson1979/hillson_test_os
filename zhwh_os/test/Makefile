# Makefile for scheduler test programs

CC = gcc
AS = gcc
LD = gcc

TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
ASFLAGS = $(TARGET_ARCH) -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ts.ld

TARGETS = simple_printf_test.elf


SCHED5_SOURCE = sched_test5.c
SHELL_SOURCE = simple_interactive_shell.c
SIMPLE_PRINTF_TEST_SOURCE = simple_printf_test.c
MINIMAL_TEST_SOURCE = minimal_test.c


SCHED5_OBJ = $(SCHED5_SOURCE:.c=.o)
SHELL_OBJ = shell_demo.o
SIMPLE_PRINTF_TEST_OBJ = simple_printf_test.o
MINIMAL_TEST_OBJ = minimal_test.o


SHELL_DEP = $(SHELL_SOURCE)

all: $(TARGETS)

sched_test5.elf: $(SCHED5_OBJ) ts.ld
	$(LD) $(LDFLAGS) -o $@ $(SCHED5_OBJ) ../user/libuser.c
	@echo "Built: $@"

shell_demo.o: $(SHELL_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(SHELL_SOURCE) -o shell_demo.o
	@echo "Compiled: $(SHELL_SOURCE)"

shell_demo.elf: shell_demo.o ts.ld
	$(LD) $(LDFLAGS) -o $@ shell_demo.o ../user/libuser.c
	@echo "Built: $@"

simple_printf_test.o: $(SIMPLE_PRINTF_TEST_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(SIMPLE_PRINTF_TEST_SOURCE) -o simple_printf_test.o
	@echo "Compiled: $(SIMPLE_PRINTF_TEST_SOURCE)"

simple_printf_test.elf: simple_printf_test.o ts.ld
	$(LD) $(LDFLAGS) -o $@ simple_printf_test.o ../user/libuser.c
	@echo "Built: $@"

minimal_test.o: $(MINIMAL_TEST_SOURCE) ts.ld
	$(CC) $(CFLAGS) -I../user -c $(MINIMAL_TEST_SOURCE) -o minimal_test.o
	@echo "Compiled: $(MINIMAL_TEST_SOURCE)"

minimal_test.elf: minimal_test.o ts.ld
	$(LD) $(LDFLAGS) -o $@ minimal_test.o ../user/libuser.c
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -I../user -c $< -o $@
	@echo "Compiled: $<"

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

clean:
	rm -f *.o $(TARGETS) *~
	@echo "Cleaned"

.PHONY: all clean
