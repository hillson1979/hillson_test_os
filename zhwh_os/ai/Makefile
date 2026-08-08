# AI Inference Engine — IBM Granite 3.0 Nano
# Build: cd ai && make
# Output: infer.elf (loadable by OS)

CC = gcc
CFLAGS = -m32 -O2 -Wall -ffreestanding -nostdlib -nostdinc \
         -fno-builtin -fno-stack-protector \
         -fno-pic -fno-pie -mno-tls-direct-seg-refs \
         -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 \
         -mno-mmx -mno-3dnow -mno-avx -mfpmath=387

INC = -I../include -I./include -I.
SRC = infer.c ggml/gguf.c ggml/matmul.c transformer.c
OBJ = $(SRC:.c=.o)
TARGET = ai.elf
LIBGCC := $(shell $(CC) -m32 -print-libgcc-file-name)

all: $(TARGET)

OSLIB = libuser_ai.o ../lvgl/string_compat.o ../lvgl/libgcc_compat.o
$(TARGET): $(OBJ) $(OSLIB)
	$(CC) -m32 -nostdlib -static -no-pie -T ../user/ts.ld -Wl,-Map=ai.map -o $@ $(OBJ) $(OSLIB) $(LIBGCC)
	cp $@ ../iso/boot/

libuser_ai.o: ../user/libuser.c ../user/libuser.h
	$(CC) $(CFLAGS) -I../user -I../include -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ) libuser_ai.o $(TARGET) ai.map
