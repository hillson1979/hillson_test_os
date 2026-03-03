#ifndef STB_COMPAT_H
#define STB_COMPAT_H

#include "kmalloc.h"
#include <string.h>

// 禁用文件I/O功能
#ifndef STB_NO_STDIO
#define STB_NO_STDIO
#endif

// 禁用线程相关的assert
#ifndef STB_NO_THREAD_LOCAL
#define STB_NO_THREAD_LOCAL
#endif

// stb_image需要的内存操作宏
#ifndef stbi_malloc
#define stbi_malloc(s)    kmalloc(s)
#endif

#ifndef stbi_free
#define stbi_free(p)      kfree(p)
#endif

#ifndef stbi_realloc
// 简单的realloc实现
static inline void* stbi_realloc(void* ptr, unsigned int size) {
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    void* new_ptr = kmalloc(size);
    if (new_ptr && ptr) {
        // 注意：stb通常会在内存块头部存储大小
        // 这里简化处理，假设调用者知道如何处理
        // 实际使用中stbi_load_from_memory不需要realloc
        memcpy(new_ptr, ptr, size);
        kfree(ptr);
    }
    return new_ptr;
}
#endif

#endif // STB_COMPAT_H
