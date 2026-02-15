CC = gcc
AS = gcc
LD = gcc

TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
ASFLAGS = $(TARGET_ARCH) -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ../test/ts.ld

LVGL_DIR = source
LVGL_SRC_DIR = $(LVGL_DIR)/src
LVGL_FLAGS = -DLV_USE_EXTRA=0 -include libuser_minimal.h -include stdarg.h -include stdint_compat.h
LVGL_INCLUDES = -I. -I$(LVGL_DIR) -I$(LVGL_SRC_DIR) -I../user -I../include/lvgl -I../include

TARGET = lvgl_keyboard_test.elf

# Main test program objects
OBJS = lvgl_keyboard_test.o lvgl_port.o string_compat.o libgcc_compat.o lvgl_stubs.o ../user/libuser.o ../test/syscalls.o

# LVGL source files - ONLY software rendering, NO hardware acceleration!
LVGL_SRC = \
	source/src/misc/lv_anim.c \
	source/src/misc/lv_area.c \
	source/src/misc/lv_async.c \
	source/src/misc/lv_bidi.c \
	source/src/misc/lv_color.c \
	source/src/misc/lv_fs.c \
	source/src/misc/lv_gc.c \
	source/src/misc/lv_ll.c \
	source/src/misc/lv_log.c \
	source/src/misc/lv_lru.c \
	source/src/misc/lv_math.c \
	source/src/misc/lv_printf.c \
	source/src/misc/lv_style.c \
	source/src/misc/lv_style_gen.c \
	source/src/misc/lv_templ.c \
	source/src/misc/lv_timer.c \
	source/src/misc/lv_tlsf.c \
	source/src/misc/lv_txt.c \
	source/src/misc/lv_txt_ap.c \
	source/src/misc/lv_utils.c \
	source/src/misc/lv_mem.c \
	source/src/core/lv_disp.c \
	source/src/core/lv_event.c \
	source/src/core/lv_group.c \
	source/src/core/lv_indev.c \
	source/src/core/lv_indev_scroll.c \
	source/src/core/lv_obj.c \
	source/src/core/lv_obj_class.c \
	source/src/core/lv_obj_draw.c \
	source/src/core/lv_obj_pos.c \
	source/src/core/lv_obj_scroll.c \
	source/src/core/lv_obj_style.c \
	source/src/core/lv_obj_style_gen.c \
	source/src/core/lv_obj_tree.c \
	source/src/core/lv_refr.c \
	source/src/core/lv_theme.c \
	source/src/draw/lv_draw.c \
	source/src/draw/lv_draw_arc.c \
	source/src/draw/lv_draw_img.c \
	source/src/draw/lv_draw_label.c \
	source/src/draw/lv_draw_layer.c \
	source/src/draw/lv_draw_line.c \
	source/src/draw/lv_draw_mask.c \
	source/src/draw/lv_draw_rect.c \
	source/src/draw/lv_draw_triangle.c \
	source/src/draw/lv_draw_transform.c \
	source/src/draw/lv_img_buf.c \
	source/src/draw/lv_img_cache.c \
	source/src/draw/lv_img_decoder.c \
	source/src/draw/sw/lv_draw_sw.c \
	source/src/draw/sw/lv_draw_sw_arc.c \
	source/src/draw/sw/lv_draw_sw_blend.c \
	source/src/draw/sw/lv_draw_sw_dither.c \
	source/src/draw/sw/lv_draw_sw_gradient.c \
	source/src/draw/sw/lv_draw_sw_img.c \
	source/src/draw/sw/lv_draw_sw_layer.c \
	source/src/draw/sw/lv_draw_sw_letter.c \
	source/src/draw/sw/lv_draw_sw_line.c \
	source/src/draw/sw/lv_draw_sw_polygon.c \
	source/src/draw/sw/lv_draw_sw_rect.c \
	source/src/draw/sw/lv_draw_sw_transform.c \
	source/src/font/lv_font.c \
	source/src/font/lv_font_fmt_txt.c \
	source/src/font/lv_font_montserrat_14.c \
	source/src/hal/lv_hal_disp.c \
	source/src/hal/lv_hal_indev.c \
	source/src/hal/lv_hal_tick.c \
	source/src/widgets/lv_bar.c \
	source/src/widgets/lv_btn.c \
	source/src/widgets/lv_label.c \
	source/src/widgets/lv_slider.c

LVGL_OBJS = $(LVGL_SRC:.c=.o)

all: $(TARGET)

# Main test program
lvgl_keyboard_test.o: lvgl_keyboard_test.c
	@echo "Compiling lvgl_keyboard_test.c..."
	@$(CC) $(CFLAGS) $(LVGL_FLAGS) $(LVGL_INCLUDES) -c $< -o $@

# LVGL port
lvgl_port.o: lvgl_port.c
	@echo "Compiling lvgl_port.c..."
	@$(CC) $(CFLAGS) $(LVGL_FLAGS) $(LVGL_INCLUDES) -c $< -o $@

# Compat layers
string_compat.o: string_compat.c
	@echo "Compiling string_compat.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

libgcc_compat.o: libgcc_compat.c
	@echo "Compiling libgcc_compat.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

lvgl_stubs.o: lvgl_stubs.c
	@echo "Compiling lvgl_stubs.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

# Dependencies
../user/libuser.o:
	@echo "Compiling libuser.c..."
	@$(CC) $(CFLAGS) -I../user -c ../user/libuser.c -o ../user/libuser.o

../test/syscalls.o:
	@echo "Assembling syscalls.S..."
	@$(AS) $(ASFLAGS) -c ../test/syscalls.S -o ../test/syscalls.o

# LVGL source files
$(LVGL_OBJS): %.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(LVGL_FLAGS) $(LVGL_INCLUDES) -c $< -o $@

# Linking
$(TARGET): $(OBJS) $(LVGL_OBJS)
	@echo "Linking $@..."
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo "Built $@ successfully!"

clean:
	@echo "Cleaning..."
	@rm -f $(OBJS) $(TARGET)
	@find source/src -name "*.o" -type f -delete 2>/dev/null || true
	@echo "Cleaned!"

.PHONY: all clean
