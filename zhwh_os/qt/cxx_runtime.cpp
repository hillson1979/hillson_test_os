/**
 * @file cxx_runtime.cpp
 * @brief Minimal C++ runtime for freestanding user-space programs
 *
 * Provides the bare minimum to compile and run C++ code without
 * libstdc++, exceptions, or RTTI.
 *
 * Put this file FIRST on the link line so _start lands at the right place.
 */

extern "C" {
#include "libuser_minimal.h"
}

// ============================================================
//  Simple static heap for new/delete (4 MB)
// ============================================================
#define CPP_HEAP_SIZE (1 * 1024 * 1024)
static char cpp_heap[CPP_HEAP_SIZE];
static char *heap_ptr = cpp_heap;

// ============================================================
//  __dso_handle — required for global destructor registration
// ============================================================
extern "C" {
    void *__dso_handle = nullptr;
}

// ============================================================
//  __cxa_atexit — register a function to run at exit
//  Our OS doesn't do shared-library unloading, so stub it.
// ============================================================
extern "C" int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso) {
    (void)destructor;
    (void)arg;
    (void)dso;
    return 0;  // success — we just ignore destructor registration
}

// ============================================================
//  __cxa_pure_virtual — called if a pure virtual is invoked
// ============================================================
extern "C" void __cxa_pure_virtual() {
    printf("[CXX] FATAL: pure virtual function called!\n");
    while (1) { __asm__ volatile("hlt"); }
}

// ============================================================
//  Exception handling stubs (we compile with -fno-exceptions,
//  but the compiler / linker may still reference these)
// ============================================================
extern "C" void __gxx_personality_v0() {
    // Stub — exceptions are disabled
}

extern "C" void _Unwind_Resume() {
    // Stub — exceptions are disabled
}

// ============================================================
//  Stack smashing protection stub
// ============================================================
extern "C" void __stack_chk_fail() {
    printf("[CXX] FATAL: stack smashing detected!\n");
    while (1) { __asm__ volatile("hlt"); }
}

// ============================================================
//  operator new / delete
//  Uses a simple bump allocator (no free, good enough for now)
// ============================================================
void *operator new(size_t size) {
    if (heap_ptr + size > cpp_heap + CPP_HEAP_SIZE) {
        printf("[CXX] FATAL: out of memory (new %u)!\n", (unsigned)size);
        while (1) { __asm__ volatile("hlt"); }
    }
    void *ptr = heap_ptr;
    heap_ptr += (size + 15) & ~15;  // 16-byte align
    return ptr;
}

void *operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void *ptr) {
    (void)ptr;
}

void operator delete[](void *ptr) {
    (void)ptr;
}

void operator delete(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
}

void operator delete[](void *ptr, size_t size) {
    (void)ptr;
    (void)size;
}

// ============================================================
//  __cxa_guard_* — guard variables for function-local statics
//  These are needed even without threads if you do:
//    static Foo f;
// ============================================================
extern "C" int __cxa_guard_acquire(long long *guard) {
    return (*guard == 0);
}

extern "C" void __cxa_guard_release(long long *guard) {
    *guard = 1;
}

extern "C" void __cxa_guard_abort(long long *guard) {
    *guard = 0;
}

// ============================================================
//  _start — program entry point (must be first in .text.start)
//  Use extern "C" to prevent name mangling
// ============================================================
extern "C" int main(void);

extern "C" __attribute__((section(".text.start")))
void _start() {
    // Call global constructors (stored in .init_array)
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    unsigned int count = __init_array_end - __init_array_start;
    for (unsigned int i = 0; i < count; i++) {
        __init_array_start[i]();
    }

    int ret = main();
    (void)ret;
    while (1) { __asm__ volatile("hlt"); }
}
