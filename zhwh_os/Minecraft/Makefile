CC = gcc
CXX = g++
LD = g++
TARGET_ARCH = -m32
CFLAGS = $(TARGET_ARCH) -g -O0 -Wall -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pic
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit
LDFLAGS = $(TARGET_ARCH) -nostdlib -static -T ../test/ts_cpp.ld
INCLUDES = -Iinclude -I../opengl/include -I../user -I../include

all: mc.elf
mc.elf: main.o world.o mesh.o ../opengl/gl_context.o ../opengl/graph_user.o ../user/libuser_nostart.o ../qt/cxx_runtime.o ../qt/qobject.o ../qt/qpainter.o ../lvgl/string_compat.o ../lvgl/libgcc_compat.o
	$(LD) $(LDFLAGS) -o $@ $^
main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
world.o: src/world.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
mesh.o: src/mesh.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
../opengl/gl_context.o: ../opengl/src/gl_context.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
../opengl/graph_user.o: ../user/graph_user.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
../user/libuser_nostart.o: ../user/libuser.c
	$(CC) $(CFLAGS) -DLIBUSER_NO_START $(INCLUDES) -c $< -o $@
clean:
	rm -f *.o *.elf ../opengl/gl_context.o ../opengl/graph_user.o ../user/libuser_nostart.o
.PHONY: all clean
