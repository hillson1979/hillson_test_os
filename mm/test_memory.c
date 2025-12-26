#include "types.h"
#include "mm.h"
#include "kmalloc.h"
#include "printf.h"
#include "mm/buddy.h"
#include "mm/slab.h"

void test_memory_management(void) {
    printf("\n=== Testing Memory Management System ===\n");
    
    // 1. 初始化内存管理系统
    printf("1. Initializing memory management system...\n");
    if (mm_init() != 0) {
        printf("   Memory management initialization failed!\n");
        return;
    }
    printf("   Memory management initialized successfully.\n");
    
    // 2. 测试 Buddy System
    printf("\n2. Testing Buddy System...\n");
    uint32_t page1 = buddy_alloc(2); // 分配 4 页 (16KB)
    uint32_t page2 = buddy_alloc(1); // 分配 2 页 (8KB)
    
    if (page1 && page2) {
        printf("   Allocated pages: page1=0x%x (order=2), page2=0x%x (order=1)\n", page1, page2);
        
        // 测试释放
        if (buddy_free(page1, 2) == 0 && buddy_free(page2, 1) == 0) {
            printf("   Pages released successfully.\n");
        } else {
            printf("   Page release failed!\n");
        }
    } else {
        printf("   Buddy System allocation failed!\n");
    }
    
    // 3. 测试 Slab Allocator
    printf("\n3. Testing Slab Allocator...\n");
    slab_cache_t* cache = slab_cache_create("test_cache", 32, 4, 0, NULL, NULL);
    if (cache) {
        printf("   Created slab cache: name=%s, object_size=%u, num_per_slab=%u\n", 
               cache->name, cache->object_size, cache->num_per_slab);
        
        // 分配多个对象
        void* obj1 = slab_alloc(cache);
        void* obj2 = slab_alloc(cache);
        void* obj3 = slab_alloc(cache);
        
        if (obj1 && obj2 && obj3) {
            printf("   Allocated 3 objects: obj1=0x%x, obj2=0x%x, obj3=0x%x\n", obj1, obj2, obj3);
            
            // 释放对象
            slab_free(cache, obj1);
            slab_free(cache, obj2);
            slab_free(cache, obj3);
            printf("   Objects released successfully.\n");
        } else {
            printf("   Slab allocation failed!\n");
        }
        
        // 销毁缓存
        slab_cache_destroy(cache);
        printf("   Slab cache destroyed.\n");
    } else {
        printf("   Slab cache creation failed!\n");
    }
    
    // 4. 测试 kmalloc 和 kfree
    printf("\n4. Testing kmalloc and kfree...\n");
    
    // 测试小对象分配
    void* small1 = kmalloc(16);
    void* small2 = kmalloc(128);
    
    // 测试大对象分配
    void* large = kmalloc(10000); // 约 10KB，应该使用 Buddy System
    
    if (small1 && small2 && large) {
        printf("   Allocated: small1=0x%x (16B), small2=0x%x (128B), large=0x%x (10KB)\n", 
               small1, small2, large);
        
        // 释放内存
        kfree(small1);
        kfree(small2);
        kfree(large);
        printf("   All memory released successfully.\n");
    } else {
        printf("   kmalloc failed!\n");
    }
    
    // 5. 测试内存统计
    printf("\n5. Memory Statistics:\n");
    uint32_t free_pages, used_pages, total_pages;
    buddy_stats(&free_pages, &used_pages, &total_pages);
    printf("   Total pages: %u\n", total_pages);
    printf("   Used pages: %u\n", used_pages);
    printf("   Free pages: %u\n", free_pages);
    
    printf("\n=== Memory Management System Test Complete ===\n");
}
