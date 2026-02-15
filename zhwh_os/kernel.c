#include "vga.h"
#include "printf.h"
#include "uart.h"
#include "netdebug.h"
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
#include "net/wifi/atheros.h"

// Forward declarations for task types
typedef struct {
    uint32_t *phys;
    uint32_t *virt;
} page_t;

extern void copy_kernel_mappings_to_pd(uint32_t *pd_user);

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
        
        
        // 🔥 内核栈溢出检测：在栈底设置哨兵
        extern uint32_t stack_base;
        extern uint32_t stack_top;
        uint32_t *stack_sentinel = &stack_base;
        *stack_sentinel = 0xDEADBEEF;  // 哨兵值

        // ⚠️ 保存内核页目录物理地址（在切换到用户进程之前）
        extern uint32_t kernel_page_directory_phys;
        uint32_t cr3_value;
        __asm__ volatile("movl %%cr3, %0" : "=r"(cr3_value));
        kernel_page_directory_phys = cr3_value & ~0xFFF;


        //printf("[kernel_main] Saved kernel CR3 phys=0x%x\n", kernel_page_directory_phys);


        // 显示内核栈信息
        printf("[kernel_main] Stack: base=0x%x top=0x%x size=%u KB\n",
               (uint32_t)&stack_base, (uint32_t)&stack_top,
               ((uint32_t)&stack_top - (uint32_t)&stack_base) / 1024);

        // 🔥 临时禁用所有 UART 和复杂初始化，测试最小启动
        // uart_init();
        // uart_puts("[UART] Serial port initialized at 115200 baud\n");

        // 🔥 初始化以太网调试接口
        //netdebug_init();

        

        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted with Multiboot 2!\n");

        

        // uart_puts("[KERNEL] Booted with Multiboot 2!\n");
        printf("Magic: 0x%x\n", mb_magic);
        printf("Info addr: 0x%x\n", mb_info_addr);
        // uart_debug("[KERNEL] Magic: 0x%x, Info: 0x%x\n", mb_magic, mb_info_addr);

        // 验证 multiboot2 魔数
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            printf("ERROR: Invalid multiboot2 magic: 0x%x\n", mb_magic);
            // uart_panic("ERROR: Invalid multiboot2 magic!");  // 🔥 注释掉，避免未初始化的串口访问导致重启
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

        // 🔥 初始化 IOAPIC（必须在键盘初始化之前！）
        extern void ioapicinit(void);
        ioapicinit();
        printf("IOAPIC initialized\n");

        printf("Before seginit\n");
        seginit();
        printf("After seginit\n");

        printf("Before tss_init\n");
        tss_init();
        printf("After tss_init\n");

        printf("Before tvinit\n");
        tvinit();
        printf("After tvinit\n");
        
        idtinit();
        printf("segment idt init is ok\n");

        // 🔥🔥 在开中断前再次确保 FPU 已初始化（防止 Trap 19）
        __asm__ volatile("fninit");
        __asm__ volatile("fnclex");
        uint32_t cr0_check;
        __asm__ volatile("movl %%cr0, %0" : "=r"(cr0_check));
        cr0_check &= ~(1 << 3);  // 清除 TS
        cr0_check &= ~(1 << 2);  // 清除 EM
        __asm__ volatile("movl %0, %%cr0" : : "r"(cr0_check));

        // 🔥 调试：打印当前栈指针
        uint32_t current_esp;
        uint32_t current_ebp;
        __asm__ volatile("movl %%esp, %0" : "=r"(current_esp));
        __asm__ volatile("movl %%ebp, %0" : "=r"(current_ebp));
        printf("[DEBUG] Current ESP=0x%x, EBP=0x%x\n", current_esp, current_ebp);
        printf("[FPU] Re-initialized before STI\n");

        // ⚠️ 暂时注释掉 STI，避免中断处理程序的问题导致系统崩溃
        // 启用全局中断（重要！）
        //__asm__ volatile("sti");
        printf("Global interrupts DISABLED (sti commented out for debugging)\n");

        // 在启用中断后初始化键盘驱动
        extern void keyboard_init(void);
        keyboard_init();
        printf("Keyboard driver initialized\n");

        // ⚠️⚠️⚠️ 关键修复：在启用中断后重新配置PIC
        // 原因：PIC可能在初始化过程中被重置
        printf("Re-configuring PIC after enabling interrupts...\n");
        unsigned char mask1_after = inb(0x21);
        printf("PIC mask before keyboard enable: 0x%x\n", mask1_after);
        mask1_after &= ~0x02;  // 清除bit 1 (IRQ1)

        // ⚠️⚠️⚠️ 强制使用内联汇编，确保 outb 不会被优化
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a" (mask1_after), "dN" ((uint16_t)0x21)
            : "memory"
        );

        // 再次验证
        unsigned char mask1_final = inb(0x21);
        printf("PIC mask after keyboard enable: 0x%x (expected: 0x%x)\n", mask1_final, mask1_after);
        printf("IRQ1 (keyboard) %s\n", (mask1_final & 0x02) ? "DISABLED ❌" : "ENABLED ✅");

        // 如果还是没变，说明 outb 完全不工作
        if (mask1_final == mask1_after && (mask1_final & 0x02)) {
            printf("⚠️⚠️⚠️ WARNING: outb() is not working! PIC mask unchanged!\n");
        } else if (mask1_final != mask1_after) {
            printf("⚠️⚠️⚠️ WARNING: PIC mask changed unexpectedly!\n");
        }

        // 初始化文件系统
        extern void fs_init(void);
        extern void vfs_set_root(struct super_block *sb);
        extern struct super_block *ramfs_mount(void);

        printf("Initializing file system...\n");
        fs_init();  // 这会调用 ramfs_mount 并设置根文件系统
        printf("File system initialized\n");

        // 初始化 PCI 总线
        extern int pci_init(void);
        printf("Initializing PCI...\n");
        pci_init();
        printf("PCI initialized\n");

        // 初始化 USB 总线（依赖 PCI）
        extern int usb_init(void);
        printf("Initializing USB...\n");
        if (usb_init() == 0) {
            printf("USB initialized\n");
        } else {
            printf("USB initialization failed (may not be critical)\n");
        }

        // 测试 USB 鼠标驱动
        printf("\n========================================\n");
        printf("Testing USB Mouse Driver...\n");
        printf("========================================\n");

        extern int usb_mouse_get_count(void);
        extern int usb_mouse_read(int mouse_index, void *report);
        extern int usb_mouse_data_available(int mouse_index);

        int mouse_count = usb_mouse_get_count();
        printf("USB Mouse count: %d\n", mouse_count);

        if (mouse_count > 0) {
            // 🔥 测试新的周期性轮询 API
            extern void usb_mouse_test_periodic(void);
            usb_mouse_test_periodic();
        } else {
            printf("No USB mouse found!\n");
        }

        printf("========================================\n\n");

        // 初始化网络协议栈
        extern void net_init(void);
        extern int loopback_init(void);
        extern int loopback_send_test(void);
        extern int rtl8139_init(void);
        extern int e1000_init(void);

        net_init();
        loopback_init();
        // 🔥 网卡驱动移到用户空间命令手动初始化（通过系统调用）
        // rtl8139_init();  // 初始化 RTL8139 网卡驱动
        // e1000_init();    // 初始化 E1000 网卡驱动
        printf("Network stack initialized\n");
        printf("Use 'net init' command to initialize network cards\n");

        // 🔥 启用以太网调试接口（网络初始化后）
        printf("Enabling network debug interface...\n");
        netdebug_enable(1);
        netdebug_set_level(2);  // 设置为 INFO 级别 (2=INFO, 1=WARN, 0=ERROR)
        // 🔥 替换 netdebug_info 为 printf，避免发送 UDP 包
        printf("[KERNEL] Network debug interface enabled\n");
        printf("[KERNEL] All firmware loading messages will be sent to network\n");
        printf("Network debug enabled on UDP port 9999\n");
        netdebug_stats();  // 显示调试接口统计信息

        // 发送网络测试包
        printf("\n=== Network Test ===\n");

        // 🔥 检查内核栈溢出
        extern uint32_t stack_base;
        if (*(uint32_t*)&stack_base != 0xDEADBEEF) {
            printf("⚠️⚠️⚠️ WARNING: Stack overflow detected! Sentinel corrupted!\n");
            printf("Expected 0xDEADBEEF, got 0x%x\n", *(uint32_t*)&stack_base);
        } else {
            printf("✓ Stack sentinel OK\n");
        }

        loopback_send_test();
        printf("=== Network Test Complete ===\n\n");

        // ⚠️⚠️⚠️ 注释掉 WiFi 自动测试，避免未初始化访问导致重启
        // WiFi 初始化必须由用户程序通过 syscall 手动触发
        /*
        // WiFi 数据包测试
        extern int wifi_send_test_packet(void);
        extern void wifi_show_stats(void);
        printf("\n=== WiFi Data Packet Test ===\n");
        wifi_show_stats();
        printf("\nSending test packet...\n");
        wifi_send_test_packet();
        printf("=== WiFi Test Complete ===\n\n");
        */

        // ⚠️⚠️⚠️ 注意：PIC已经在启用中断后配置完毕
        // 不要在这里重复配置，避免覆盖之前的设置

        // 在VGA上显示测试消息，确认系统正常运行
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        vga[10] = (0x0E << 8) | 'T';
        vga[11] = (0x0E << 8) | 'E';
        vga[12] = (0x0E << 8) | 'S';
        vga[13] = (0x0E << 8) | 'T';
        printf("VGA test: wrote TEST to screen at position 10-13\n");

        // 实验2：检测并初始化 VBE/Framebuffer（从 Multiboot2 读取）
        printf("\n=== VBE/Framebuffer Initialization ===\n");

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
                    printf("=== Framebuffer Tag Structure (Hex Dump) ===\n");

                    // 打印整个结构体的十六进制dump
                    uint8_t *fb_bytes = (uint8_t *)fb;
                    printf("Raw bytes (%d bytes):\n", sizeof(multiboot_tag_framebuffer_t));
                    for (int i = 0; i < sizeof(multiboot_tag_framebuffer_t); i++) {
                        if (i % 16 == 0) {
                            printf("  %04x: ", i);
                        }
                        printf("%02x ", fb_bytes[i]);
                        if (i % 16 == 15) {
                            printf("\n");
                        }
                    }
                    printf("\n");

                    // 手动解析每个字段来验证
                    printf("Manual field parsing:\n");
                    printf("  type (offset 0x00):     0x%02x%02x%02x%02x -> %d\n",
                           fb_bytes[3], fb_bytes[2], fb_bytes[1], fb_bytes[0],
                           *(uint32_t*)&fb_bytes[0]);
                    printf("  size (offset 0x04):     0x%02x%02x%02x%02x -> %d\n",
                           fb_bytes[7], fb_bytes[6], fb_bytes[5], fb_bytes[4],
                           *(uint32_t*)&fb_bytes[4]);
                    printf("  framebuffer_addr (0x08): 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                           fb_bytes[15], fb_bytes[14], fb_bytes[13], fb_bytes[12],
                           fb_bytes[11], fb_bytes[10], fb_bytes[9], fb_bytes[8]);
                    printf("  framebuffer_pitch (0x10): 0x%02x%02x%02x%02x -> %d\n",
                           fb_bytes[19], fb_bytes[18], fb_bytes[17], fb_bytes[16],
                           *(uint32_t*)&fb_bytes[16]);
                    printf("  framebuffer_width (0x14):  0x%02x%02x%02x%02x -> %d\n",
                           fb_bytes[23], fb_bytes[22], fb_bytes[21], fb_bytes[20],
                           *(uint32_t*)&fb_bytes[20]);
                    printf("  framebuffer_height (0x18): 0x%02x%02x%02x%02x -> %d\n",
                           fb_bytes[27], fb_bytes[26], fb_bytes[25], fb_bytes[24],
                           *(uint32_t*)&fb_bytes[24]);
                    printf("  framebuffer_bpp (0x1C):    0x%02x -> %d\n",
                           fb_bytes[28], fb_bytes[28]);
                    printf("  framebuffer_type (0x1D):   0x%02x -> %d\n",
                           fb_bytes[29], fb_bytes[29]);
                    printf("  reserved (0x1E):            0x%02x%02x -> %d\n",
                           fb_bytes[31] | (fb_bytes[30] << 8),
                           *(uint16_t*)&fb_bytes[30]);
                    printf("\n");

                    // 结构体解析值
                    printf("Structure field values:\n");
                    printf("  framebuffer_addr:   0x%08x%08x\n",
                           (uint32_t)(fb->framebuffer_addr >> 32),
                           (uint32_t)fb->framebuffer_addr);
                    printf("  framebuffer_pitch:  0x%04x (%d)\n", fb->framebuffer_pitch, fb->framebuffer_pitch);
                    printf("  framebuffer_width:  0x%04x (%d)\n", fb->framebuffer_width, fb->framebuffer_width);
                    printf("  framebuffer_height: 0x%04x (%d)\n", fb->framebuffer_height, fb->framebuffer_height);
                    printf("  framebuffer_bpp:    0x%02x (%d)\n", fb->framebuffer_bpp, fb->framebuffer_bpp);
                    printf("  framebuffer_type:   0x%02x (%d)\n", fb->framebuffer_type, fb->framebuffer_type);
                    printf("=========================================\n");

                    // ⚠️ 临时修复：手动解析字段,避免结构体对齐问题
                    uint64_t fb_addr = *(uint64_t*)&fb_bytes[8];
                    uint32_t fb_pitch = *(uint32_t*)&fb_bytes[16];
                    uint32_t fb_width = *(uint32_t*)&fb_bytes[20];
                    uint32_t fb_height = *(uint32_t*)&fb_bytes[24];
                    uint8_t fb_bpp = fb_bytes[28];
                    uint8_t fb_type = fb_bytes[29];

                    printf("✓ Manual parsing successful!\n");
                    printf("  Parsed values: addr=0x%x, %dx%d, bpp=%d, type=%d\n",
                           (uint32_t)fb_addr, fb_width, fb_height, fb_bpp, fb_type);

                    // 检测是否为文本模式 (使用手动解析的正确值)
                    if (fb_type != 1) {  // 必须是 RGB 图形模式
                        printf("✗ ERROR: Not RGB framebuffer mode!\n");
                        printf("  framebuffer_type=%d (must be 1 for RGB)\n", fb_type);
                        printf("  Type meanings:\n");
                        printf("    0 = Indexed color\n");
                        printf("    1 = RGB (required for graphics) ✅\n");
                        printf("    2 = EGA text\n");
                        printf("    3 = VBE text (current)\n");
                        printf("\n");
                        printf("  Current settings:\n");
                        printf("    addr: 0x%x\n", (uint32_t)fb_addr);
                        printf("    resolution: %dx%d\n", fb_width, fb_height);
                        printf("    bpp: %d, pitch: %d\n", fb_bpp, fb_pitch);
                        printf("\n");
                        printf("  GUI functions will NOT work!\n");
                        printf("  Please check GRUB configuration:\n");
                        printf("    - Ensure gfxmode is set correctly\n");
                        printf("    - Try: set gfxmode=1024x768x16\n");
                        printf("    - Try: set gfxmode=800x600x16\n");
                        found_framebuffer = 0;
                        break;
                    }

                    // 额外检查：bpp 和 pitch 必须有效
                    if (fb_bpp == 0 || fb_pitch == 0) {
                        printf("✗ ERROR: Invalid framebuffer parameters!\n");
                        printf("  bpp=%d (must be > 0)\n", fb_bpp);
                        printf("  pitch=%d (must be > 0)\n", fb_pitch);
                        found_framebuffer = 0;
                        break;
                    }

                    printf("✓ RGB framebuffer mode detected!\n");

                    found_framebuffer = 1;

                    // 初始化 VBE 驱动（使用手动解析的正确值）
                    extern void vbe_init_from_multiboot(uint64_t fb_addr, uint32_t width,
                                                        uint32_t height, uint32_t pitch, uint8_t bpp);
                    vbe_init_from_multiboot(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
                    printf("✓ VBE driver initialized from Multiboot2 info\n");
                    break;
                }
                fb_tag = (multiboot_tag_t *)((uint8_t *)fb_tag + ((fb_tag->size + 7) & ~7));
            }

            if (!found_framebuffer) {
                printf("✗ No valid framebuffer info available from GRUB\n");
                printf("  Note: GRUB did not provide RGB framebuffer (type=1)\n");
                printf("\n");
                printf("⚠ WARNING: GUI functions will NOT work!\n");
                printf("\n");
                printf("Real-mode VBE thunk is disabled due to complexity.\n");
                printf("Please fix GRUB configuration instead:\n");
                printf("\n");
                printf("Solution: Add these lines to zh.sh BEFORE menuentry entries:\n");
                printf("  set gfxmode=1024x768x32\n");
                printf("  set gfxpayload=keep\n");
                printf("  insmod all_video\n");
                printf("  terminal_output gfxterm\n");
                printf("\n");
                printf("Or use VGA 13h mode (320x200x256) as fallback.\n");
            }
        }

        printf("=============================================\n\n");

        // ⚠️⚠️⚠️ 临时修复：注释掉 PID=1 内核任务，避免切换回它时崩溃
        // 原因：PID=1 内核任务的栈空间不足，切换回它时会触发 triple fault
        // 详见：PID1_CRASH_FIX.md
        // task_t *th_k=init_task(0);
        // printf("start kernel task\n");
        // start_task_kernel(th_k,kernel_task_main);

        // 直接创建用户任务作为第一个任务
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
        // 屏蔽之后的日志
        #if 0

        // 调试：输出multiboot2模块信息
        dump_multiboot2_modules(mb_info_addr);

        // 重要：先手动调用 user_task_main 进行初始化（页表、模块加载等）
        // 初始化完成后，user_task_main 会返回
        user_task_main(th_u);

        // 初始化完成后，将用户任务添加到调度器队列
        // start_task 会将 th_u 添加到 sched_root 链表，这样调度器才能找到它
        start_task(th_u, user_task_main);

        // 设置任务状态为 PS_CREATED，让调度器知道需要跳转到用户态
        th_u->state = PS_CREATED;
        printf("[kernel_main] User task initialized, state=PS_CREATED\n");

        printf("user task 0x%x kernel task 0x%x\n",th_u,th_k);
        #endif

        // 实际需要的代码（不打印日志）
        user_task_main(th_u);
        start_task(th_u, user_task_main);
        th_u->state = PS_CREATED;

        /*
        // 创建第二个用户进程（测试调度）- 已弃用
        // 现在使用 fork() 系统调用替代
        printf("=== Creating second user task for scheduler testing ===\n");
        printf("th_u = 0x%x, th_u->pde = 0x%x, th_u->cr3 = 0x%x\n",
               th_u, th_u->pde, th_u->cr3);

        task_t *th_u2 = init_task(true);  // 创建第二个任务
        if (th_u2) {
            printf("th_u2 created: 0x%x\n", th_u2);

            // 实现简单的 fork 机制：为第二个进程创建独立的页目录
            // 参考：
            // - Brown University CS: "fork() clones user-space processes with full address space copies"
            // - Reddit r/osdev: "fork() and copy-on-write in Linux"
            //
            // 关键点：
            // 1. 每个进程需要独立的页目录（CR3）
            // 2. 页目录项指向相同的物理页（共享内存）
            // 3. 内核映射（0xC0000000+）需要复制到每个页目录

            extern page_t alloc_page_table_();
            extern void copy_kernel_mappings_to_pd(uint32_t *pd_user);
            extern uint32_t pmm_alloc_page(void);

            // 1. 分配新的页目录
            page_t pde2 = alloc_page_table_();
            th_u2->pde = (uint32_t*)pde2.phys;
            th_u2->cr3 = (uint32_t*)pde2.phys;  // CR3 字段存储物理地址
            uint32_t *pd_user2 = pde2.virt;

            printf("[fork] Allocated new page dir: phys=0x%x, virt=0x%x\n",
                   pde2.phys, pd_user2);

            // 2. 复制内核高端映射（768-1023，即 0xC0000000-0xFFFFFFFF）
            // 这样内核代码在所有进程中都可访问
            copy_kernel_mappings_to_pd(pd_user2);
            printf("[fork] Copied kernel mappings to new page dir\n");

            // 3. 复制用户空间映射（0-767，即 0x00000000-0xBFFFFFFF）
            // 这是简单的页表复制，不是 Copy-on-Write
            // 两个进程会共享相同的物理页，但有独立的页表结构

            // 获取第一个任务的页目录虚拟地址
            extern uint32_t pd[];
            uint32_t *pd_user1 = (uint32_t*)phys_to_virt((uint32_t)th_u->pde);

            int copied_entries = 0;
            for (int i = 0; i < 768; i++) {  // 用户空间：0-767
                if (pd_user1[i] & PAGE_PRESENT) {
                    pd_user2[i] = pd_user1[i];  // 复制页表项指针
                    copied_entries++;
                }
            }
            printf("[fork] Copied %d user space page table entries\n", copied_entries);

            // 4. 分配新的内核栈
            uint32_t kstack2 = pmm_alloc_page();
            th_u2->kstack = (uint32_t*)kstack2;
            th_u2->esp0 = (uint32_t)(kstack2 + PAGE_SIZE);

            printf("[fork] Task2: pde=0x%x, cr3=0x%x, kstack=0x%x\n",
                   th_u2->pde, th_u2->cr3, th_u2->kstack);

            // 5. 复制 trapframe
            if (!th_u2->tf) {
                printf("[fork] ERROR: th_u2->tf is NULL!\n");
            } else {
                memcpy(th_u2->tf, th_u->tf, sizeof(struct trapframe));
                printf("[fork] Copied trapframe: eip=0x%x, esp=0x%x\n",
                       th_u2->tf->eip, th_u2->tf->esp);
            }

            start_task_user(th_u2, user_task_main);
            printf("[fork] Second user task created: 0x%x\n", th_u2);
        } else {
            printf("Failed to create second user task!\n");
        }
        printf("=== Second user task creation completed ===\n");
        */

        // 启动调度器
        // printf("Starting scheduler with multiple tasks...\n");
        efficient_scheduler_loop();
        // printf("Kernel main completed successfully!\n");
	return (42);
}
