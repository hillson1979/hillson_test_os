# ========================================================
# Text Editor Makefile
# zhwh_os - 轻量级文本编辑器（Sublime 风格）
# ========================================================

CC = gcc
AS = gcc
LD = gcc

TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-builtin-memcpy -fno-builtin-memset -fno-stack-protector -fno-pic -fno-threadsafe-statics -mno-tls-direct-seg-refs
ASFLAGS = $(TARGET_ARCH) -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ../test/ts.ld

LVGL_DIR = ../lvgl/source
LVGL_SRC_DIR = $(LVGL_DIR)/src
LVGL_FLAGS = -DLV_USE_EXTRA=0 -DSTB_NO_STDIO -include libuser_minimal.h -include stdarg.h -include stdint_compat.h
LVGL_INCLUDES = -I. -I$(LVGL_DIR) -I$(LVGL_SRC_DIR) -I../user -I../include/lvgl -I../include

TARGET = text_editor.elf

# 主程序对象
OBJS = text_editor.o string_compat.o libgcc_compat.o lvgl_stubs.o ../test/syscalls.o ../user/libuser.o

# LVGL 源文件 - 仅软件渲染
LVGL_SRC = \
	$(LVGL_SRC_DIR)/misc/lv_anim.c \
	$(LVGL_SRC_DIR)/misc/lv_area.c \
	$(LVGL_SRC_DIR)/misc/lv_async.c \
	$(LVGL_SRC_DIR)/misc/lv_bidi.c \
	$(LVGL_SRC_DIR)/misc/lv_color.c \
	$(LVGL_SRC_DIR)/misc/lv_fs.c \
	$(LVGL_SRC_DIR)/misc/lv_gc.c \
	$(LVGL_SRC_DIR)/misc/lv_ll.c \
	$(LVGL_SRC_DIR)/misc/lv_log.c \
	$(LVGL_SRC_DIR)/misc/lv_lru.c \
	$(LVGL_SRC_DIR)/misc/lv_math.c \
	$(LVGL_SRC_DIR)/misc/lv_printf.c \
	$(LVGL_SRC_DIR)/misc/lv_style.c \
	$(LVGL_SRC_DIR)/misc/lv_style_gen.c \
	$(LVGL_SRC_DIR)/misc/lv_templ.c \
	$(LVGL_SRC_DIR)/misc/lv_timer.c \
	$(LVGL_SRC_DIR)/misc/lv_tlsf.c \
	$(LVGL_SRC_DIR)/misc/lv_txt.c \
	$(LVGL_SRC_DIR)/misc/lv_txt_ap.c \
	$(LVGL_SRC_DIR)/misc/lv_utils.c \
	$(LVGL_SRC_DIR)/misc/lv_mem.c \
	$(LVGL_SRC_DIR)/core/lv_disp.c \
	$(LVGL_SRC_DIR)/core/lv_event.c \
	$(LVGL_SRC_DIR)/core/lv_group.c \
	$(LVGL_SRC_DIR)/core/lv_indev.c \
	$(LVGL_SRC_DIR)/core/lv_indev_scroll.c \
	$(LVGL_SRC_DIR)/core/lv_obj.c \
	$(LVGL_SRC_DIR)/core/lv_obj_class.c \
	$(LVGL_SRC_DIR)/core/lv_obj_draw.c \
	$(LVGL_SRC_DIR)/core/lv_obj_pos.c \
	$(LVGL_SRC_DIR)/core/lv_obj_scroll.c \
	$(LVGL_SRC_DIR)/core/lv_obj_style.c \
	$(LVGL_SRC_DIR)/core/lv_obj_style_gen.c \
	$(LVGL_SRC_DIR)/core/lv_obj_tree.c \
	$(LVGL_SRC_DIR)/core/lv_refr.c \
	$(LVGL_SRC_DIR)/core/lv_theme.c \
	$(LVGL_SRC_DIR)/draw/lv_draw.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_arc.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_img.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_label.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_layer.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_line.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_mask.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_rect.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_triangle.c \
	$(LVGL_SRC_DIR)/draw/lv_draw_transform.c \
	$(LVGL_SRC_DIR)/draw/lv_img_buf.c \
	$(LVGL_SRC_DIR)/draw/lv_img_cache.c \
	$(LVGL_SRC_DIR)/draw/lv_img_decoder.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_arc.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_blend.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_dither.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_gradient.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_img.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_layer.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_letter.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_line.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_polygon.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_rect.c \
	$(LVGL_SRC_DIR)/draw/sw/lv_draw_sw_transform.c \
	$(LVGL_SRC_DIR)/font/lv_font.c \
	$(LVGL_SRC_DIR)/font/lv_font_fmt_txt.c \
	$(LVGL_SRC_DIR)/font/lv_font_montserrat_14.c \
	$(LVGL_SRC_DIR)/hal/lv_hal_disp.c \
	$(LVGL_SRC_DIR)/hal/lv_hal_indev.c \
	$(LVGL_SRC_DIR)/hal/lv_hal_tick.c \
	$(LVGL_SRC_DIR)/widgets/lv_bar.c \
	$(LVGL_SRC_DIR)/widgets/lv_btn.c \
	$(LVGL_SRC_DIR)/widgets/lv_btnmatrix.c \
	$(LVGL_SRC_DIR)/widgets/lv_label.c \
	$(LVGL_SRC_DIR)/widgets/lv_list.c \
	$(LVGL_SRC_DIR)/widgets/lv_slider.c \
	$(LVGL_SRC_DIR)/widgets/lv_textarea.c \
	$(LVGL_SRC_DIR)/widgets/lv_img.c \
	$(LVGL_SRC_DIR)/widgets/lv_objx_templ.c

LVGL_OBJS = $(LVGL_SRC:.c=.o)

all: $(TARGET)

# 主程序
text_editor.o: text_editor.c
	@echo "Compiling text_editor.c..."
	@$(CC) $(CFLAGS) $(LVGL_FLAGS) $(LVGL_INCLUDES) -c $< -o $@

# LVGL 兼容文件
string_compat.o: ../lvgl/string_compat.c
	@echo "Compiling string_compat.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

libgcc_compat.o: ../lvgl/libgcc_compat.c
	@echo "Compiling libgcc_compat.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

lvgl_stubs.o: ../lvgl/lvgl_stubs.c
	@echo "Compiling lvgl_stubs.c..."
	@$(CC) $(CFLAGS) $(LVGL_INCLUDES) -c $< -o $@

# 依赖
../user/libuser.o: ../user/libuser.c ../user/libuser.h
	@echo "Compiling libuser.c..."
	$(CC) $(CFLAGS) -I../user -c ../user/libuser.c -o ../user/libuser.o

../test/syscalls.o:
	@echo "Assembling syscalls.S..."
	@$(AS) $(ASFLAGS) -c ../test/syscalls.S -o ../test/syscalls.o

# LVGL 源文件
$(LVGL_OBJS): %.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(LVGL_FLAGS) $(LVGL_INCLUDES) -c $< -o $@

# 链接
$(TARGET): text_editor.o $(OBJS) $(LVGL_OBJS)
	@echo "Linking $@..."
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo "Built $@ successfully!"

clean:
	@echo "Cleaning..."
	@rm -f text_editor.o string_compat.o libgcc_compat.o lvgl_stubs.o $(TARGET)
	@rm -f ../user/libuser.o
	@find $(LVGL_SRC_DIR) -name "*.o" -type f -delete 2>/dev/null || true
	@echo "Cleaned!"

.PHONY: all clean
