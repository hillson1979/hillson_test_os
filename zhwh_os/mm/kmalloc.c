#include "kmalloc.h"
#include "mm/slab.h"
#include "mm/buddy.h"
#include "printf.h"
#include "string.h"
#include "page.h"

// 用于小型对象的 slab 缓存
static slab_cache_t* small_obj_cache[10] = {NULL};
static uint32_t small_obj_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

// 初始化 kmalloc 系统
int kmalloc_init(void) {
    int i;
    
    // 创建各种大小的 slab 缓存
    for (i = 0; i < sizeof(small_obj_sizes)/sizeof(small_obj_sizes[0]); i++) {
        small_obj_cache[i] = slab_cache_create(
            "small_obj",
            small_obj_sizes[i],
            sizeof(void*),
            0,
            NULL, NULL);
        
        if (!small_obj_cache[i]) {
            printf("Failed to create slab cache for size %u\n", small_obj_sizes[i]);
            return -1;
        }
    }
    
    printf("kmalloc_init: initialized\n");
    return 0;
}

// 分配内存
void *kmalloc(unsigned sz) {
    int i;
    void* ptr = NULL;
    
    if (sz == 0) {
        return NULL;
    }
    
    // 找到合适大小的 slab 缓存
    for (i = 0; i < sizeof(small_obj_sizes)/sizeof(small_obj_sizes[0]); i++) {
        if (sz <= small_obj_sizes[i]) {
            ptr = slab_alloc(small_obj_cache[i]);
            break;
        }
    }
    
    // 如果大小超过了最大的 slab 缓存，使用 buddy system 直接分配
    if (!ptr) {
        uint32_t pages = (sz + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t order = pages_to_order(pages);
        uint32_t page = buddy_alloc(order);
        
        if (page) {
            ptr = phys_to_virt(page * PAGE_SIZE);
        }
    }
    
    return ptr;
}

// 释放内存
void kfree(void *p) {
    if (!p) {
        return;
    }
    
    // 检查是否是小对象（通过页面大小判断）
    uint32_t virt = (uint32_t)p;
    uint32_t page_start = virt & ~(PAGE_SIZE - 1);
    
    // 检查是否是 slab 分配的对象
    // slab 分配的对象所在页面的第一个结构是 slab_t
    slab_t* slab = (slab_t*)page_start;
    
    // 检查 slab 结构的有效性
    if (slab->next != NULL || slab->prev != NULL || slab->free_objects != NULL) {
        // 是 slab 分配的对象，尝试找到对应的缓存
        for (int i = 0; i < sizeof(small_obj_sizes)/sizeof(small_obj_sizes[0]); i++) {
            if (p >= slab->start && 
                p < slab->start + slab->total_count * small_obj_sizes[i]) {
                // 释放到对应的 slab 缓存
                slab_free(small_obj_cache[i], p);
                return;
            }
        }
    }
    
    // 大对象，释放到 buddy system
    uint32_t page = virt_to_phys(p) / PAGE_SIZE;
    buddy_free(page, 0);
}

// kalloc 是 kmalloc 的别名
void *kalloc() {
    return kmalloc(PAGE_SIZE);
}
