CC = gcc
LD = gcc
TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ../test/ts.ld
INCLUDES = -Iinclude -I../user -I../include

all: gl_test.elf

gl_test.elf: gl_test.o gl_context.o graph_user.o ../user/libuser_nostart.o ../lvgl/string_compat.o ../lvgl/libgcc_compat.o
	$(LD) $(LDFLAGS) -o $@ $^

gl_test.o: gl_test.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

gl_context.o: src/gl_context.c
	$(CC) $(CFLAGS) $(INCLUDES) -I../user -c $< -o $@

graph_user.o: ../user/graph_user.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

../user/libuser_nostart.o: ../user/libuser.c
	$(CC) $(CFLAGS) -DLIBUSER_NO_START -I../user -I../include -c $< -o $@

clean:
	rm -f *.o *.elf ../user/libuser_nostart.o

.PHONY: all clean
