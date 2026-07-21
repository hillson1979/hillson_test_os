# JVM for HillsonOS - Build Configuration
#
# Usage:
#   make            — 编译 jvm.elf
#   make test       — 编译测试 Java 程序
#   make clean      — 清理构建产物

CC = gcc
LD = gcc

TARGET_ARCH = -m32

# 与内核构建保持一致
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall \
         -ffreestanding -nostdlib -nostdinc \
         -fno-builtin -fno-stack-protector \
         -fno-pic \
         -I../include \
         -I../user \
         -Iport \
         -Ijvm \
         -Iclasspath

ASFLAGS = $(TARGET_ARCH) -fno-pic

LDFLAGS = $(TARGET_ARCH) -nostdlib -static \
          -T ts_jvm.ld

# --- 源文件 ---

# OS 移植层
PORT_SRC = port/os_port.c

# JVM 核心
JVM_SRC = jvm/jvm.c \
          jvm/classfile.c \
          jvm/interpreter.c \
          jvm/gc.c \
          jvm/native.c

# Classpath 基础类库（本地方法实现）
CLASSPATH_SRC = classpath/java_lang.c \
                classpath/java_io.c \
                classpath/java_net.c

# 所有源文件 (含 syscalls.S 汇编)
ALL_SRC = $(PORT_SRC) $(JVM_SRC) $(CLASSPATH_SRC) main.c
ALL_ASM = ../test/syscalls.S

# 对象文件
OBJS = $(ALL_SRC:.c=.o) $(notdir $(ALL_ASM:.S=.o))

# libuser + 系统调用桩
LIBUSER_OBJ  = ../user/libuser.o

# 额外依赖
EXTRA_OBJS = $(LIBUSER_OBJ)

# 目标
TARGET = jvm.elf

# --- 规则 ---

all: $(TARGET)
	@echo "=== JVM build complete: $(TARGET) ==="

$(TARGET): $(OBJS) $(EXTRA_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(EXTRA_OBJS)
	@echo "Linked: $@"

# vpath 搜索路径
vpath %.S ../test

# 编译 C 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

# 汇编 .S 文件
%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@
	@echo "Assembled: $<"

# 确保 libuser 已编译
$(LIBUSER_OBJ):
	$(MAKE) -C ../user libuser.o

test: $(TARGET)
	@echo "=== Compiling Java test programs ==="
	javac -d test/classes test/HelloWorld.java 2>/dev/null || \
		(echo "Note: javac not found; use pre-compiled or cross-compile Java classes" && \
		 echo "Place .class files in test/classes/")

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf test/classes
	@echo "Cleaned"

jamvm:
	$(MAKE) -C jamvm

clean-jamvm:
	$(MAKE) -C jamvm clean

.PHONY: all test clean jamvm clean-jamvm
