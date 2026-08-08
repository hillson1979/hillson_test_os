# C++ Qt-style framework for zhwh_os
# Run from WSL: cd /mnt/f/hillson_test_os/zhwh_os/qt && make

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

USER_DIR = ../user
LVGL_DIR = ../lvgl
INCLUDE_DIR = ../include

INCLUDES = -I. -I./include -I$(USER_DIR) -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/lvgl

# Shared OS compatibility objects
LIB_OBJS = $(USER_DIR)/libuser_nostart.o \
           $(LVGL_DIR)/string_compat.o \
           $(LVGL_DIR)/libgcc_compat.o

# Qt framework core objects
QT_OBJS = cxx_runtime.o qobject.o qpainter.o qwidget.o

# Qt/Embedded 3.3.8b port objects
QT_EMBEDDED_OBJS = qscreenhillsonfb_qws.o qkbdhillson_qws.o qmousehillson_qws.o qapplication_qt.o

# ============================================================
#  Targets
# ============================================================
all: cpp_test.elf sigslot_test.elf gui_test.elf editor_test.elf fb_test.elf qt_embedded_test.elf

# --- Basic C++ runtime test ---
cpp_test.elf: cxx_runtime.o main.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- Signal/slot test ---
sigslot_test.elf: cxx_runtime.o qobject.o test_signal_slot.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- GUI test (draws on framebuffer!) ---
gui_test.elf: $(QT_OBJS) test_gui.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- Interactive text editor test ---
editor_test.elf: $(QT_OBJS) qtextdocument.o qtextedit.o test_editor.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- Raw framebuffer test (no Qt, just pixels) ---
fb_test.elf: cxx_runtime.o test_fb.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- Qt/Embedded 3.3.8b port test ---
qt_embedded_test.elf: $(QT_OBJS) $(QT_EMBEDDED_OBJS) test_qt_embedded.o $(LIB_OBJS)
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ $^

# --- Compile C++ ---
%.o: %.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# --- Compile C (OS libs) ---
$(USER_DIR)/libuser_nostart.o: $(USER_DIR)/libuser.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -DLIBUSER_NO_START -I$(USER_DIR) -I$(INCLUDE_DIR) -c $< -o $@

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
