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

    // 3. 测试 kmalloc 和 kfree
    printf("\n3. Testing kmalloc and kfree...\n");

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

    // 4. 测试内存统计
    printf("\n4. Memory Statistics:\n");
    uint32_t free_pages, used_pages, total_pages;
    buddy_stats(&free_pages, &used_pages, &total_pages);
    printf("   Total pages: %u\n", total_pages);
    printf("   Used pages: %u\n", used_pages);
    printf("   Free pages: %u\n", free_pages);

    printf("\n=== Memory Management System Test Complete ===\n");
}

void test_kernel_user_memory_isolation(void) {
    printf("\n=== Testing Kernel and User Memory Isolation ===\n");

    // 1. 显示初始状态
    printf("\n1. Initial memory state:\n");
    pmm_print_stats();

    // 2. 测试内核内存分配
    printf("\n2. Testing kernel memory allocation (should use reserved 128MB)...\n");
    uint32_t kernel_pages[5];
    for (int i = 0; i < 5; i++) {
        kernel_pages[i] = pmm_alloc_pages_type(4, MEM_ALLOC_KERNEL);  // 分配 4 页 (16KB)
        if (kernel_pages[i]) {
            printf("   Kernel allocation %d: 0x%x (page %u)\n",
                   i, kernel_pages[i], kernel_pages[i] / 4096);
        } else {
            printf("   Kernel allocation %d: FAILED\n", i);
        }
    }

    // 3. 测试用户空间内存分配
    printf("\n3. Testing user space memory allocation (should use memory after 128MB)...\n");
    uint32_t user_pages[5];
    for (int i = 0; i < 5; i++) {
        user_pages[i] = umem_alloc_pages(4);  // 分配 4 页 (16KB)
        if (user_pages[i]) {
            printf("   User allocation %d: 0x%x (page %u)\n",
                   i, user_pages[i], user_pages[i] / 4096);
        } else {
            printf("   User allocation %d: FAILED\n", i);
        }
    }

    // 4. 验证内存隔离
    printf("\n4. Verifying memory isolation...\n");
    uint32_t kernel_reserved_end_page = (0x1000000 / 4096) + 32768;  // base_page + 128MB

    bool isolation_ok = true;
    for (int i = 0; i < 5; i++) {
        if (kernel_pages[i]) {
            uint32_t page = kernel_pages[i] / 4096;
            if (page >= kernel_reserved_end_page) {
                printf("   ERROR: Kernel memory %d at page %u is outside reserved region!\n", i, page);
                isolation_ok = false;
            }
        }
        if (user_pages[i]) {
            uint32_t page = user_pages[i] / 4096;
            if (page < kernel_reserved_end_page) {
                printf("   ERROR: User memory %d at page %u is inside kernel reserved region!\n", i, page);
                isolation_ok = false;
            }
        }
    }

    if (isolation_ok) {
        printf("   Memory isolation verification PASSED!\n");
    } else {
        printf("   Memory isolation verification FAILED!\n");
    }

    // 5. 显示分配后状态
    printf("\n5. Memory state after allocations:\n");
    pmm_print_stats();

    // 6. 释放内存
    printf("\n6. Releasing allocated memory...\n");
    for (int i = 0; i < 5; i++) {
        if (kernel_pages[i]) {
            pmm_free_pages(kernel_pages[i], 4);
            printf("   Released kernel allocation %d\n", i);
        }
        if (user_pages[i]) {
            umem_free_pages(user_pages[i], 4);
            printf("   Released user allocation %d\n", i);
        }
    }

    // 7. 显示最终状态
    printf("\n7. Final memory state:\n");
    pmm_print_stats();

    printf("\n=== Kernel and User Memory Isolation Test Complete ===\n");
}
