/**
 * @file usb_kmalloc.c — USB专用伙伴系统分配器
 * 其他模块不受影响，继续用 kmalloc_early
 */
#include <string.h>
#include "kernel/hillsonos.h"
#include "page.h"
#include "mm.h"

/* 从伙伴系统分配 (used by USB via hillsonos.h redefine) */
void *usb_kmalloc(unsigned int size) {
    if (size == 0) return NULL;
    if (size <= 2048) {
        /* 小对象用kmalloc_early (不改其他模块) */
        extern void *kmalloc_early(unsigned int);
        return kmalloc_early(size);
    }
    /* 大对象走伙伴系统 */
    uint32_t pages = (size + 4095) / 4096;
    uint32_t phys = pmm_alloc_pages(pages);
    if (!phys) return NULL;
    void *v = phys_to_virt(phys);
    memset(v, 0, pages * 4096);
    return v;
}

void usb_kfree(void *ptr) {
    if (!ptr) return;
    extern void kfree(void *);
    kfree(ptr);
}
