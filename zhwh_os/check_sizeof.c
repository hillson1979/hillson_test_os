#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct trapframe {
  uint32_t edi;     // offset 0
  uint32_t esi;     // offset 4
  uint32_t ebp;     // offset 8
  uint32_t oesp;    // offset 12
  uint32_t ebx;     // offset 16
  uint32_t edx;     // offset 20
  uint32_t ecx;     // offset 24
  uint32_t eax;     // offset 28
  uint32_t ds;      // offset 32
  uint32_t es;      // offset 36
  uint32_t fs;      // offset 40
  uint32_t gs;      // offset 44
  uint32_t trapno;  // offset 48
  uint32_t err;     // offset 52
  uint32_t eip;     // offset 56
  uint32_t cs;      // offset 60
  uint32_t eflags;  // offset 64
  uint32_t esp;     // offset 68
  uint32_t ss;      // offset 72
} __attribute__((packed));

int main() {
    printf("sizeof(struct trapframe) = %d\n", sizeof(struct trapframe));
    printf("Expected: 76 bytes\n");
    
    // 验证偏移量
    printf("\nOffset verification:\n");
    printf("eip offset = %lu (expected 56)\n", __builtin_offsetof(struct trapframe, eip));
    printf("cs offset = %lu (expected 60)\n", __builtin_offsetof(struct trapframe, cs));
    printf("eflags offset = %lu (expected 64)\n", __builtin_offsetof(struct trapframe, eflags));
    printf("esp offset = %lu (expected 68)\n", __builtin_offsetof(struct trapframe, esp));
    printf("ss offset = %lu (expected 72)\n", __builtin_offsetof(struct trapframe, ss));
    
    return 0;
}
