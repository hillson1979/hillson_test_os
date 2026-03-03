#include <stdio.h>
#include <stdint.h>
#include <include/interrupt.h>

int main() {
    printf("offsetof(eax) = %lu\n", __builtin_offsetof(struct trapframe, eax));
    printf("offsetof(ecx) = %lu\n", __builtin_offsetof(struct trapframe, ecx));
    printf("offsetof(edx) = %lu\n", __builtin_offsetof(struct trapframe, edx));
    printf("offsetof(ebx) = %lu\n", __builtin_offsetof(struct trapframe, ebx));
    printf("sizeof(struct trapframe) = %d\n", sizeof(struct trapframe));
    return 0;
}
