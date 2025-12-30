#include "vga.h"
#include "printf.h"
#include "pci.h"
#include "multiboot.h"
#include "highmem_mapping.h"
#include "acpi.h"
#include "lapic.h"
#include "ioapic.h"
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
struct multiboot;

void dump_multiboot_modules(struct multiboot *mb) {
    if (!mb) {
        printf("no multiboot info\n");
        return;
    }
    if (!(mb->flags & MULTIBOOT_FLAG_MODS)) {
        printf("no modules\n");
        return;
    }

    uint32_t count = mb->mods_count;
    uint32_t mods_addr = mb->mods_addr; // 这通常是物理地址 (GRUB)
    multiboot_module_t *mods = (multiboot_module_t *)phys_to_virt(mods_addr);

    printf("multiboot modules count=%u\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t start = mods[i].mod_start;
        uint32_t end   = mods[i].mod_end;
        uint32_t strp  = mods[i].string;
        const char *name = (const char *)phys_to_virt(strp); // 字符串地址也可能需要转
        printf("mod %u: start=0x%x end=0x%x name=%s\n", i, start, end, name);
    }
}


int
kernel_main(struct multiboot * mb)
{
        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted!\n");
        printf("Physical: 0x%x\n", 0x100000);
        printf("Virtual:  0x%x\n", 0xC0000000);      
        //mod = (multiboot_module_t *)phys_to_virt(mb->mods_addr);
        
        mpinit();
        init_highmem_mapping();
        //acpi_init();

        // 保存 multiboot 信息,供内存管理使用
        multiboot_info = mb;
        printf("Multiboot info: mem_lower=%u KB, mem_upper=%u KB\n",
               mb->mem_lower, mb->mem_upper);

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

        // 调试：输出multiboot模块信息
        dump_multiboot_modules(mb);

        start_task_user(th_u,user_task_main);
        printf("user task 0x%x kernel task 0x%x\n",th_u,th_k);

        // 启动调度器
        efficient_scheduler_loop();
        printf("Kernel main completed successfully!\n");
	return (42);
}
