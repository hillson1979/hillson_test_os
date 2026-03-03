// video_kmalloc_compat.c
// 🔥 用户态简单内存池（使用 BSS 中的静态数组）

// 🔥 策略：只为小对象分配堆，大对象使用独立的静态缓冲区
//
// 小对象堆：128KB（用于字符串、临时缓冲区等）
// 大对象（静态）：
//   - video_buf: 300KB (在 video_player.c 中)
//   - jpeg_buf: 200KB (在 video_player.c 中)
//   - LVGL buf: 400KB (在 lvgl_port.c 中)
//
// 总 BSS ≈ 128KB + 300KB + 200KB + 400KB ≈ 1MB
#define HEAP_SIZE (128 * 1024)  // 128KB 堆（只用于小对象）
static uint8_t video_heap[HEAP_SIZE];
static uint32_t heap_offset = 0;

void *kmalloc(unsigned int size)
{
    if (size == 0) {
        return NULL;
    }

    // 简单的对齐
    size = (size + 15) & ~15;

    if (heap_offset + size > HEAP_SIZE) {
        printf("[kmalloc] Out of memory! need=%u, offset=%u, max=%u\n",
               size, heap_offset, HEAP_SIZE);
        return NULL;  // 堆溢出
    }

    void *ptr = &video_heap[heap_offset];
    heap_offset += size;

    // 只在分配大块内存时打印
    if (size > 4096) {
        printf("[kmalloc] alloc=%u ptr=0x%x (offset now %u/%u)\n",
               size, (uint32_t)ptr, heap_offset, HEAP_SIZE);
    }

    return ptr;
}

void kfree(void *ptr)
{
    // 简单实现：不释放内存（防止碎片）
    (void)ptr;
}

void *krealloc(void *ptr, unsigned int size)
{
    void *new_ptr;

    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    new_ptr = kmalloc(size);
    if (new_ptr && ptr) {
        // 简单复制（不知道原始大小，可能有问题）
        extern void *memcpy(void *, const void *, unsigned int);
        memcpy(new_ptr, ptr, size);
    }

    return new_ptr;
}
