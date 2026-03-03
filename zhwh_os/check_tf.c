#include <stdio.h>
#include <stdint.h>
#include <include/interrupt.h>

int main() {
    printf('sizeof(struct trapframe) = %d\n', sizeof(struct trapframe));
    printf('offsetof(ds) = %d\n', __builtin_offsetof(struct trapframe, ds));
    printf('offsetof(es) = %d\n', __builtin_offsetof(struct trapframe, es));
    printf('offsetof(fs) = %d\n', __builtin_offsetof(struct trapframe, fs));
    printf('offsetof(gs) = %d\n', __builtin_offsetof(struct trapframe, gs));
    printf('offsetof(eax) = %d\n', __builtin_offsetof(struct trapframe, eax));
    printf('offsetof(ecx) = %d\n', __builtin_offsetof(struct trapframe, ecx));
    printf('offsetof(edx) = %d\n', __builtin_offsetof(struct trapframe, edx));
    printf('offsetof(ebx) = %d\n', __builtin_offsetof(struct trapframe, ebx));
    printf('offsetof(oesp) = %d\n', __builtin_offsetof(struct trapframe, oesp));
    printf('offsetof(ebp) = %d\n', __builtin_offsetof(struct trapframe, ebp));
    printf('offsetof(esi) = %d\n', __builtin_offsetof(struct trapframe, esi));
    printf('offsetof(edi) = %d\n', __builtin_offsetof(struct trapframe, edi));
    printf('offsetof(eip) = %d\n', __builtin_offsetof(struct trapframe, eip));
    return 0;
}
