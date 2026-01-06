#include "vga.h"
#include "printf.h"
#include "pci.h"
#include "multiboot2.h"
#include "highmem_mapping.h"
#include "acpi.h"
#include "lapic.h"
#include "ioapic.h"
#include "vbe.h"
//#include "ebda.h"
#include "segment.h"
#include "interrupt.h"
#include "mm.h"
#include "kmalloc.h"
//#include "task.h"
#include "sched.h"
#include "x86/io.h"

extern void* _kernel_start_virtual;
extern void* _kernel_end_virtual;
extern void* data;
struct task_t * th_u,* th_k;

/* 转换 multiboot2 物理地址到虚拟地址 */
static inline void* mb2_phys_to_virt(uint32_t phys_addr) {
    return (void*)((uint8_t*)0xC0000000 + phys_addr);
}

void dump_multiboot2_modules(uint32_t mb_info_addr) {
    if (!mb_info_addr) {
        printf("no multiboot2 info\n");
        return;
    }

    // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
    uint32_t *mod_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mod_info_ptr + 8);
    int found_modules = 0;

    // 遍历所有标签
    int tag_count = 0;
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        printf("Tag %d: type=%d, size=%d\n", tag_count++, tag->type, tag->size);
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            multiboot_tag_module_t *mod_tag = (multiboot_tag_module_t *)tag;
            printf("mod %d: start=0x%x end=0x%x cmdline=%s\n",
                   found_modules,
                   mod_tag->mod_start,
                   mod_tag->mod_end,
                   mod_tag->cmdline);
            found_modules++;
        }
        // 移动到下一个标签 (大小必须是8字节对齐)
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (!found_modules) {
        printf("no modules found\n");
    }
}


int
kernel_main(uint32_t mb_magic, uint32_t mb_info_addr)
{
        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted with Multiboot 2!\n");
        printf("Magic: 0x%x\n", mb_magic);
        printf("Info addr: 0x%x\n", mb_info_addr);

        // 验证 multiboot2 魔数
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            printf("ERROR: Invalid multiboot2 magic: 0x%x\n", mb_magic);
            return -1;
        }

        mpinit();
        init_highmem_mapping();
        //acpi_init();

        // 保存 multiboot2 信息,供内存管理使用
        multiboot2_info_addr = mb_info_addr;

        // 解析基本内存信息
        uint32_t mem_lower = 0, mem_upper = 0;

        // 在 Multiboot 2 中，ebx 指向信息结构的开始地址
        // 信息结构从大小字段开始(4字节)，然后是保留字段(4字节)，再然后是标签数组
        uint32_t *mb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);

        // 第一个标签从偏移8字节开始
        multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);

        while (tag->type != MULTIBOOT_TAG_TYPE_END) {
            if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
                multiboot_tag_basic_meminfo_t *meminfo = (multiboot_tag_basic_meminfo_t *)tag;
                mem_lower = meminfo->mem_lower;
                mem_upper = meminfo->mem_upper;
                break;
            }
            tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
        }

        printf("Multiboot2 info: mem_lower=%u KB, mem_upper=%u KB\n", mem_lower, mem_upper);

        printf("Initializing memory management...\n");
        if(mm_init()==0){
            printf("Memory management initialized\n");
        } else {
            printf("Memory management initialization failed!\n");
        }

        printf("After mm_init, before hardware init\n");

        // 必须初始化 LAPIC，因为 logical_cpu_id() 依赖它
        lapicinit();

        printf("Before seginit\n");
        seginit();
        printf("After seginit\n");

        printf("Before tss_init\n");
        tss_init();
        printf("After tss_init\n");

        printf("Before tvinit\n");
        tvinit();
        printf("After tvinit\n");

        printf("segment idt init is ok\n");
        idtinit();

        // 启用全局中断（重要！）
        __asm__ volatile("sti");
        printf("Global interrupts enabled\n");

        // 在启用中断后初始化键盘驱动
        extern void keyboard_init(void);
        keyboard_init();
        printf("Keyboard driver initialized\n");

        // 启用键盘中断 (IRQ1)
        // 传统PIC方法
        unsigned char mask1 = inb(0x21);
        printf("PIC initial mask: 0x%x\n", mask1);
        mask1 &= ~0x02;  // 清除bit 1 (IRQ1)
        outb(mask1, 0x21);

        // 验证是否设置成功
        unsigned char mask1_verify = inb(0x21);
        printf("PIC new mask: 0x%x (verified)\n", mask1_verify);
        printf("Keyboard IRQ1 enabled via PIC (mask & 0x02 = %d)\n", mask1_verify & 0x02);

        // 不再使用IOAPIC，避免冲突
        // extern void ioapicenable(int irq, int cpunum);
        // ioapicenable(1, 0);

        // 测试：读取键盘状态
        unsigned char kbd_status = inb(0x64);
        printf("Keyboard status port: 0x%x\n", kbd_status);

        // 在VGA上显示测试消息，确认系统正常运行
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        vga[10] = (0x0E << 8) | 'T';
        vga[11] = (0x0E << 8) | 'E';
        vga[12] = (0x0E << 8) | 'S';
        vga[13] = (0x0E << 8) | 'T';
        printf("VGA test: wrote TEST to screen at position 10-13\n");

        // 实验2：检测并打印 VBE/Framebuffer 信息（从 Multiboot2 读取）
        printf("\n=== VBE/Framebuffer Detection (Multiboot2) ===\n");

        // 遍历 multiboot2 标签查找 framebuffer 信息
        {
            // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
            uint32_t *fb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);
            multiboot_tag_t *fb_tag = (multiboot_tag_t *)((uint8_t *)fb_info_ptr + 8);
            int found_framebuffer = 0;

            while (fb_tag->type != MULTIBOOT_TAG_TYPE_END) {
                if (fb_tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                    multiboot_tag_framebuffer_t *fb = (multiboot_tag_framebuffer_t *)fb_tag;
                    printf("✓ Framebuffer info available!\n");
                    printf("  framebuffer_addr:   0x%llx\n", fb->framebuffer_addr);
                    printf("  framebuffer_pitch:  %d\n", fb->framebuffer_pitch);
                    printf("  framebuffer_width:  %d\n", fb->framebuffer_width);
                    printf("  framebuffer_height: %d\n", fb->framebuffer_height);
                    printf("  framebuffer_bpp:    %d\n", fb->framebuffer_bpp);
                    printf("  framebuffer_type:   %d\n", fb->framebuffer_type);
                    found_framebuffer = 1;
                    break;
                }
                fb_tag = (multiboot_tag_t *)((uint8_t *)fb_tag + ((fb_tag->size + 7) & ~7));
            }

            if (!found_framebuffer) {
                printf("✗ No framebuffer info available\n");
            }
        }

        printf("=============================================\n\n");

        task_t *th_k=init_task(0);
        printf("start kernel task\n");
        start_task_kernel(th_k,kernel_task_main);
        task_t *th_u=init_task(1);

        // 注释掉kmalloc测试,避免影响用户进程加载
        /*
        // 输出内存检测结果
        print_memory_detection_result();

        // 输出 PMM 统计信息
        pmm_print_stats();

        // 测试 kmalloc 和 kfree 功能
        printf("\n=== Testing kmalloc/kfree functionality ===\n");

        // 测试 1: 小内存分配（使用 early pool）
        printf("\nTest 1: Small allocation (early pool)\n");
        void *ptr1 = kmalloc(128);
        printf("  Allocated 128 bytes at: 0x%x\n", (uint32_t)ptr1);

        // 测试 2: 中等内存分配（使用 early pool）
        printf("\nTest 2: Medium allocation (early pool)\n");
        void *ptr2 = kmalloc(1024);
        printf("  Allocated 1024 bytes at: 0x%x\n", (uint32_t)ptr2);

        // 测试 3: 大内存分配（使用 PMM）
        printf("\nTest 3: Large allocation (PMM)\n");
        void *ptr3 = kmalloc(8192);  // 8KB = 2 pages
        printf("  Allocated 8192 bytes at: 0x%x\n", (uint32_t)ptr3);

        // 测试 4: 超大内存分配（使用 PMM）
        printf("\nTest 4: Extra large allocation (PMM)\n");
        void *ptr4 = kmalloc(16384);  // 16KB = 4 pages
        printf("  Allocated 16384 bytes at: 0x%x\n", (uint32_t)ptr4);

        // 测试 5: kzalloc 分配并清零
        printf("\nTest 5: kzalloc (zero-initialized)\n");
        void *ptr5 = kzalloc(512);
        printf("  Allocated 512 zero-initialized bytes at: 0x%x\n", (uint32_t)ptr5);

        // 显示分配统计
        printf("\n");
        kmalloc_print_stats();

        // 测试 6: kfree 测试
        printf("\nTest 6: Testing kfree\n");
        printf("  Freeing ptr2 (1024 bytes)\n");
        kfree(ptr2);

        printf("  Freeing ptr4 (16384 bytes)\n");
        kfree(ptr4);

        // 再次显示统计
        printf("\nAfter freeing:\n");
        kmalloc_print_stats();

        printf("=== kmalloc/kfree tests completed ===\n\n");
        */

        // 启动用户进程
        printf("start user task \n");

        // 调试：输出multiboot2模块信息
        dump_multiboot2_modules(mb_info_addr);

        start_task_user(th_u,user_task_main);
        printf("user task 0x%x kernel task 0x%x\n",th_u,th_k);

        // 启动调度器
        efficient_scheduler_loop();
        printf("Kernel main completed successfully!\n");
	return (42);
}
