// syscall.c
#include "vga.h"
#include "interrupt.h"
#include "printf.h"
#include "task.h"
#include "multiboot2.h"
#include "highmem_mapping.h"
#include "page.h"
#include "lapic.h"
#include "net.h"
#include "pci.h"
#include "x86/io.h"  // 🔥 添加：引入 outl/inl 函数

// PCI 配置空间 I/O 端口
#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC

// 系统调用号定义
#define SYS_NET_PING 30
#define SYS_NET_IFCONFIG 31
#define SYS_WIFI_SCAN 32
#define SYS_WIFI_CONNECT 33
#define SYS_WIFI_DISCONNECT 34
#define SYS_WIFI_STATUS 35
#define SYS_WIFI_INIT 36
#define SYS_WIFI_FW_BEGIN 37
#define SYS_WIFI_FW_CHUNK 38
#define SYS_WIFI_FW_END 39
#define SYS_WIFI_LOAD_FIRMWARE 40
//#define SYS_EXECV 41  // 暂时禁用
#define SYS_LSPCI 42  // 🔥 新增：列出 PCI 设备
#define SYS_NET_INIT_RTL8139 43  // 🔥 新增：初始化 RTL8139
#define SYS_NET_INIT_E1000 44   // 🔥 新增：初始化 E1000
#define SYS_NET_SEND_UDP 45     // 🔥 新增：发送 UDP 包
#define SYS_NET_SET_DEVICE 46   // 🔥 设置当前使用的网卡
#define SYS_NET_POLL_RX 47      // 🔥 轮询RX（调试用）
#define SYS_NET_DUMP_REGS 48     // 🔥 转储网卡寄存器状态
#define SYS_NET_ARP 49           // 🔥 ARP 命令（显示/扫描 ARP 缓存）
#define SYS_NET_DUMP_RX_REGS 50 // 🔥 转储 RX 寄存器（详细）
#define SYS_NET_IFUP 51        // 🔥 启动网络接口
//#define SYS_NET_RAW_DUMP_RX_DESC 52  // 🔥 暂时注释掉
#define SYS_MSI_TEST 60        // 🔥 MSI 测试
#define SYS_NET_LOOPBACK_TEST 61  // 🔥 E1000 硬件 loopback 测试（轮询）
#define SYS_NET_LOOPBACK_TEST_INT 62  // 🔥 E1000 硬件 loopback 测试（中断）

// GUI 系统调用
#define SYS_GUI_FB_INFO 70      // 获取帧缓冲区信息
#define SYS_GUI_FB_BLIT 71      // 位图传输到帧缓冲区
#define SYS_GUI_INPUT_READ 72   // 读取输入设备事件
#define SYS_USB_MOUSE_POLL 73   // 轮询 USB 鼠标事件

// WiFi
static uint8_t  *fw_buf      = NULL;
static uint32_t  fw_size     = 0;
static uint32_t  fw_received = 0;
static uint32_t  fw_checksum = 0;

// USB 鼠标状态
static int usb_mouse_x = 512;
static int usb_mouse_y = 384;
static uint8_t usb_mouse_buttons = 0;

// 🔥 当前选择的网络设备名称（空字符串表示自动选择）
// 🔥 改为非 static，以便网络模块可以访问
char current_net_device[16] = {0};


#define FW_CHUNK_SIZE   4096
#define FW_MAX_SIZE     (2 * 1024 * 1024)  // 2MB，支持大容量固件（Intel 677KB + Atheros等）

// 
extern task_t *current_task[];

// 
extern void *kmalloc(uint32_t size);
extern void kfree(void *ptr);
extern void *memcpy(void *dst, const void *src, int n);

/*
typedef struct trapframe {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t vec;
    uint32_t err;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;   // user esp
    uint32_t ss;
} trapframe_t;
*/

/*  copy_from_user */
int copy_from_user(char *dst, const char *src, uint32_t n) {
    // /
    for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
    return 0;
}

/*  copy_to_user */
int copy_to_user(char *dst, const char *src, uint32_t n) {
    // 从内核空间拷贝到用户空间
    for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
    return 0;
}

/* sys_write(fd, buf, len) */
static int sys_write(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1 && fd != 2) return -1;  // 只支持stdout(1)和stderr(2)

    // 输出到串口
    extern void uart_putc(char c);
    for (uint32_t i = 0; i < len; ++i) {
        uart_putc(buf[i]);
    }
    return (int)len;
}

/*  */
void do_exit(int code) {
    extern task_t *current_task[];
    extern int need_resched;
    task_t *task = current_task[logical_cpu_id()];

    if (!task) {
        printf("[do_exit] No current task!\n");
        return;
    }

    printf("[do_exit] Task %d exiting with code %d\n", task->pid, code);

    // 1. 
    task->state = PS_TERMNAT;

    // 2. 
    // task->user_stack 
    if (task->user_stack != 0) {
        printf("[do_exit] Freeing user stack at 0x%x (virt)\n", (uint32_t)task->user_stack);
        //  user_stack  do_fork 
        pmm_free_page((uint32_t)task->user_stack);
        task->user_stack = 0;
    }

    // 3. 
    // task->pde 
    if (task->pde != 0 && task->pde != (uint32_t*)0x101000) {
        printf("[do_exit] Freeing user page directory at 0x%x (virt)\n", (uint32_t)task->pde);
        // TODO: 
        // 
        // pmm_free_page 
        //  virt_to_phys 
        printf("[do_exit] TODO: Need virt_to_phys conversion for PDE\n");
        task->pde = 0;
    }

    // 4.  trapframe
    if (task->tf != 0) {
        printf("[do_exit] Trapframe was at 0x%x (will be freed with kstack)\n", (uint32_t)task->tf);
        task->tf = 0;
    }

    // 5.  (kstack) 
    // 
    // 
    printf("[do_exit] Kernel stack at 0x%x (keeping for now)\n", task->kstack);

    // 6. 
    // TODO: 
    printf("[do_exit] Task %d marked as terminated\n", task->pid);

    // 7. 
    need_resched = 1;

    // 8. 
    // 
    task_t *next = task->next;
    if (next == task || next->state == PS_TERMNAT || next->state == PS_DESTROY) {
        printf("[do_exit] No more runnable tasks, halting...\n");
        __asm__ volatile("cli");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    // 
    // 
}

/* sys_exit(code) */
static void sys_exit(int code) {
    do_exit(code);

    //  do_exit 
    // 
    while (1) {
        __asm__ volatile("hlt");
    }
}

enum {
    SYS_PRINTF = 1,
    SYS_EXIT,
    SYS_YIELD,
    SYS_GET_MEM_STATS,
    SYS_READ_MEM,
    SYS_GET_MEM_USAGE,
    SYS_GETCHAR,      // = 7 
    SYS_PUTCHAR,      // = 8 ()
    SYS_GET_FRAMEBUFFER,  //  framebuffer
    SYS_GETCWD,       //
    SYS_WRITE,        //  SYS_FORK = 11
    SYS_FORK,         // fork  (11)
    SYS_OPEN = 20,    // open
    SYS_CLOSE,        // close
    SYS_READ,         // read
    SYS_LSEEK,        // lseek
    // 网络和 WiFi 系统调用使用宏定义（见上方）
    SYS_EXECV = 41,    // execv
};

void syscall_dispatch(struct trapframe *tf) {
    //  
    // printf("[syscall_dispatch] ENTER: tf=%p\n", tf);
    // printf("  eax=%d (syscall num), ebx=0x%x (arg1), ecx=0x%x (arg2), edx=0x%x (arg3)\n",
    //        tf->eax, tf->ebx, tf->ecx, tf->edx);
    // printf("  trapno=%d, eip=0x%x, cs=0x%x, ds=0x%x\n", tf->trapno, tf->eip, tf->cs, tf->ds);

    uint32_t num = tf->eax;
    uint32_t arg1 = tf->ebx;
    uint32_t arg2 = tf->ecx;
    uint32_t arg3 = tf->edx;

    switch (num) {
        case SYS_PRINTF: {
            // CPU:pushf/popfEFLAGS.AC
            const char *user_fmt = (const char*)arg1;
            char kbuf[512];
            int i = 0;

            // ,SMAP
            for (i = 0; i < 511; i++) {
                char c;
                // pushf/popfEFLAGS.AC
                __asm__ volatile (
                    "pushfl\n"                    // EFLAGS
                    "orl $0x40000, (%%esp)\n"    // AC(bit 18)
                    "popfl\n"                     // EFLAGS(AC=1)

                    "movb (%1), %0\n"             // 

                    "pushfl\n"
                    "andl $~0x40000, (%%esp)\n"  // AC
                    "popfl\n"                     // EFLAGS

                    : "=&r"(c)
                    : "r"(user_fmt + i)
                    : "memory", "cc"
                );
                if (c == '\0') break;
                kbuf[i] = c;
            }
            kbuf[i] = '\0';

            //   vga_putc 
            for (int j = 0; j < i; j++) {
                vga_putc(kbuf[j]);
            }
            tf->eax = i;  // 
            break;
        }
        case SYS_EXIT:
            // 打印退出信息
            printf("[USER] exit() called with code=%d\n", arg1);
            do_exit(arg1);
            // do_exit() 
            // 
            tf->eax = 0;
            break;
        case SYS_YIELD: {
            // CPU,
            extern int need_resched;
            need_resched = 1;
            tf->eax = 0;
            break;
        }
        case SYS_GET_MEM_STATS: {
            // 
            extern uint32_t buddy_get_total_pages(void);
            extern uint32_t buddy_get_free_pages(void);
            extern uint32_t buddy_get_used_pages(void);

            struct mem_stats {
                uint32_t total_pages;
                uint32_t free_pages;
                uint32_t used_pages;
            } *stats = (struct mem_stats*)arg1;

            if (stats) {
                stats->total_pages = buddy_get_total_pages();
                stats->free_pages = buddy_get_free_pages();
                stats->used_pages = buddy_get_used_pages();
                tf->eax = 0;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_READ_MEM: {
            // 
            uint32_t addr = arg1;
            uint32_t *value = (uint32_t*)arg2;

            if (value && addr >= 0xC0000000) {
                *value = *(uint32_t*)addr;
                tf->eax = 0;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_GETCHAR: {
            // 
            extern int keyboard_getchar(void);
            int c = keyboard_getchar();
            tf->eax = c;
            break;
        }
        case SYS_PUTCHAR: {
            // (EBX)
            uint8_t ch = (uint8_t)(arg1 & 0xFF);
            extern void vga_putc(char);
            vga_putc((char)ch);
            tf->eax = 0;
            break;
        }
        case SYS_GET_FRAMEBUFFER: {
            //  framebuffer 
            struct framebuffer_info {
                uint32_t addr;
                uint32_t width;
                uint32_t height;
                uint32_t pitch;
                uint8_t bpp;
            } *fb = (struct framebuffer_info*)arg1;

            if (fb && multiboot2_info_addr) {
                //  multiboot2  framebuffer 
                // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
                uint32_t *mb_info_ptr = (uint32_t *)phys_to_virt(multiboot2_info_addr);
                multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);
                while (tag->type != MULTIBOOT_TAG_TYPE_END) {
                    if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                        multiboot_tag_framebuffer_t *fb_info = (multiboot_tag_framebuffer_t *)tag;
                        fb->addr = (uint32_t)(fb_info->framebuffer_addr & 0xFFFFFFFF);
                        fb->width = fb_info->framebuffer_width;
                        fb->height = fb_info->framebuffer_height;
                        fb->pitch = fb_info->framebuffer_pitch;
                        fb->bpp = fb_info->framebuffer_bpp;
                        tf->eax = 0;
                        break;
                    }
                    tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
                }
                if (tag->type == MULTIBOOT_TAG_TYPE_END) {
                    tf->eax = -1;
                }
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_GETCWD: {
            // getcwd(buf, size) - 
            char *buf = (char*)arg1;
            uint32_t size = arg2;

            if (buf && size >= 2) {
                // 
                buf[0] = '/';
                buf[1] = '\0';
                tf->eax = 1;  // null
            } else {
                tf->eax = -1;  // NULL
            }
            break;
        }
        case SYS_WRITE: {
            // write(fd, buf, len) - arg1=fd, arg2=buf, arg3=len
            int fd = (int)arg1;
            const char *user_buf = (const char*)arg2;
            uint32_t len = arg3;

            static int write_count = 0;
            write_count++;

            if (fd == 1 && len < 512) {  // stdout
                //
                char kbuf[512];
                int copied = 0;

                // SMAP
                for (uint32_t i = 0; i < len; i++) {
                    char c;
                    __asm__ volatile (
                        "pushfl\n"                    // EFLAGS
                        "orl $0x40000, (%%esp)\n"    // AC
                        "popfl\n"

                        "movb (%1), %0\n"             //

                        "pushfl\n"
                        "andl $~0x40000, (%%esp)\n"  // AC
                        "popfl\n"

                        : "=&r"(c)
                        : "r"(user_buf + i)
                        : "memory", "cc"
                    );
                    kbuf[i] = c;
                    copied++;
                }

                // 输出到串口，添加前缀和计数器
                kbuf[copied] = '\0';
                printf("[USER #%d] %s", write_count, kbuf);
                tf->eax = copied;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_FORK: {
            // fork()  - 
            // PID0
            extern task_t* do_fork(void);
            task_t *child = do_fork();
            if (child) {
                //  PID
                tf->eax = child->pid;
                //   printf ES 
                // printf("[fork] Parent PID=%d, Child PID=%d\n", current_task[logical_cpu_id()]->pid, child->pid);
            } else {
                //  0
                tf->eax = 0;
            }
            break;
        }
        case SYS_OPEN: {
            // open(pathname, flags)
            const char *pathname = (const char*)arg1;
            int flags = (int)arg2;

            printf("[syscall] SYS_OPEN: pathname=0x%x, flags=%d\n", (uint32_t)pathname, flags);

            // 
            // 
            char kpath[256];
            int i = 0;

            printf("[syscall] Reading from user address 0x%x\n", (uint32_t)pathname);

            //  get_physical_address 
            extern uint32_t kernel_page_directory_phys;
            uint32_t *pd_virt = (uint32_t*)phys_to_virt(kernel_page_directory_phys);

            // 
            printf("[syscall] DEBUG: pathname=0x%x, 0xC0000000=%d\n",
                   (uint32_t)pathname, ((uint32_t)pathname < 0xC0000000));
            if ((uint32_t)pathname < 0xC0000000) {
                printf("[syscall] User space address, attempting page table walk...\n");

                // 
                uint32_t str_virt = (uint32_t)pathname;
                uint32_t pd_idx = (str_virt >> 22) & 0x3FF;
                uint32_t pt_idx = (str_virt >> 12) & 0x3FF;
                uint32_t page_offset = str_virt & 0xFFF;

                printf("[syscall] pd_idx=%d, pt_idx=%d, offset=0x%x\n", pd_idx, pt_idx, page_offset);

                //  PDE
                uint32_t pde_entry = pd_virt[pd_idx];
                printf("[syscall] pde_entry=0x%x\n", pde_entry);

                if (!(pde_entry & 0x1)) {
                    printf("[syscall] ERROR: PDE not present!\n");
                    kpath[0] = '\0';
                } else {
                    // 
                    uint32_t pt_phys = pde_entry & ~0xFFF;

                    printf("[syscall] Page table at phys=0x%x\n", pt_phys);

                    //  map_highmem_physical 
                    extern void* map_highmem_physical(uint32_t phys_addr, uint32_t size, uint32_t flags);
                    uint32_t *pt_virt = (uint32_t*)map_highmem_physical(pt_phys, 4096, 0);

                    if (pt_virt != NULL) {
                        uint32_t pte = pt_virt[pt_idx];
                        printf("[syscall] pte=0x%x\n", pte);

                        if (pte & 0x1) {
                            uint32_t phys_page = pte & ~0xFFF;
                            printf("[syscall] phys_page=0x%x\n", phys_page);

                            //  map_highmem_physical 
                            uint8_t *user_page_virt = (uint8_t*)map_highmem_physical(phys_page, 4096, 0);

                            printf("[syscall] user_page_virt=0x%x\n", (uint32_t)user_page_virt);

                            if (user_page_virt != NULL) {
                                // page_offset
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset, user_page_virt[page_offset]);
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset+1, user_page_virt[page_offset+1]);
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset+2, user_page_virt[page_offset+2]);

                                // 16
                                printf("[syscall] Raw data: ");
                                for (int j = 0; j < 16; j++) {
                                    printf("%02x ", user_page_virt[page_offset + j]);
                                }
                                printf("\n");

                                // 
                                int i;
                                for (i = 0; i < 255; i++) {
                                    kpath[i] = user_page_virt[page_offset + i];
                                    if (kpath[i] == '\0') break;
                                }
                                kpath[i] = '\0';

                                printf("[syscall] Copied path: '%s' (len=%d)\n", kpath, i);
                                printf("[syscall] kpath[0]=0x%x ('%c'), kpath[1]=0x%x\n",
                                       (unsigned char)kpath[0], kpath[0] ? kpath[0] : '?',
                                       (unsigned char)kpath[1]);
                            } else {
                                printf("[syscall] ERROR: Failed to map user page!\n");
                                kpath[0] = '\0';
                            }
                        } else {
                            printf("[syscall] ERROR: PTE not present!\n");
                            kpath[0] = '\0';
                        }
                    } else {
                        printf("[syscall] ERROR: Failed to map page table!\n");
                        kpath[0] = '\0';
                    }
                }
            } else {
                printf("[syscall] Kernel space address, copying directly\n");
                // 
                for (i = 0; i < 255; i++) {
                    kpath[i] = pathname[i];
                    if (kpath[i] == '\0') break;
                }
                kpath[i] = '\0';
                printf("[syscall] Copied path: '%s' (len=%d)\n", kpath, i);
            }

            //  VFS 
            extern struct file *filp_open(const char *, int);
            struct file *file = filp_open(kpath, flags);
            if (file) {
                //   fd
                //  fd 
                tf->eax = (int)file;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_CLOSE: {
            // close(fd)
            int fd = (int)arg1;
            struct file *file = (struct file*)fd;

            //  VFS 
            extern int filp_close(struct file *);
            int ret = filp_close(file);
            tf->eax = ret;
            break;
        }
        case SYS_READ: {
            // read(fd, buf, len)
            int fd = (int)arg1;
            char *user_buf = (char*)arg2;
            uint32_t len = arg3;
            struct file *file = (struct file*)fd;

            //  VFS 
            extern int filp_read(struct file *, char *, uint32_t);
            char kbuf[512];
            uint32_t to_read = (len < 512) ? len : 512;
            int ret = filp_read(file, kbuf, to_read);

            if (ret > 0) {
                // 
                for (int i = 0; i < ret; i++) {
                    char c = kbuf[i];
                    __asm__ volatile (
                        "pushfl\n"
                        "orl $0x40000, (%%esp)\n"
                        "popfl\n"
                        "movb %0, (%1)\n"
                        "pushfl\n"
                        "andl $~0x40000, (%%esp)\n"
                        "popfl\n"
                        :
                        : "r"(c), "r"(user_buf + i)
                        : "memory", "cc"
                    );
                }
            }
            tf->eax = ret;
            break;
        }
        case SYS_LSEEK: {
            // lseek(fd, offset, whence)
            int fd = (int)arg1;
            int offset = (int)arg2;
            int whence = (int)arg3;
            struct file *file = (struct file*)fd;

            //  VFS 
            extern int filp_lseek(struct file *, int64_t, int);
            int ret = filp_lseek(file, (int64_t)offset, whence);
            tf->eax = ret;
            break;
        }
        case SYS_NET_PING: {
            // net_ping(ip_addr, device)
            // arg1: IP 地址字符串
            // arg2: 设备名称（可选，NULL表示使用默认设备）
            const char *ip_str = (const char *)arg1;
            const char *dev_name = (const char *)arg2;

            // 临时覆盖 current_net_device（如果提供了设备名）
            char old_device[32] = {0};
            if (dev_name != NULL && dev_name[0] != '\0') {
                // 保存旧设备名
                strncpy(old_device, current_net_device, sizeof(old_device) - 1);
                // 设置新设备名
                strncpy(current_net_device, dev_name, sizeof(current_net_device) - 1);
                printf("[syscall] Temporarily setting device to: %s\n", current_net_device);
            }

            // 解析 IP  (a.b.c.d)
            uint32_t ip = 0;
            int parts[4];
            int part_count = 0;
            const char *p = ip_str;
            int current = 0;

            while (*p && part_count < 4) {
                if (*p == '.') {
                    parts[part_count++] = current;
                    current = 0;
                    p++;
                } else if (*p >= '0' && *p <= '9') {
                    current = current * 10 + (*p - '0');
                    p++;
                } else {
                    break;
                }
            }
            parts[part_count] = current;

            if (part_count == 3) {
                // 组装 32 位 IP（主机字节序）
                ip = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];

                // 🔍 调试：打印解析出的 IP
                printf("[syscall] Parsed IP: 0x%x (%d.%d.%d.%d)\n", ip,
                       parts[0], parts[1], parts[2], parts[3]);

                //  ping
                // 🔥 使用与 UDP 相同的设备选择逻辑
                extern net_device_t *net_device_get_default(void);
                extern int net_get_device_count(void);
                extern net_device_t **net_get_all_devices(void);

                net_device_t *dev = NULL;
                int count = net_get_device_count();
                net_device_t **devices = net_get_all_devices();

                // 如果用户指定了网卡，查找指定的网卡
                if (current_net_device[0] != '\0') {
                    printf("[syscall] Looking for device: %s\n", current_net_device);
                    for (int i = 0; i < count; i++) {
                        if (devices[i] && strcmp(devices[i]->name, current_net_device) == 0) {
                            dev = devices[i];
                            printf("[syscall] Using specified device: %s\n", dev->name);
                            break;
                        }
                    }
                    if (!dev) {
                        printf("[syscall] ERROR: Device '%s' not found\n", current_net_device);
                        tf->eax = -3;
                        break;
                    }
                } else {
                    // 自动选择：查找第一个非loopback设备（以太网设备）
                    for (int i = 0; i < count; i++) {
                        if (devices[i] && devices[i]->send != NULL) {
                            // 检查设备名称，跳过loopback
                            if (strcmp(devices[i]->name, "lo") != 0) {
                                dev = devices[i];
                                printf("[syscall] Auto-selected device: %s\n", dev->name);
                                break;
                            }
                        }
                    }
                }

                if (!dev) {
                    printf("[syscall] No network device available\n");
                    tf->eax = -1;
                    break;
                }

                // icmp_send_echo 已在 net.h 中声明
                //  发送 4 个 ping 包
                int i;
                for (i = 0; i < 4; i++) {
                    icmp_send_echo(dev, ip, 0x1234, i + 1);
                }

                // 恢复旧的设备名（如果之前保存了）
                if (old_device[0] != '\0') {
                    strncpy(current_net_device, old_device, sizeof(current_net_device) - 1);
                    printf("[syscall] Restored device to: %s\n", current_net_device);
                }

                tf->eax = 0;  // 成功
            } else {
                // 恢复旧的设备名（即使IP无效也要恢复）
                if (old_device[0] != '\0') {
                    strncpy(current_net_device, old_device, sizeof(current_net_device) - 1);
                    printf("[syscall] Restored device to: %s\n", current_net_device);
                }
                tf->eax = -2;  // IP 地址无效
            }
            break;
        }
        case SYS_NET_IFCONFIG: {
            // net_ifconfig() - 显示网卡接口配置
            // 🔥 修复：显示所有注册的网络设备，而不只是第一个
            extern int net_get_device_count(void);
            extern net_device_t *net_device_get_default();

            int count = net_get_device_count();
            printf("\n=== Network Interface Configuration ===\n");
            printf("Total devices: %d\n\n", count);

            if (count == 0) {
                printf("No network device found\n");
                tf->eax = -1;
                break;
            }

            // 显示所有设备
            extern net_device_t **net_get_all_devices(void);
            net_device_t **devices = net_get_all_devices();

            for (int i = 0; i < count && devices[i]; i++) {
                net_device_t *dev = devices[i];

                printf("--- Device %d ---\n", i);
                printf("Name:       %s\n", dev->name);

                // MAC 地址
                printf("MAC:        %02x:%02x:%02x:%02x:%02x:%02x\n",
                       dev->mac_addr[0], dev->mac_addr[1],
                       dev->mac_addr[2], dev->mac_addr[3],
                       dev->mac_addr[4], dev->mac_addr[5]);

                // IP 地址
                printf("IP:         %d.%d.%d.%d\n",
                       (dev->ip_addr >> 24) & 0xFF,
                       (dev->ip_addr >> 16) & 0xFF,
                       (dev->ip_addr >> 8) & 0xFF,
                       dev->ip_addr & 0xFF);

                // 子网掩码
                printf("Netmask:    %d.%d.%d.%d\n",
                       (dev->netmask >> 24) & 0xFF,
                       (dev->netmask >> 16) & 0xFF,
                       (dev->netmask >> 8) & 0xFF,
                       dev->netmask & 0xFF);

                // MTU
                printf("MTU:        %d bytes\n", dev->mtu);

                // 状态
                printf("Status:     UP\n");

                // 设备类型
                printf("Type:       ");
                if (dev->name[0] == 'l' && dev->name[1] == 'o') {
                    printf("Loopback\n");
                } else if (dev->name[0] == 'e' && dev->name[1] == 't' && dev->name[2] == 'h') {
                    // 以太网设备，尝试从 PCI 获取更多信息
                    int eth_num = dev->name[3] - '0';
                    if (eth_num >= 0) {
                        // 遍历 PCI 设备，查找第 eth_num 个网络设备
                        pci_dev_t **pci_devices = pci_get_devices();
                        int net_count = 0;
                        int found = 0;

                        for (int j = 0; pci_devices[j] != NULL && !found; j++) {
                            pci_dev_t *pci = pci_devices[j];
                            if (pci->header.class == 0x02) {  // 网络设备
                                if (net_count == eth_num) {
                                    const char *vendor = pci_get_vendor_name(pci->header.vendor_id);
                                    const char *device = pci_get_device_name(pci->header.vendor_id, pci->header.device_id);
                                    if (vendor && device) {
                                        printf("%s %s\n", vendor, device);
                                        found = 1;
                                    }
                                }
                                net_count++;
                            }
                        }

                        if (!found) {
                            printf("Ethernet\n");
                        }
                    } else {
                        printf("Ethernet\n");
                    }
                } else {
                    printf("Unknown\n");
                }

                // 🔥 显示 E1000 的 IRQ 信息
                extern int e1000_irq;
                if (e1000_irq != -1) {
                    printf("IRQ:        %d\n", e1000_irq);
                }

                printf("\n");
            }

            tf->eax = 0;  // 成功
            break;
        }
        case SYS_WIFI_INIT: {
            // wifi_init() - WiFi 
            extern int atheros_init(void);
            int ret = atheros_init();
            tf->eax = ret;  // 0-1
            break;
        }
        case SYS_WIFI_SCAN: {
            // wifi_scan() - WiFi 
            extern int wifi_scan(void);
            int ret = wifi_scan();
            tf->eax = ret;  // 
            break;
        }
        case SYS_WIFI_CONNECT: {
            // wifi_connect(ssid, password) - WiFi 
            const char *ssid = (const char *)arg1;
            const char *password = (const char *)arg2;

            // 
            char kssid[32];
            char kpassword[64];

            int i;
            for (i = 0; i < 31 && ssid[i] != '\0'; i++) {
                __asm__ volatile (
                    "pushfl\n"
                    "orl $0x40000, (%%esp)\n"
                    "popfl\n"
                    "movb (%1), %0\n"
                    "pushfl\n"
                    "andl $~0x40000, (%%esp)\n"
                    "popfl\n"
                    : "=r"(kssid[i])
                    : "r"(&ssid[i])
                    : "memory", "cc"
                );
            }
            kssid[i] = '\0';

            for (i = 0; i < 63 && password[i] != '\0'; i++) {
                __asm__ volatile (
                    "pushfl\n"
                    "orl $0x40000, (%%esp)\n"
                    "popfl\n"
                    "movb (%1), %0\n"
                    "pushfl\n"
                    "andl $~0x40000, (%%esp)\n"
                    "popfl\n"
                    : "=r"(kpassword[i])
                    : "r"(&password[i])
                    : "memory", "cc"
                );
            }
            kpassword[i] = '\0';

            extern int wifi_connect(const char *, const char *);
            int ret = wifi_connect(kssid, kpassword);
            tf->eax = ret;
            break;
        }
        case SYS_WIFI_DISCONNECT: {
            // wifi_disconnect() - WiFi 
            extern int wifi_disconnect(void);
            int ret = wifi_disconnect();
            tf->eax = ret;
            break;
        }
        case SYS_WIFI_STATUS: {
            // wifi_status() - WiFi 
            extern void wifi_status(void);
            wifi_status();
            tf->eax = 0;
            break;
        }
        case SYS_WIFI_LOAD_FIRMWARE: {
            //  
            // arg1 = &struct user_buf ()

            struct user_buf {
                const void *ptr;
                uint32_t len;
            } ubuf;

            //  
            const struct user_buf *user_ubuf = (const struct user_buf *)arg1;

            //  memcpy 
            memcpy(&ubuf, user_ubuf, sizeof(ubuf));

            //  
            if (ubuf.len == 0 || ubuf.len > (2 * 1024 * 1024)) {  //  2MB
                tf->eax = -1;  // -EINVAL
                break;
            }

            //  
            uint8_t *fw_buffer = (uint8_t *)kmalloc(ubuf.len);
            if (!fw_buffer) {
                tf->eax = -2;  // -ENOMEM
                break;
            }

            //    memcpy 
            memcpy(fw_buffer, ubuf.ptr, ubuf.len);

            //   fw_buffer 
            extern uint32_t atheros_wifi_mem_base;
            extern int intel_fw_load_from_buffer(uint32_t mem_base, const uint8_t *fw_data, uint32_t fw_size);
            int ret = intel_fw_load_from_buffer(atheros_wifi_mem_base, fw_buffer, ubuf.len);

            tf->eax = ret;  // 0
            break;
        }
        // ==================== WiFi ====================
        case SYS_WIFI_FW_BEGIN: {
            // arg1 = uint32_t size
            uint32_t size = arg1;

            // 
            if (size == 0 || size > FW_MAX_SIZE) {
                tf->eax = -1;  // -EINVAL
                break;
            }

            // 
            if (fw_buf) {
                tf->eax = -2;  // -EBUSY
                break;
            }

            // 
            fw_buf = (uint8_t *)kmalloc(size);
            if (!fw_buf) {
                tf->eax = -3;  // -ENOMEM
                break;
            }

            fw_size = size;
            fw_received = 0;
            fw_checksum = 0;

            printf("[syscall] WiFi FW BEGIN: allocated %u bytes at 0x%x\n",
                   size, (uint32_t)fw_buf);

            tf->eax = 0;  // 
            break;
        }
        case SYS_WIFI_FW_CHUNK: {
            // arg1 = const void *ptr
            // arg2 = uint32_t len
            // arg3 = uint32_t offset

            const uint8_t *user_ptr = (const uint8_t *)arg1;
            uint32_t len = arg2;
            uint32_t offset = arg3;

            // 
            if (!fw_buf) {
                tf->eax = -1;  // -EINVAL
                break;
            }

            // 
            if (offset + len > fw_size) {
                printf("[syscall] WiFi FW CHUNK: offset=%u len=%u exceeds size=%u\n",
                       offset, len, fw_size);
                tf->eax = -1;
                break;
            }

            if (!user_ptr || len == 0 || len > FW_CHUNK_SIZE) {
                tf->eax = -1;
                break;
            }

            //  memcpy len  4KB
            memcpy(fw_buf + offset, user_ptr, len);

            //  checksum
            for (uint32_t i = 0; i < len; i++) {
                fw_checksum += fw_buf[offset + i];
            }

            fw_received += len;

            tf->eax = 0;  // 
            break;
        }
        case SYS_WIFI_FW_END: {
            // 

            // 
            if (!fw_buf) {
                tf->eax = -1;  // -EINVAL
                break;
            }

            // 
            if (fw_received != fw_size) {
                printf("[syscall] WiFi FW END: incomplete! received=%u expected=%u\n",
                       fw_received, fw_size);
                kfree(fw_buf);
                fw_buf = NULL;
                tf->eax = -1;
                break;
            }

            printf("[syscall] WiFi FW END: complete! size=%u checksum=0x%x\n",
                   fw_size, fw_checksum);

            //   magic
            if (fw_size < 4) {
                printf("[syscall] WiFi FW END: firmware too small!\n");
                kfree(fw_buf);
                fw_buf = NULL;
                tf->eax = -1;
                break;
            }

            // Intel  magic: 0x000000004
            //  size 

            //  WiFi 
            extern uint32_t atheros_wifi_mem_base;
            extern int intel_fw_load_from_buffer(uint32_t mem_base, const uint8_t *fw_data, uint32_t fw_size);
            int ret = intel_fw_load_from_buffer(atheros_wifi_mem_base, fw_buf, fw_size);

            // 
            kfree(fw_buf);
            fw_buf = NULL;

            tf->eax = ret;  // 
            break;
        }
        case SYS_EXECV: {
            // execv(path, argv) - 
            //  execv 
            // 
            const char *path = (const char *)arg1;
            char *const *argv = (char *const *)arg2;

            // 
            // 
            // 1.  ELF 
            // 2. 
            // 3. 
            // 4.  trapframe 

            tf->eax = -1;  //
            break;
        }
        case SYS_LSPCI: {
            // 🔥 lspci - 列出所有 PCI 设备（网络设备放最后）
            printf("\n=== PCI Device List ===\n\n");

            pci_dev_t **pci_devices = pci_get_devices();
            pci_dev_t *network_devices[16];  // 保存网络设备
            int net_count = 0;
            int total_count = 0;

            // 第一遍：收集所有设备
            for (int i = 0; pci_devices[i] != NULL; i++) {
                total_count++;
            }

            // 第二遍：先显示非网络设备，收集网络设备
            printf("[Non-Network Devices]\n");
            for (int i = 0; pci_devices[i] != NULL; i++) {
                pci_dev_t *pci = pci_devices[i];

                // 🔍 调试：打印完整的 class code 信息
                // PCI Class Code 位于 offset 0x08-0x0B
                // 直接读取配置空间的 dword
                uint32_t addr =
                    0x80000000 |
                    ((pci->bus_id & 0xFF) << 16) |
                    ((pci->dev_id & 0x1F) << 11) |
                    ((pci->fn_id & 0x7) << 8) |
                    0x08;  // offset 0x08

                outl(CONFIG_ADDRESS, addr);
                uint32_t raw_class_dword = inl(CONFIG_DATA);

                uint8_t base_class = pci->header.class;
                uint8_t subclass = pci->header.subclass;
                uint8_t prog_if = pci->header.prog_if;
                uint8_t revision = pci->header.revision_id;

                // 检查是否是网络设备 (Base Class = 0x02)
                // 尝试多种可能的字节序解释
                int is_network_v1 = (base_class == 0x02);  // 结构体中的 class 字段
                int is_network_v2 = ((raw_class_dword >> 24) == 0x02);  // 最高字节
                int is_network_v3 = ((raw_class_dword >> 16) == 0x02);  // 第三字节
                int is_network_v4 = ((raw_class_dword >> 8) == 0x02);   // 第二字节

                int is_network = is_network_v1 || is_network_v2 || is_network_v3 || is_network_v4;

                // 🔍 调试输出（只打印前几个设备）
                static int debug_shown = 0;
                if (!debug_shown && i < 3) {
                    printf("[DEBUG] Device[%d]: raw_dword=0x%08x rev=0x%02x class=0x%02x sub=0x%02x prog=0x%02x\n",
                           i, raw_class_dword, revision, base_class, subclass, prog_if);
                    printf("[DEBUG]   Network checks: v1=%d v2=%d v3=%d v4=%d final=%d\n",
                           is_network_v1, is_network_v2, is_network_v3, is_network_v4, is_network);
                    if (i == 2) debug_shown = 1;
                }

                // 如果是网络设备，保存起来稍后显示
                if (is_network) {
                    if (net_count < 16) {
                        network_devices[net_count++] = pci;
                    }
                    continue;
                }

                // 显示非网络设备
                const char *vendor = pci_get_vendor_name(pci->header.vendor_id);
                const char *device = pci_get_device_name(pci->header.vendor_id, pci->header.device_id);

                printf("  [%02d] %04x:%04x %s %s\n",
                       i,
                       pci->header.vendor_id,
                       pci->header.device_id,
                       vendor ? vendor : "Unknown",
                       device ? device : "Device");
                printf("       Class: 0x%02x, IRQ: %d\n",
                       pci->header.class,
                       pci->header.u.h00.interrupt_line);
            }

            // 最后显示网络设备
            if (net_count > 0) {
                printf("\n[Network Devices]\n");
                for (int i = 0; i < net_count; i++) {
                    pci_dev_t *pci = network_devices[i];
                    const char *vendor = pci_get_vendor_name(pci->header.vendor_id);
                    const char *device = pci_get_device_name(pci->header.vendor_id, pci->header.device_id);

                    printf("  [%02d] %04x:%04x %s %s\n",
                           i,
                           pci->header.vendor_id,
                       pci->header.device_id,
                           vendor ? vendor : "Unknown",
                           device ? device : "Device");
                    printf("       Class: 0x%02x (Network), IRQ: %d\n",
                           pci->header.class,
                           pci->header.u.h00.interrupt_line);

                    // 🔥 手动注册 E1000 中断处理函数（已移到 e1000_init_dev 中）
                    // if (pci->header.vendor_id == 0x8086 && pci->header.device_id == 0x1502) {
                    //     printf("[lspci] E1000 82579LM detected!\n");
                    //
                    //     // 🔥 使用 pci_read_config_dword 读取 IRQ（offset 0x3C）
                    //     extern uint32_t pci_read_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
                    //     uint32_t irq_value = pci_read_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, 0x3C);
                    //     uint8_t irq = irq_value & 0xFF;  // 取最低字节
                    //
                    //     printf("[lspci] E1000 IRQ from PCI (offset 0x3C): %d\n", irq);
                    //
                    //     // 🔥 如果 IRQ 为 0 或 0xFF，使用默认值 11
                    //     if (irq == 0 || irq == 0xFF) {
                    //         irq = 11;
                    //         printf("[lspci] IRQ not configured, using default: %d\n", irq);
                    //     }
                    //
                    //     // 注册中断处理函数到 IOAPIC
                    //     extern void ioapicenable(int irq, int cpu);
                    //     printf("[lspci] Registering IRQ %d to IOAPIC...\n", irq);
                    //     ioapicenable(irq, 0);
                    //     printf("[lspci] E1000 IRQ %d registered!\n", irq);
                    // }
                }
            }

            printf("\nTotal: %d PCI devices (%d network)\n", total_count, net_count);
            tf->eax = 0;
            break;
        }
        case SYS_NET_INIT_RTL8139: {
            // 🔥 初始化 RTL8139 网卡
            extern int rtl8139_init(void);
            int ret = rtl8139_init();
            tf->eax = ret;
            break;
        }
        case SYS_NET_INIT_E1000: {
            // 🔥 初始化 E1000 网卡
            // 参数: tf->ebx = 设备名称（如 "eth0", "eth1"）
            const char *dev_name_user = (const char *)tf->ebx;

            if (dev_name_user == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            // 🔥 将设备名称从用户空间复制到内核空间
            static char dev_name_kernel[16];
            copy_from_user(dev_name_kernel, dev_name_user, 16);
            dev_name_kernel[15] = '\0';  // 确保以 null 结尾

            printf("[syscall] E1000 init: device=%s\n", dev_name_kernel);

            extern int e1000_init(const char *dev_name);
            int ret = e1000_init(dev_name_kernel);
            tf->eax = ret;
            break;
        }
        case SYS_NET_SEND_UDP: {
            // 🔥 发送 UDP 包
            // 参数: tf->ebx = IP字符串指针, tf->ecx = 端口, tf->edx = 数据指针, tf->esi = 数据长度
            const char *ip_str = (const char *)tf->ebx;
            int port = (int)tf->ecx;
            const char *data = (const char *)tf->edx;
            int len = (int)tf->esi;

            printf("[syscall] Send UDP: ip_str='%s' (len=%d), port=%d, len=%d\n", ip_str, strlen(ip_str), port, len);

            // 1. 解析 IP 地址字符串为 32 位整数
            uint32_t dst_ip = 0;
            uint8_t octets[4];
            int octet_idx = 0;
            uint32_t current = 0;

            // 跳过前导空格
            const char *p = ip_str;
            while (*p == ' ') p++;

            for (; *p != '\0'; p++) {
                if (*p == '.') {
                    octets[octet_idx++] = (uint8_t)current;
                    current = 0;
                } else if (*p >= '0' && *p <= '9') {
                    current = current * 10 + (*p - '0');
                } else if (*p == ' ') {
                    // 遇到空格，停止解析
                    break;
                }
            }
            octets[octet_idx] = (uint8_t)current;

            dst_ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];

            printf("[syscall] Parsed IP: %d.%d.%d.%d -> 0x%08X\n",
                   octets[0], octets[1], octets[2], octets[3], dst_ip);

            // 2. 获取网络设备
            extern net_device_t *net_device_get_default(void);
            extern int net_get_device_count(void);
            extern net_device_t **net_get_all_devices(void);

            net_device_t *dev = NULL;
            int count = net_get_device_count();
            net_device_t **devices = net_get_all_devices();

            // 如果用户指定了网卡，查找指定的网卡
            if (current_net_device[0] != '\0') {
                printf("[syscall] Looking for device: %s\n", current_net_device);
                for (int i = 0; i < count; i++) {
                    if (devices[i] && strcmp(devices[i]->name, current_net_device) == 0) {
                        dev = devices[i];
                        printf("[syscall] Using specified device: %s\n", dev->name);
                        break;
                    }
                }
                if (!dev) {
                    printf("[syscall] ERROR: Device '%s' not found\n", current_net_device);
                    tf->eax = -3;
                    break;
                }
            } else {
                // 自动选择：查找第一个非loopback设备（以太网设备）
                for (int i = 0; i < count; i++) {
                    if (devices[i] && devices[i]->send != NULL) {
                        // 检查设备名称，跳过loopback
                        if (strcmp(devices[i]->name, "lo") != 0) {
                            dev = devices[i];
                            printf("[syscall] Auto-selected device: %s\n", dev->name);
                            break;
                        }
                    }
                }
            }

            if (!dev) {
                printf("[syscall] No network device available\n");
                tf->eax = -2;
                break;
            }

            // 3. 调用 UDP 输出函数
            extern int udp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
                                 uint16_t dst_port, uint8_t *data, uint32_t len);

            // 🔥 使用动态源端口
            // 策略：使用动态端口范围 (49152-65535)
            // 端口计算：基础端口 + (目标端口的哈希)
            // 这样同一目标端口会使用相同的源端口，便于 NAT 穿透
            static uint16_t udp_src_port_counter = 0;
            uint16_t src_port = 49152 + ((udp_src_port_counter++ + port) % 16384);

            printf("[syscall] Using src port=%d, dst port=%d\n", src_port, port);

            int ret = udp_output(dev, dst_ip, src_port, port, (uint8_t *)data, len);
            tf->eax = ret;
            break;
        }
        case SYS_NET_SET_DEVICE: {
            // 🔥 设置当前使用的网络设备
            // 参数: tf->ebx = 设备名称字符串指针
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL || strcmp(dev_name, "auto") == 0) {
                // 设置为空字符串表示自动选择
                current_net_device[0] = '\0';
                printf("[syscall] Device selection: auto\n");
                tf->eax = 0;
            } else {
                // 🔥 将设备名称复制到内核缓冲区（而不是保存用户空间指针）
                int i;
                for (i = 0; i < 15 && dev_name[i] != '\0'; i++) {
                    current_net_device[i] = dev_name[i];
                }
                current_net_device[i] = '\0';
                printf("[syscall] Device selection: %s (copied to kernel)\n", current_net_device);
                tf->eax = 0;
            }
            break;
        }
        case SYS_NET_POLL_RX: {
            // 🔥 轮询RX（通用接口）
            // TODO: 需要添加设备名称参数
            printf("[syscall] POLL_RX called (TODO: needs device parameter)\n");
            tf->eax = 0;
            break;
        }
        case SYS_NET_DUMP_REGS: {
            // 🔥 转储网卡寄存器状态
            // 参数: tf->ebx = 设备名称（如 "eth0", "eth1"）
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] Dumping registers for device: %s\n", dev_name);

            // 根据设备名称判断类型并调用相应的 dump 函数
            // 目前只支持 E1000（eth0, eth1 等）
            if (strncmp(dev_name, "eth", 3) == 0) {
                extern void e1000_dump_regs(void);
                e1000_dump_regs();
            } else {
                printf("[syscall] ERROR: Unsupported device type: %s\n", dev_name);
                tf->eax = -1;
                break;
            }

            tf->eax = 0;
            break;
        }
        case SYS_NET_ARP: {
            // 🔥 ARP 命令 - 显示/扫描 ARP 缓存
            // arg1 (ebx) = 设备名称
            // arg2 (ecx) = scan 标志 (1=扫描并更新, 0=仅显示)
            const char *dev_name = (const char *)tf->ebx;
            int scan = (int)tf->ecx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            // 查找设备
            extern net_device_t **net_get_all_devices(void);
            extern int net_get_device_count(void);
            net_device_t **devices = net_get_all_devices();
            int count = net_get_device_count();

            net_device_t *dev = NULL;
            for (int i = 0; i < count; i++) {
                if (devices[i] && strcmp(devices[i]->name, dev_name) == 0) {
                    dev = devices[i];
                    break;
                }
            }

            if (!dev) {
                printf("[syscall] ERROR: Device '%s' not found\n", dev_name);
                tf->eax = -1;
                break;
            }

            extern void arp_show_cache(net_device_t *dev, int scan);
            arp_show_cache(dev, scan);
            tf->eax = 0;
            break;
        }
        case SYS_NET_DUMP_RX_REGS: {
            // 🔥 转储 RX 寄存器（详细）
            // 参数: tf->ebx = 设备名称（如 "eth0", "eth1"）
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] Dumping RX registers for device: %s\n", dev_name);

            // 查找设备
            extern net_device_t **net_get_all_devices(void);
            extern int net_get_device_count(void);
            net_device_t **devices = net_get_all_devices();
            int count = net_get_device_count();

            net_device_t *dev = NULL;
            for (int i = 0; i < count; i++) {
                if (devices[i] && strcmp(devices[i]->name, dev_name) == 0) {
                    dev = devices[i];
                    break;
                }
            }

            if (!dev) {
                printf("[syscall] ERROR: Device '%s' not found\n", dev_name);
                tf->eax = -1;
                break;
            }

            // 🔥 调用 net_dump_rx_regs，它会显示统计信息和 ARP 表
            extern void net_dump_rx_regs(net_device_t *dev);
            net_dump_rx_regs(dev);

            tf->eax = 0;
            break;
        }
        case SYS_NET_IFUP: {
            // 🔥 启动网络接口
            // arg1 (ebx) = 设备名称字符串指针
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] IFUP: device=%s\n", dev_name);

            // 调用 e1000_ifup
            extern int e1000_ifup(const char *dev_name);
            int ret = e1000_ifup(dev_name);
            tf->eax = ret;
            break;
        }
        case SYS_MSI_TEST: {
            // 🔥 MSI 测试 - 手动触发 MSI 来验证中断路径
            extern void msi_test_full_path(void);
            msi_test_full_path();
            tf->eax = 0;
            break;
        }
        case SYS_NET_LOOPBACK_TEST: {
            // 🔥 E1000 硬件 loopback 测试 - 测试 TX/RX/DMA（轮询版本）
            extern int e1000_loopback_test(void);
            int ret = e1000_loopback_test();
            tf->eax = ret;
            break;
        }
        case SYS_NET_LOOPBACK_TEST_INT: {
            // 🔥 E1000 硬件 loopback 测试 - 测试 TX/RX/MSI/DMA（中断版本）
            extern int e1000_loopback_test_interrupt(void);
            int ret = e1000_loopback_test_interrupt();
            tf->eax = ret;
            break;
        }
        // ==================== GUI 系统调用 ====================
        case SYS_GUI_FB_INFO: {
            // 获取帧缓冲区信息
            // 参数：ebx = fb_info_t* (用户态指针)
            // 返回：eax = 0 成功，-1 失败

            // 使用 VBE 驱动获取真实的帧缓冲区信息
            extern int vbe_is_available(void);
            extern uint32_t vbe_get_framebuffer(void);
            extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
            extern uint8_t vbe_get_bpp(void);
            extern uint16_t vbe_get_pitch(void);

            // 定义帧缓冲区信息结构（必须与用户空间一致）
            struct fb_info {
                void *fb_addr;
                uint32_t width;
                uint32_t height;
                uint32_t pitch;
                uint32_t bpp;
            } info;

            // 检查 VBE 是否可用
            if (!vbe_is_available()) {
                printf("[GUI FB INFO] VBE not available\n");
                tf->eax = -1;
                break;
            }

            // 填充帧缓冲区信息
            uint16_t width, height;
            vbe_get_resolution(&width, &height);

            uint32_t fb_phys = vbe_get_framebuffer();
            info.fb_addr = (void *)0xF0000000;  // 返回固定的虚拟地址
            info.width = width;
            info.height = height;
            info.pitch = vbe_get_pitch();
            info.bpp = vbe_get_bpp();

            printf("[GUI FB INFO] fb_phys=%p, fb_virt=%p, %dx%d, pitch=%d, bpp=%d\n",
                   fb_phys, info.fb_addr, info.width, info.height, info.pitch, info.bpp);

            // 🔥 重要: 将framebuffer映射到用户地址空间
            extern void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags);

            uint32_t fb_virt = 0xF0000000;
            uint32_t fb_size = info.pitch * info.height;
            uint32_t num_pages = (fb_size + 4095) / 4096;

            // 获取当前任务的页目录物理地址
            extern task_t *current_task[];
            task_t *task = current_task[logical_cpu_id()];
            uint32_t user_pde_phys = (uint32_t)(task->cr3);

            printf("[GUI FB INFO] Mapping framebuffer to user space...\n");
            printf("[GUI FB INFO]   user_pde_phys = 0x%x\n", user_pde_phys);
            printf("[GUI FB INFO]   fb_virt = 0x%x, fb_phys = 0x%x\n", fb_virt, fb_phys);
            printf("[GUI FB INFO]   num_pages = %d\n", num_pages);

            // 逐页映射到用户地址空间
            for (uint32_t i = 0; i < num_pages; i++) {
                uint32_t phys = fb_phys + i * 4096;
                uint32_t virt = fb_virt + i * 4096;
                map_page(user_pde_phys, virt, phys, 0x7);  // USER | WRITE | PRESENT
            }

            printf("[GUI FB INFO] ✓ Framebuffer mapped to user space!\n");

            // 拷贝到用户空间
            struct fb_info *user_info = (struct fb_info *)tf->ebx;
            if (copy_to_user(user_info, &info, sizeof(info)) != 0) {
                tf->eax = -1;
                break;
            }

            tf->eax = 0;
            break;
        }
        case SYS_GUI_FB_BLIT: {
            // 位图传输到帧缓冲区
            // 参数：ebx = x, ecx = y, edx = width, esi = height, edi = data (用户态指针)
            // 返回：eax = 0 成功，-1 失败

            int x = (int)tf->ebx;
            int y = (int)tf->ecx;
            int width = (int)tf->edx;
            int height = (int)tf->esi;
            const uint16_t *data = (const uint16_t *)tf->edi;

            // 简单验证
            if (width <= 0 || height <= 0 || data == NULL) {
                tf->eax = -1;
                break;
            }

            // 使用 VBE 函数获取帧缓冲区信息
            extern int vbe_is_available(void);
            extern uint32_t vbe_get_framebuffer(void);
            extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
            extern uint16_t vbe_get_pitch(void);

            // 检查 VBE 是否可用
            if (!vbe_is_available()) {
                tf->eax = -1;
                break;
            }

            // 获取帧缓冲区信息
            uint32_t fb_addr = vbe_get_framebuffer();
            uint16_t fb_width, fb_height;
            vbe_get_resolution(&fb_width, &fb_height);
            uint16_t fb_pitch = vbe_get_pitch();

            // 计算目标地址
            uint16_t *fb = (uint16_t *)fb_addr;
            uint16_t *dst = fb + y * (fb_pitch / 2) + x;

            // 逐行拷贝（处理 pitch）
            for (int i = 0; i < height && (y + i) < fb_height; i++) {
                for (int j = 0; j < width && (x + j) < fb_width; j++) {
                    // 从用户空间读取像素数据
                    uint16_t pixel;
                    if (copy_from_user((char *)&pixel, (const char *)&data[i * width + j], 2) != 0) {
                        tf->eax = -1;
                        goto blit_done;
                    }
                    dst[i * (fb_pitch / 2) + j] = pixel;
                }
            }

            tf->eax = 0;
        blit_done:
            break;
        }
        case SYS_GUI_INPUT_READ: {
            // 读取输入设备事件（键盘或鼠标）
            // 参数：ebx = input_event_t* (用户态指针)
            //         ecx = 事件类型 (1=键盘, 2=鼠标)
            // 返回：eax = 1 有事件, 0 无事件

            static int call_count = 0;
            static int keyboard_call_count = 0;
            static int mouse_call_count = 0;
            call_count++;

            // 定义输入事件结构（必须与用户空间一致）
            struct input_event {
                uint32_t type;      // 1=键盘, 2=鼠标
                int x;             // 鼠标 X 或 键码
                int y;             // 鼠标 Y 或 保留
                uint32_t pressed;  // 按键状态或保留
            } event;

            // 检查请求的事件类型
            uint32_t event_type = tf->ecx;

            if (event_type == 1) {
                keyboard_call_count++;
                if (keyboard_call_count % 100 == 0) {
                    printf("[SYS_GUI_INPUT_READ] Keyboard call #%d\n", keyboard_call_count);
                }
            } else if (event_type == 2) {
                mouse_call_count++;
                if (mouse_call_count % 100 == 0) {
                    printf("[SYS_GUI_INPUT_READ] Mouse call #%d\n", mouse_call_count);
                }
            }

            if (event_type == 1) {
                // 键盘事件 - 使用非阻塞方式读取
                extern int keyboard_scancode_available(void);
                extern int keyboard_get_scancode_nonblock(void);

                if (keyboard_scancode_available()) {
                    int scancode = keyboard_get_scancode_nonblock();
                    event.type = 1;
                    event.x = scancode;
                    event.y = 0;
                    event.pressed = 1;

                    // 拷贝到用户空间
                    struct input_event *user_event = (struct input_event *)tf->ebx;
                    if (copy_to_user(user_event, &event, sizeof(event)) != 0) {
                        tf->eax = -1;
                        break;
                    }

                    // 打印键盘事件（每次都打印）
                    printf("[SYS_GUI_INPUT_READ] KEY: scancode=0x%x\n", scancode);

                    tf->eax = 1;  // 有事件
                } else {
                    tf->eax = 0;  // 无事件

                    // 每1000次打印一次"无事件"
                    if (keyboard_call_count % 1000 == 0) {
                        printf("[SYS_GUI_INPUT_READ] No keyboard event (call=%d)\n", keyboard_call_count);
                    }
                }
            } else if (event_type == 2) {
                // 🔥 方案B：鼠标事件 - 状态机 + 边沿触发
                extern int usb_mouse_get_count(void);
                extern int usb_mouse_read(int mouse_index, void *report);
                extern int usb_mouse_data_available(int mouse_index);

                // 🔥 鼠标状态机（保存上次返回给用户态的状态）
                static struct {
                    int last_returned_x;
                    int last_returned_y;
                    uint32_t last_returned_buttons;
                    int initialized;
                } mouse_event_state = {0};

                // 🔥 初始化（使用当前全局状态作为初始值）
                if (!mouse_event_state.initialized) {
                    mouse_event_state.last_returned_x = usb_mouse_x;
                    mouse_event_state.last_returned_y = usb_mouse_y;
                    mouse_event_state.last_returned_buttons = usb_mouse_buttons;
                    mouse_event_state.initialized = 1;
                }

                int new_data_from_usb = 0;

                // 🔥 尝试读取USB鼠标数据（非阻塞）
                if (usb_mouse_get_count() > 0) {
                    if (usb_mouse_data_available(0)) {
                        struct {
                            uint8_t buttons;
                            int8_t x;
                            int8_t y;
                        } mouse_report;

                        int bytes = usb_mouse_read(0, &mouse_report);
                        if (bytes > 0) {
                            // 有新USB数据，更新全局状态
                            int old_buttons = usb_mouse_buttons;

                            usb_mouse_x += mouse_report.x;
                            usb_mouse_y += mouse_report.y;

                            // 边界检查
                            if (usb_mouse_x < 0) usb_mouse_x = 0;
                            if (usb_mouse_y < 0) usb_mouse_y = 0;
                            if (usb_mouse_x > 1024) usb_mouse_x = 1024;
                            if (usb_mouse_y > 768) usb_mouse_y = 768;

                            usb_mouse_buttons = mouse_report.buttons;
                            new_data_from_usb = 1;

                            // 🔥 调试：打印USB原始数据
                            static int usb_read_count = 0;
                            if (++usb_read_count <= 5 || (old_buttons != usb_mouse_buttons)) {
                                printf("[SYS] USB RAW: btn=%d->%d x=%d y=%d\n",
                                       old_buttons, usb_mouse_buttons,
                                       mouse_report.x, mouse_report.y);
                            }
                        }
                    }
                }

                // 🔥 关键：比较当前全局状态与上次返回给用户的状态
                // 如果有差异，就返回事件（边沿触发）
                int state_changed =
                    (usb_mouse_x != mouse_event_state.last_returned_x ||
                     usb_mouse_y != mouse_event_state.last_returned_y ||
                     usb_mouse_buttons != mouse_event_state.last_returned_buttons);

                if (state_changed) {
                    // 🔥 返回当前状态给用户
                    event.type = 2;
                    event.x = usb_mouse_x;
                    event.y = usb_mouse_y;
                    event.pressed = usb_mouse_buttons;

                    // 🔥 更新"上次返回"的状态
                    mouse_event_state.last_returned_x = usb_mouse_x;
                    mouse_event_state.last_returned_y = usb_mouse_y;
                    mouse_event_state.last_returned_buttons = usb_mouse_buttons;

                    // 拷贝到用户空间
                    struct input_event *user_event = (struct input_event *)tf->ebx;
                    if (copy_to_user(user_event, &event, sizeof(event)) != 0) {
                        tf->eax = -1;
                        break;
                    }

                    // 打印鼠标事件
                    printf("[SYS_GUI_INPUT_READ] MOUSE: x=%d y=%d btn=%d (changed)\n",
                           usb_mouse_x, usb_mouse_y, usb_mouse_buttons);

                    tf->eax = 1;  // 有事件
                } else {
                    tf->eax = 0;  // 无事件

                    // 每1000次打印一次"无事件"
                    if (mouse_call_count % 1000 == 0) {
                        printf("[SYS_GUI_INPUT_READ] No mouse event (call=%d)\n", mouse_call_count);
                    }
                }
            } else {
                // 其他事件类型暂不支持
                tf->eax = -1;
            }
            break;
        }
        case SYS_USB_MOUSE_POLL: {
            // 轮询 USB 鼠标事件（用于非阻塞读取）
            // 参数：ebx = usb_mouse_report_t* (用户态指针)
            // 返回：eax = 1 有数据, 0 无数据, -1 错误

            extern int usb_mouse_get_count(void);
            extern int usb_mouse_read(int mouse_index, void *report);
            extern int usb_mouse_data_available(int mouse_index);

            static int poll_count = 0;
            poll_count++;

            if (usb_mouse_get_count() == 0) {
                tf->eax = -1;  // 没有鼠标
                if (poll_count % 100 == 0) {
                    printf("[SYS_USB_MOUSE_POLL] No mouse found (poll=%d)\n", poll_count);
                }
                break;
            }

            // 检查是否有数据
            int avail = usb_mouse_data_available(0);
            if (!avail) {
                tf->eax = 0;  // 无数据
                if (poll_count % 100 == 0) {
                    printf("[SYS_USB_MOUSE_POLL] No data available (poll=%d)\n", poll_count);
                }
                break;
            }

            // 读取鼠标数据
            struct {
                uint8_t buttons;
                int8_t x;
                int8_t y;
            } mouse_report;

            int bytes = usb_mouse_read(0, &mouse_report);
            if (bytes <= 0) {
                tf->eax = -1;
                printf("[SYS_USB_MOUSE_POLL] ERROR: usb_mouse_read returned %d\n", bytes);
                break;
            }

            // 拷贝到用户空间
            void *user_report = (void *)tf->ebx;
            if (copy_to_user(user_report, &mouse_report, sizeof(mouse_report)) != 0) {
                tf->eax = -1;
                break;
            }

            tf->eax = 1;  // 有数据
            break;
        }
        default:
            //   printf ES 
            // printf("[syscall] unknown num=%d\n", num);
            tf->eax = -1;
            break;
    }

    // CR3!
    // CR3,
    // Linux 0.11CR3
}

// ==================== 系统调用包装函数 ====================

/**
 * @brief syscall_net_ifconfig() - 获取网卡接口配置
 */
int syscall_net_ifconfig(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_NET_IFCONFIG)
        : "memory", "cc"
    );
    return ret;
}

/**
 * @brief syscall_lspci() - 列出所有 PCI 设备
 */
int syscall_lspci(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_LSPCI)
        : "memory", "cc"
    );
    return ret;
}

/**
 * @brief net_send_udp() - 发送 UDP 包
 */
int net_send_udp(const char *ip, int port, const char *data, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_NET_SEND_UDP), "b"(ip), "c"(port), "d"(data), "S"(len)
        : "memory", "cc"
    );
    return ret;
}
