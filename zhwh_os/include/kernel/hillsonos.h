/**
 * @file hillsonos.h
 */
#ifndef KERNEL_HILLSONOS_H
#define KERNEL_HILLSONOS_H

#include <kernel.h>
#include <kernel/asm.h>
#include <kernel/errno.h>
#include <stdint.h>

/* USB files: printk -> usb_printk */
extern int usb_printk(const char *fmt, ...);
/* USB files use usb_printk() directly — no macro magic */

/* tick_delay — busy-wait for N microseconds (approximate) */
static inline void tick_delay(int us) {
    //volatile int i;
    for (int i = 0; i < us * 100; i++) {
        __asm__ volatile("pause" : : : "memory");
    }
}

/* Page table helpers */
#include <page.h>

/* get_phys_addr is in usb_hci_stubs.c */

/* Physical/virtual address types (used by HCI code) */
typedef uint32_t physical_addr;
typedef uint32_t virtual_addr;

/* mmio_map — map physical MMIO range to a usable kernel virtual address.
 * Returns virtual address matching phys_start offset within the mapped page. */
static inline void *mmio_map(physical_addr phys_start, physical_addr phys_end) {
    uint32_t sz = phys_end - phys_start;
    if (sz == 0) sz = 0x1000;

    /* If phys_start is below the top of physical RAM, use direct mapping */
    if (phys_start < 0x08000000) {
        return phys_to_virt(phys_start);
    }

    /* For high MMIO: use a fixed MMIO window at 0xF8000000.
     * Page-align both physical and virtual addresses. */
    uint32_t phys_base = phys_start & ~0xFFF;   /* page-align down */
    uint32_t phys_end_aligned = (phys_end + 0xFFF) & ~0xFFF; /* page-align up */
    uint32_t page_offset = phys_start - phys_base;

    /* Use a counter to allocate unique virtual pages for each MMIO mapping */
    static uint32_t mmio_virt_next = 0xF8000000;
    uint32_t virt_base = mmio_virt_next;
    mmio_virt_next += (phys_end_aligned - phys_base);

    extern void map_4k_page(uint32_t phys, uint32_t virt, uint32_t flags);
    for (uint32_t pa = phys_base; pa < phys_end_aligned; pa += 0x1000) {
        uint32_t va = virt_base + (pa - phys_base);
        map_4k_page(pa, va, PAGE_PRESENT | PAGE_WRITABLE | PTE_CACHE_DISABLE);
    }

    return (void *)(virt_base + page_offset);
}

/* Physical memory allocator stubs (map to dma_alloc_coherent / page allocator) */
static inline uintptr_t pmmngr_alloc_block(void) {
    uint32_t dma_handle;
    void *ptr = dma_alloc_coherent(4096, &dma_handle);
    if (!ptr) return 0;
    return virt_to_phys(ptr);
}

static inline uintptr_t pmmngr_alloc_blocks(uint32_t count) {
    uint32_t dma_handle;
    void *ptr = dma_alloc_coherent(count * 4096, &dma_handle);
    if (!ptr) return 0;
    return virt_to_phys(ptr);
}

static inline void pmmngr_free_block(uintptr_t phys) {
    dma_free_coherent(phys_to_virt(phys), 4096);
}

static inline void pmmngr_free_blocks(void *phys, uint32_t count) {
    dma_free_coherent(phys_to_virt((uint32_t)phys), count * 4096);
}

/* Page table query */
static inline void *get_page_entry(uint32_t vaddr) {
    (void)vaddr;
    return NULL;  /* stub */
}

static inline void vmmngr_free_pages(virtual_addr vaddr, uint32_t size) {
    (void)vaddr; (void)size;
}

#endif /* KERNEL_HILLSONOS_H */
