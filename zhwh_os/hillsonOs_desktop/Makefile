# HillsonOS Desktop — Makefile
# Build: cd /mnt/f/hillson_test_os/zhwh_os/hillsonOs_desktop && make

CXX = g++
CC  = gcc
LD  = g++

TARGET_ARCH = -m32

CXXFLAGS = $(TARGET_ARCH) -g -O0 -Wall \
	-ffreestanding -nostdlib -nostdinc \
	-fno-builtin -fno-stack-protector \
	-fno-exceptions -fno-rtti \
	-fno-threadsafe-statics -fno-use-cxa-atexit \
	-fno-pic -mno-tls-direct-seg-refs \
	-mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 \
	-mno-mmx -mno-3dnow -mno-avx \
	-mgeneral-regs-only

CFLAGS = $(TARGET_ARCH) -g -O0 -Wall \
	-ffreestanding -nostdlib -nostdinc \
	-fno-builtin -fno-builtin-memcpy -fno-builtin-memset \
	-fno-stack-protector -fno-pic \
	-fno-threadsafe-statics -mno-tls-direct-seg-refs

LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ts_cpp.ld

QT_DIR   = ../qt
USER_DIR = ../user
LVGL_DIR = ../lvgl
INC_DIR  = ../include

INCLUDES = -I./include -I. -I$(QT_DIR) -I$(QT_DIR)/include -I$(USER_DIR) -I$(INC_DIR) -I$(INC_DIR)/lvgl

# Shared OS compatibility objects
LIB_OBJS = $(USER_DIR)/libuser.o \
           $(LVGL_DIR)/string_compat.o \
           $(LVGL_DIR)/libgcc_compat.o

# Qt framework objects (compiled in qt/ directory)
QT_OBJS = $(QT_DIR)/cxx_runtime.o \
          $(QT_DIR)/qobject.o \
          $(QT_DIR)/qpainter.o \
          $(QT_DIR)/qwidget.o \
          $(QT_DIR)/qtextdocument.o \
          $(QT_DIR)/qtextedit.o \
          $(QT_DIR)/qkbdhillson_qws.o

# Desktop own objects
DESKTOP_OBJS = qdesktop.o qdesktopwindow.o qtexteditorapp.o qusbmonitor.o qsysinfo.o qterminal.o desktop.o

# ============================================================
#  Target
# ============================================================
all: desktop.elf

desktop.elf: $(QT_OBJS) $(DESKTOP_OBJS) $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^
	@cp desktop.elf ../iso/boot/

# --- Compile C++ ---
%.o: %.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# --- Compile C (OS libs) ---
$(USER_DIR)/libuser.o: $(USER_DIR)/libuser.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -I$(USER_DIR) -I$(INC_DIR) -c $< -o $@

$(LVGL_DIR)/string_compat.o: $(LVGL_DIR)/string_compat.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LVGL_DIR)/libgcc_compat.o: $(LVGL_DIR)/libgcc_compat.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning..."
	@rm -f *.o *.elf

.PHONY: all clean
