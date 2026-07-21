# JamVM for HillsonOS — Build Configuration
#
# Compiles JamVM 2.0 source with HillsonOS compatibility layer.
#
# Usage:
#   make            — build jamvm.elf
#   make clean      — clean build artifacts

# ================================================================
# 目录路径
# ================================================================
JAMVM_SRC   = ../jamvm_src/src
JAMVM_INTERP = $(JAMVM_SRC)/interp/engine
JAMVM_CLASSLIB = $(JAMVM_SRC)/classlib/openjdk
ZHWH_OS     = ../../..
PORT_DIR    = ../port
COMPAT_DIR  = compat
CLASSPATH_DIR = classpath
OS_DIR      = os/hillson

# ================================================================
# 编译器
# ================================================================
CC = gcc
LD = gcc
TARGET_ARCH = -m32

# ================================================================
# Include 路径
# 注意: COMPAT_DIR 必须在最前面，以拦截 <stdio.h> 等标准头文件
# ================================================================
INCLUDES = \
    -I. \
    -I$(COMPAT_DIR) \
    -I$(JAMVM_SRC) \
    -I$(JAMVM_INTERP) \
    -I$(CLASSPATH_DIR) \
    -I$(PORT_DIR) \
    -I$(ZHWH_OS)/include \
    -I$(OS_DIR)/i386

# ================================================================
# 编译标志
# ================================================================
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall \
         -ffreestanding -nostdlib -nostdinc \
         -fno-builtin -fno-stack-protector \
         -fno-pic -fno-rtti -fno-exceptions \
         -DHAVE_CONFIG_H \
         -DJAMVM_VERSION=\"2.0.1-hillson\" \
         -DDEFAULT_MIN_HEAP=4194304 \
         -DDEFAULT_MAX_HEAP=67108864 \
         $(INCLUDES)

ASFLAGS = $(TARGET_ARCH) -fno-pic

LDFLAGS = $(TARGET_ARCH) -nostdlib -static \
          -T $(ZHWH_OS)/user/ts.ld

# ================================================================
# 源文件
# ================================================================

# --- 兼容层 ---
COMPAT_SRC = $(COMPAT_DIR)/compat.c

# --- OS 移植层 ---
PORT_SRC = $(PORT_DIR)/os_port.c
OS_SRC = $(OS_DIR)/os.c \
         $(OS_DIR)/i386/init.c \
         $(OS_DIR)/i386/dll_md.c

# --- JamVM 核心源文件 ---
JAMVM_CORE_SRC = \
    $(JAMVM_SRC)/alloc.c \
    $(JAMVM_SRC)/cast.c \
    $(JAMVM_SRC)/class.c \
    $(JAMVM_SRC)/dll.c \
    $(JAMVM_SRC)/excep.c \
    $(JAMVM_SRC)/execute.c \
    $(JAMVM_SRC)/frame.c \
    $(JAMVM_SRC)/hash.c \
    $(JAMVM_SRC)/hooks.c \
    $(JAMVM_SRC)/init.c \
    $(JAMVM_SRC)/jni.c \
    $(JAMVM_SRC)/lock.c \
    $(JAMVM_SRC)/natives.c \
    $(JAMVM_SRC)/properties.c \
    $(JAMVM_SRC)/reflect.c \
    $(JAMVM_SRC)/resolve.c \
    $(JAMVM_SRC)/shutdown.c \
    $(JAMVM_SRC)/sig.c \
    $(JAMVM_SRC)/string.c \
    $(JAMVM_SRC)/stubs.c \
    $(JAMVM_SRC)/symbol.c \
    $(JAMVM_SRC)/thread.c \
    $(JAMVM_SRC)/time.c \
    $(JAMVM_SRC)/utf8.c \
    $(JAMVM_SRC)/zip.c \
    $(JAMVM_SRC)/access.c

# --- JamVM 解释器引擎 ---
JAMVM_INTERP_SRC = \
    $(JAMVM_INTERP)/interp.c \
    $(JAMVM_INTERP)/interp2.c \
    $(JAMVM_SRC)/interp/direct.c \
    $(JAMVM_SRC)/interp/inlining.c

# --- 入口点 ---
MAIN_SRC = main.c

# --- 所有目标文件 ---
ALL_SRC = $(COMPAT_SRC) $(PORT_SRC) $(OS_SRC) \
          $(JAMVM_CORE_SRC) $(JAMVM_INTERP_SRC) $(MAIN_SRC)

OBJS = $(notdir $(ALL_SRC:.c=.o))

# 外部依赖
LIBUSER_OBJ = $(ZHWH_OS)/user/libuser.o
SYSCALL_OBJ = $(ZHWH_OS)/user/syscall_entry.o
EXTRA_OBJS = $(LIBUSER_OBJ) $(SYSCALL_OBJ)

TARGET = jamvm.elf

# ================================================================
# vpath: 告诉 make 在哪里找源文件
# ================================================================
vpath %.c $(sort $(dir $(ALL_SRC)))

# ================================================================
# 规则
# ================================================================

all: $(TARGET)
	@echo "=== JamVM build complete: $(TARGET) ==="

$(TARGET): $(OBJS) $(EXTRA_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(EXTRA_OBJS)
	@echo "Linked: $@"

# 编译规则 — 每个 .c 对应一个 .o
# 使用 vpath 自动从对应目录找源文件
%.o: %.c config.h $(COMPAT_DIR)/compat.h
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC  $<"

# 手动指定需要特殊 include 路径的文件
interp.o: $(JAMVM_INTERP)/interp.c
	$(CC) $(CFLAGS) -I$(JAMVM_SRC)/interp -c $< -o $@

interp2.o: $(JAMVM_INTERP)/interp2.c
	$(CC) $(CFLAGS) -I$(JAMVM_SRC)/interp -c $< -o $@

direct.o: $(JAMVM_SRC)/interp/direct.c
	$(CC) $(CFLAGS) -I$(JAMVM_SRC)/interp -c $< -o $@

inlining.o: $(JAMVM_SRC)/interp/inlining.c
	$(CC) $(CFLAGS) -I$(JAMVM_SRC)/interp -c $< -o $@

# 外部依赖
$(LIBUSER_OBJ):
	$(MAKE) -C $(ZHWH_OS)/user libuser.o

$(SYSCALL_OBJ):
	$(MAKE) -C $(ZHWH_OS)/user syscall_entry.o

# 清理
clean:
	rm -f *.o $(TARGET)
	@echo "Cleaned"

.PHONY: all clean
