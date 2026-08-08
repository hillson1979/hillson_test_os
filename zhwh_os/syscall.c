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
#include "font8x8.h"
#include "pci.h"
#include "x86/io.h"  // ???? ???????????????outl/inl ??????
#include "elf.h"
#include "string.h"
#include "fs.h"
#include "mm.h"
#include "usb_display.h"

#define SYS_USB_INIT 83
#define SYS_USB_LOG_SAVE 84
#define SYS_USB_STATUS 85
#define SYS_USB_LOG_DISPLAY 86

// PCI ???????????? I/O ??????
#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC

// ?????????????????????
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
//#define SYS_EXECV 41  // ????????????
#define SYS_NET_BIND 52       // ???? ?????? UDP ??????
#define SYS_NET_RECV_UDP 53    // ???? ?????? UDP ??????
#define UDP_RX_BUF_SIZE (200 * 1024)  // 200KB??????????????????????????????
#define SYS_LSPCI 42  // ???? ???????????????PCI ??????
#define SYS_NET_INIT_RTL8139 43  // ???? ?????????????????? RTL8139
#define SYS_NET_INIT_E1000 44   // ???? ?????????????????? E1000
#define SYS_NET_SEND_UDP 45     // ???? ???????????????UDP ???
#define SYS_NET_SET_DEVICE 46   // ???? ???????????????????????????
#define SYS_NET_POLL_RX 47      // ???? ??????RX???????????????
#define SYS_NET_DUMP_REGS 48     // ???? ???????????????????????????
#define SYS_NET_ARP 49           // ???? ARP ????????????????????? ARP ?????????
#define SYS_NET_DUMP_RX_REGS 50 // ???? ?????? RX ?????????????????????
#define SYS_NET_IFUP 51        // ???? ??????????????????
//#define SYS_NET_RAW_DUMP_RX_DESC 52  // ???? ???????????????
#define SYS_MSI_TEST 60        // ???? MSI ??????
#define SYS_NET_LOOPBACK_TEST 61  // ???? E1000 ?????? loopback ??????????????????
#define SYS_NET_LOOPBACK_TEST_INT 62  // ???? E1000 ?????? loopback ??????????????????

// GUI ????????????
#define SYS_GUI_FB_INFO 70      // ????????????????????????
#define SYS_GUI_FB_BLIT 71      // ???????????????????????????
#define SYS_GUI_INPUT_READ 72   // ????????????????????????
#define SYS_USB_MOUSE_POLL 73   // ?????? USB ????????????
#define SYS_USB_MOUSE_INFO 76
#define SYS_CHERRYUSB_MOUSE_READ 78   // ?????? USB ??????????????????

// WiFi
static uint8_t  *fw_buf      = NULL;
static uint32_t  fw_size     = 0;
static uint32_t  fw_received = 0;
static uint32_t  fw_checksum = 0;

// USB ????????????
static int usb_mouse_x = 512;
static int usb_mouse_y = 384;
static uint8_t usb_mouse_buttons = 0;
static int mouse_last_x = -1, mouse_last_y = -1, mouse_last_buttons = -1;

// ???? ?????????????????????????????????????????????????????????????????????
// ???? ?????????static?????????????????????????????????
char current_net_device[16] = {0};


#define FW_CHUNK_SIZE   4096
#define FW_MAX_SIZE     (2 * 1024 * 1024)  // 2MB???????????????????????????Intel 677KB + Atheros??????

// 
extern task_t *current_task[];

//
extern void *kmalloc(uint32_t size);
extern void kfree(void *ptr);
extern uint32_t kernel_page_directory_phys;
extern void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags);
struct trapframe *saved_desktop_tf = 0;  /* spawn JVM ???????????????????????? */
struct task_t *saved_desktop_task = 0;   /* ???????????????????????? */

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
int copy_from_user(void *dst_ptr, const void *src_ptr, uint32_t n) {
    char *dst = (char *)dst_ptr;
    const char *src = (const char *)src_ptr;
    // ????????????????????????????????????
    for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
    return 0;
}

/*  copy_to_user */
int copy_to_user(void *dst_ptr, const void *src_ptr, uint32_t n) {
    char *dst = (char *)dst_ptr;
    const char *src = (const char *)src_ptr;
    // ????????????????????????????????????
    for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
    return 0;
}

/* sys_write(fd, buf, len) */
static int sys_write(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1 && fd != 2) return -1;  // ?????????stdout(1)???stderr(2)

    // ???????????????
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

    // 2. user_stack ?????????????????????????????????, ????????????
    // (?????? PD ?????????????????????????????????
    task->user_stack = 0;

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

    // 7. spawn ??????????????????, ????????????
    extern struct trapframe *saved_desktop_tf;
    extern struct task_t *saved_desktop_task;
    if (saved_desktop_tf != 0 && saved_desktop_task != 0) {
        /* Persist the foreground AI/session log to the physical USB model disk.
         * Failure is non-fatal: desktop restoration must always continue. */
        extern void *console_get_buf(void);
        extern int console_get_len(void);
        int log_len = console_get_len();
        if (log_len > 65536) log_len = 65536;
        int log_ret = fat32_overwrite_file("/usb/USB_AI.LOG",
                                           (const char *)console_get_buf(),
                                           (uint32_t)log_len);
        printf("[do_exit] USB AI log save %s (%d bytes)\n",
               log_ret == 0 ? "OK" : "failed", log_len);

        printf("[do_exit] Restoring desktop pid=%d\n", saved_desktop_task->pid);
        struct trapframe *dtf = saved_desktop_tf;
        struct task_t *dtask = saved_desktop_task;
        saved_desktop_tf = 0;
        saved_desktop_task = 0;
        dtask->state = 1; // PS_RUNNING
        dtask->tf = dtf;
        current_task[logical_cpu_id()] = dtask;
        extern void task_to_user_mode_with_task(struct task_t*);
        task_to_user_mode_with_task(dtask);
    }

    // 8.
    need_resched = 1;

    // 9.
    // 
    task_t *next = task->next;
    if (next == task || next->state == PS_TERMNAT || next->state == PS_DESTROY) {
        printf("[do_exit] No more runnable tasks, halting...\n");
        extern void klog_save_to_ramfs(void);
        klog_save_to_ramfs();
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

// ???????????????????????????user/libuser.h ???????????????
#define SYS_PRINTF 1
#define SYS_EXIT 2
#define SYS_YIELD 3
#define SYS_GET_MEM_STATS 4
#define SYS_READ_MEM 5
#define SYS_GET_MEM_USAGE 6
#define SYS_GETCHAR 7
#define SYS_PUTCHAR 8
#define SYS_GET_FRAMEBUFFER 10
#define SYS_GETCWD 9
#define SYS_WRITE 11
#define SYS_FORK 12
#define SYS_OPEN 20
#define SYS_CLOSE 21
#define SYS_READ 22
#define SYS_LSEEK 23
#define SYS_SBRK 24
#define SYS_EXECV 41
#define SYS_SPAWN 63
#define SYS_LS_DISK 79  // list local disk directory

void syscall_dispatch(struct trapframe *tf) {
     
    // printf("[syscall_dispatch] ENTER: tf=%x\n", tf);
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

            // ?????????????????????
            extern void uart_putc(char c);
            for (int j = 0; j < i; j++) {
                uart_putc(kbuf[j]);
            }

            tf->eax = i;  //
            break;
        }
        case SYS_EXIT:
            // ??????????????????
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
            uint8_t ch = (uint8_t)(arg1 & 0xFF);
            extern void vga_putc(char);
            vga_putc((char)ch);
            extern void console_write_char(char);
            console_write_char((char)ch);
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

                // ??????????????????????????????????????????
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

            if (!file || !user_buf || len == 0) {
                tf->eax = (len == 0) ? 0 : -1;
                break;
            }

            /* Keep large I/O off the kernel stack. 64 KiB amortizes syscall
             * and USB/FAT traversal overhead while remaining reclaimable. */
            uint32_t to_read = (len < 64 * 1024) ? len : 64 * 1024;
            char *kbuf = (char *)kmalloc(to_read);
            if (!kbuf) {
                tf->eax = -1;
                break;
            }

            extern int filp_read(struct file *, char *, uint32_t);
            int ret = filp_read(file, kbuf, to_read);
            if (ret > 0 && copy_to_user(user_buf, kbuf, (uint32_t)ret) != 0)
                ret = -1;
            kfree(kbuf);
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
        case SYS_SBRK: {
            // sbrk(increment) ???extend user program break
            uint32_t increment = arg1;
            extern struct task_t *current_task[4];
            /* System calls run on the task selected for this logical CPU. */
            struct task_t *task = current_task[logical_cpu_id()];
            if (!task) { tf->eax = 0; break; }

            // Initialize brk ???start high to avoid desktop (0x8000000+~8MB)
            if (task->user_brk == 0)
                task->user_brk = 0x10000000;  // 256MB, above all loaded programs

            uint32_t old_brk = task->user_brk;
            if (increment == 0) { tf->eax = old_brk; break; }

            // Round up to page boundary
            uint32_t num_pages = (increment + 0xFFF) / 0x1000;
            if (num_pages == 0) num_pages = 1;
            uint32_t new_brk = old_brk + num_pages * 0x1000;
            if (new_brk < old_brk || new_brk >= 0xBF000000) {
                printf("[sbrk] rejected: old=0x%x pages=%u\n", old_brk, num_pages);
                tf->eax = 0;
                break;
            }

            /* The user heap is virtually contiguous but its physical pages do
             * not need to be. This lets AI grow without a huge buddy order. */
            extern void map_4k_page(uint32_t phys, uint32_t virt, uint32_t flags);
            int allocation_failed = 0;
            for (uint32_t i = 0; i < num_pages; i++) {
                uint32_t phys = pmm_alloc_page();
                if (!phys) {
                    printf("[sbrk] out of memory after %u/%u pages\n", i, num_pages);
                    allocation_failed = 1;
                    break;
                }
                uint32_t virt = old_brk + i * 0x1000;
                map_4k_page(phys, virt, 0x7);
                memset((void *)virt, 0, 0x1000);
            }
            if (allocation_failed) { tf->eax = 0; break; }
            task->user_brk = new_brk;
            printf("[sbrk] +%u bytes, %u noncontiguous pages, virt=0x%x -> 0x%x\n",
                   increment, num_pages, old_brk, task->user_brk);
            tf->eax = old_brk;
            break;
        }
        case SYS_NET_PING: {
            // net_ping(ip_addr, device)
            // arg1: IP ???????????????
            // arg2: ????????????????????????NULL???????????????????????????
            const char *ip_str = (const char *)arg1;
            const char *dev_name = (const char *)arg2;

            // ???????????? current_net_device??????????????????????????????
            char old_device[32] = {0};
            if (dev_name != NULL && dev_name[0] != '\0') {
                // ??????????????????
                strncpy(old_device, current_net_device, sizeof(old_device) - 1);
                // ??????????????????
                strncpy(current_net_device, dev_name, sizeof(current_net_device) - 1);
                printf("[syscall] Temporarily setting device to: %s\n", current_net_device);
            }

            // ?????? IP  (a.b.c.d)
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
                // ?????? 32 ???IP?????????????????????
                ip = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];

                // ???? ???????????????????????????IP
                printf("[syscall] Parsed IP: 0x%x (%d.%d.%d.%d)\n", ip,
                       parts[0], parts[1], parts[2], parts[3]);

                //  ping
                // ???? ?????????UDP ???????????????????????????
                extern net_device_t *net_device_get_default(void);
                extern int net_get_device_count(void);
                extern net_device_t **net_get_all_devices(void);

                net_device_t *dev = NULL;
                int count = net_get_device_count();
                net_device_t **devices = net_get_all_devices();

                // ???????????????????????????????????????????????????
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
                    // ?????????????????????????????????loopback???????????????????????????
                    for (int i = 0; i < count; i++) {
                        if (devices[i] && devices[i]->send != NULL) {
                            // ???????????????????????????loopback
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

                // icmp_send_echo ?????? net.h ?????????
                //  ??????4 ???ping ???
                int i;
                for (i = 0; i < 4; i++) {
                    icmp_send_echo(dev, ip, 0x1234, i + 1);
                }

                // ????????????????????????????????????????????????
                if (old_device[0] != '\0') {
                    strncpy(current_net_device, old_device, sizeof(current_net_device) - 1);
                    printf("[syscall] Restored device to: %s\n", current_net_device);
                }

                tf->eax = 0;  // ??????
            } else {
                // ??????????????????????????????IP?????????????????????
                if (old_device[0] != '\0') {
                    strncpy(current_net_device, old_device, sizeof(current_net_device) - 1);
                    printf("[syscall] Restored device to: %s\n", current_net_device);
                }
                tf->eax = -2;  // IP ????????????
            }
            break;
        }
        case SYS_NET_IFCONFIG: {
            // net_ifconfig() - ????????????????????????
            // ???? ??????????????????????????????????????????????????????????????????
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

            // ??????????????????
            extern net_device_t **net_get_all_devices(void);
            net_device_t **devices = net_get_all_devices();

            for (int i = 0; i < count && devices[i]; i++) {
                net_device_t *dev = devices[i];

                printf("--- Device %d ---\n", i);
                printf("Name:       %s\n", dev->name);

                // MAC ??????
                printf("MAC:        %02x:%02x:%02x:%02x:%02x:%02x\n",
                       dev->mac_addr[0], dev->mac_addr[1],
                       dev->mac_addr[2], dev->mac_addr[3],
                       dev->mac_addr[4], dev->mac_addr[5]);

                // IP ??????
                printf("IP:         %d.%d.%d.%d\n",
                       (dev->ip_addr >> 24) & 0xFF,
                       (dev->ip_addr >> 16) & 0xFF,
                       (dev->ip_addr >> 8) & 0xFF,
                       dev->ip_addr & 0xFF);

                // ????????????
                printf("Netmask:    %d.%d.%d.%d\n",
                       (dev->netmask >> 24) & 0xFF,
                       (dev->netmask >> 16) & 0xFF,
                       (dev->netmask >> 8) & 0xFF,
                       dev->netmask & 0xFF);

                // MTU
                printf("MTU:        %d bytes\n", dev->mtu);

                // ??????
                printf("Status:     UP\n");

                // ????????????
                printf("Type:       ");
                if (dev->name[0] == 'l' && dev->name[1] == 'o') {
                    printf("Loopback\n");
                } else if (dev->name[0] == 'e' && dev->name[1] == 't' && dev->name[2] == 'h') {
                    // ???????????????????????????PCI ??????????????????
                    int eth_num = dev->name[3] - '0';
                    if (eth_num >= 0) {
                        // ?????? PCI ?????????????????? eth_num ???????????????
                        pci_dev_t **pci_devices = pci_get_devices();
                        int net_count = 0;
                        int found = 0;

                        for (int j = 0; pci_devices[j] != NULL && !found; j++) {
                            pci_dev_t *pci = pci_devices[j];
                            if (pci->header.class == 0x02) {  // ????????????
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

                // ???? ?????? E1000 ???IRQ ??????
                extern int e1000_irq;
                if (e1000_irq != -1) {
                    printf("IRQ:        %d\n", e1000_irq);
                }

                printf("\n");
            }

            tf->eax = 0;  // ??????
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
            // execv(path, argv) ????????????ELF ?????????
            const char *path = (const char *)arg1;
            printf("[execv] path=0x%x\n", (uint32_t)path);

            // ---- 1. ???????????????????????????----
            char kpath[256];
            {
                uint32_t str_virt = (uint32_t)path;
                if (str_virt >= 0xC0000000) {
                    int i = 0;
                    while (i < 255 && path[i]) kpath[i] = path[i], i++;
                    kpath[i] = '\0';
                } else {
                    uint32_t *pd_virt = (uint32_t*)phys_to_virt(kernel_page_directory_phys);
                    uint32_t pd_idx = (str_virt >> 22) & 0x3FF;
                    uint32_t pt_idx = (str_virt >> 12) & 0x3FF;
                    uint32_t page_off = str_virt & 0xFFF;
                    uint32_t pde_entry = pd_virt[pd_idx];

                    if (!(pde_entry & 0x1)) { tf->eax = -1; break; }
                    uint32_t pt_phys = pde_entry & ~0xFFF;
                    uint32_t *pt_virt = (uint32_t*)map_highmem_physical(pt_phys, 4096, 0);
                    if (!pt_virt) { tf->eax = -1; break; }
                    uint32_t pte = pt_virt[pt_idx];
                    if (!(pte & 0x1)) { tf->eax = -1; break; }
                    uint32_t phys_page = pte & ~0xFFF;
                    uint8_t *user_page = (uint8_t*)map_highmem_physical(phys_page, 4096, 0);
                    if (!user_page) { tf->eax = -1; break; }
                    int i;
                    for (i = 0; i < 255; i++) {
                        kpath[i] = user_page[page_off + i];
                        if (!kpath[i]) break;
                    }
                    kpath[i] = '\0';
                }
            }
            printf("[execv] path='%s'\n", kpath);

            // ---- 2. ?????????????????????ELF ?????? ----
            struct file *fp = filp_open(kpath, 0);
            if (!fp) { printf("[execv] Cannot open '%s'\n", kpath); tf->eax = -1; break; }
            printf("[execv] File opened, inode=%d\n", fp->f_inode->i_ino);

            uint32_t file_size = (uint32_t)filp_lseek(fp, 0, 2);
            filp_lseek(fp, 0, 0);
            if (file_size < 52 || file_size > 16*1024*1024) {
                printf("[execv] Bad file size %u\n", file_size);
                filp_close(fp); tf->eax = -1; break;
            }

            uint8_t *elf_buf = (uint8_t*)kmalloc(file_size);
            if (!elf_buf) { filp_close(fp); tf->eax = -1; break; }

            // ???????????????????????????(fork ????????????????????? ????????????????????? PD)
            uint32_t map_target_pd = kernel_page_directory_phys;
            {
                task_t *ct = current_task[logical_cpu_id()];
                if (ct && ct->cr3) {
                    uint32_t c = (uint32_t)ct->cr3;
                    if (c >= 0x1000) map_target_pd = c & ~0xFFF;
                }
            }
            printf("[execv] Using PD: 0x%x\n", map_target_pd);

            // ?????? kmalloc ????????????kmalloc ?????? phys_to_virt ???????????????8MB
            // identity ????????????????????????????????????
            {
                uint32_t buf_phys = (uint32_t)elf_buf - 0xC0000000;
                for (uint32_t off = 0; off < file_size; off += 4096) {
                    map_page(map_target_pd,
                             (uint32_t)elf_buf + off,
                             buf_phys + off, 0x3);
                }
            }

            int total_read = 0;
            while (total_read < (int)file_size) {
                int n = filp_read(fp, (char*)elf_buf + total_read, file_size - total_read);
                if (n <= 0) break;
                total_read += n;
            }
            filp_close(fp);
            printf("[execv] Read %d bytes\n", total_read);

            if (total_read < 52) { kfree(elf_buf); tf->eax = -1; break; }

            // ---- 3. ?????? ELF ----
            Elf32_Ehdr *eh = (Elf32_Ehdr *)elf_buf;
            if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
                eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
                printf("[execv] Not a valid ELF\n");
                kfree(elf_buf); tf->eax = -1; break;
            }
            printf("[execv] ELF entry=0x%x phoff=%u phnum=%u\n",
                   eh->e_entry, eh->e_phoff, eh->e_phnum);

            // ---- 5. ?????? PT_LOAD ???----
            {
                Elf32_Phdr *ph = (Elf32_Phdr *)(elf_buf + eh->e_phoff);
                for (int i = 0; i < eh->e_phnum; i++, ph++) {
                    if (ph->p_type != 1) continue;
                    uint32_t va = ph->p_vaddr, memsz = ph->p_memsz;
                    uint32_t filesz = ph->p_filesz, file_off = ph->p_offset;
                    printf("[execv] PT_LOAD va=0x%x memsz=0x%x filesz=0x%x\n", va, memsz, filesz);

                    for (uint32_t off = 0; off < memsz; off += 4096) {
                        uint32_t dst_va = va + off;
                        uint32_t dst_pa = pmm_alloc_page();
                        if (!dst_pa) { kfree(elf_buf); tf->eax = -1; goto execv_done; }

                        uint8_t *dst_virt = (dst_pa >= 0x800000) ?
                            (uint8_t*)map_highmem_physical(dst_pa, 4096, 0x3) :
                            (uint8_t*)phys_to_virt(dst_pa);
                        memset(dst_virt, 0, 4096);

                        if (off < filesz) {
                            uint32_t copy_sz = (off + 4096 > filesz) ? (filesz - off) : 4096;
                            memcpy(dst_virt, elf_buf + file_off + off, copy_sz);
                        }
                        map_page(map_target_pd, dst_va, dst_pa, 0x007);
                    }
                }
                printf("[execv] ELF segments loaded\n");
            }

            // ---- 5. ???????????????(129 ??? 0xBFF7F000 - 0xBFFFFFFF) ---- (129 ??? 0xBFF7F000 - 0xBFFFFFFF) ----
            {
                for (int i = 0; i < 129; i++) {
                    uint32_t sp = pmm_alloc_page();
                    if (!sp) { kfree(elf_buf); tf->eax = -1; goto execv_done; }
                    uint32_t sva = 0xC0000000 - (i + 1) * 4096;
                    map_page(map_target_pd, sva, sp, 0x007);
                }
                printf("[execv] Stack mapped\n");
            }

            // ---- 6b. ?????? trapframe ----
            {
                tf->eip     = eh->e_entry;
                tf->cs      = 0x1B;
                tf->ds      = 0x23;
                tf->es      = 0x23;
                tf->fs      = 0x23;
                tf->gs      = 0x23;
                tf->ss      = 0x23;
                tf->eflags  = 0x00000202;
                tf->esp     = 0xC0000000 - 64;
                tf->ebp     = 0;
                tf->eax     = 0;

                task_t *ct = current_task[logical_cpu_id()];
                if (ct) ct->user_stack = (uint32_t*)0xC0000000;

                printf("[execv] Done: eip=0x%x esp=0x%x entry=0x%x\n",
                       tf->eip, tf->esp, eh->e_entry);
            }

            kfree(elf_buf);
            tf->eax = 0;

            execv_done:
            break;
        }
        case SYS_SPAWN: {
            // spawn(path, arg) ????????????????????????ELF
            const char *spath = (const char *)arg1;
            const char *sarg  = (const char *)arg2;
            printf("[spawn] path=0x%x arg=0x%x\n", (uint32_t)spath, (uint32_t)sarg);

            // 1. ????????????
            char skpath[256];
            if (!spath) { tf->eax = -1; break; }
            copy_from_user(skpath, spath, sizeof(skpath) - 1);
            skpath[sizeof(skpath) - 1] = 0;
            printf("[spawn] path='%s'\n", skpath);

            // 2. ?????? & ?????? ELF
            struct file *sfp = filp_open(skpath, 0);
            if (!sfp) { printf("[spawn] open failed\n"); tf->eax = -1; break; }
            uint32_t ssize = (uint32_t)filp_lseek(sfp, 0, 2); filp_lseek(sfp, 0, 0);
            if (ssize < 52 || ssize > 16*1024*1024) { filp_close(sfp); tf->eax = -1; break; }
            uint8_t *self = (uint8_t*)kmalloc(ssize);
            if (!self) { filp_close(sfp); tf->eax = -1; break; }
            // ?????? kmalloc ??????
            {
                uint32_t bp = (uint32_t)self - 0xC0000000;
                for (uint32_t o = 0; o < ssize; o += 4096)
                    map_page(kernel_page_directory_phys, (uint32_t)self + o, bp + o, 0x3);
            }
            int sr = 0; while (sr < (int)ssize) { int n = filp_read(sfp, (char*)self + sr, ssize - sr); if (n <= 0) break; sr += n; }
            filp_close(sfp);
            if (sr < 52) { kfree(self); tf->eax = -1; break; }

            // 3. ?????? ELF
            Elf32_Ehdr *seh = (Elf32_Ehdr *)self;
            if (seh->e_ident[0] != 0x7F || seh->e_ident[1] != 'E' || seh->e_ident[2] != 'L' || seh->e_ident[3] != 'F')
                { kfree(self); tf->eax = -1; break; }

            // 4. ?????????????????? JVM ?????? (PID=2), ???????????????
            extern task_t *current_task[];
            static task_t *jvm_task = 0;
            if (!jvm_task) {
                extern task_t *init_task(bool with_ustack);
                jvm_task = init_task(true);
            }
            task_t *newt = jvm_task;
            if (!newt) { tf->eax = -1; break; }
            /* ?????????????????? ???????????? */

            // 5. ?????? PT_LOAD ????????? PD (JVM ?????????0xA0000000, ?????????
            Elf32_Phdr *sph = (Elf32_Phdr *)(self + seh->e_phoff);
            for (int i = 0; i < seh->e_phnum; i++, sph++) {
                if (sph->p_type != 1) continue;
                for (uint32_t o = 0; o < sph->p_memsz; o += 4096) {
                    uint32_t dva = sph->p_vaddr + o;
                    uint32_t dpa = pmm_alloc_page();
                    if (!dpa) { kfree(self); tf->eax = -1; goto spawn_done; }
                    uint8_t *dv = (dpa >= 0x800000) ? (uint8_t*)map_highmem_physical(dpa, 4096, 0x3)
                                                    : (uint8_t*)phys_to_virt(dpa);
                    memset(dv, 0, 4096);
                    if (o < sph->p_filesz) {
                        uint32_t cs = (o + 4096 > sph->p_filesz) ? (sph->p_filesz - o) : 4096;
                        memcpy(dv, self + sph->p_offset + o, cs);
                    }
                    map_page(kernel_page_directory_phys, dva, dpa, 0x007);
                }
            }

            // 6. ?????????(???????????? 0xBF800000, ???????????????0xC0000000)
            for (int i = 0; i < 16; i++) {
                uint32_t stp = pmm_alloc_page();
                if (!stp) { kfree(self); tf->eax = -1; goto spawn_done; }
                map_page(kernel_page_directory_phys, 0xBF800000 - (i + 1) * 4096, stp, 0x007);
            }

            // 6b. ????????????????????????
            uint32_t user_esp = 0xBF800000 - 64;
            {
                // ??????????????????????????????
                char argbuf[128]; int alen = 0;
                if (sarg)
                    copy_from_user(argbuf, sarg, sizeof(argbuf) - 1);
                else
                    argbuf[0] = 0;
                argbuf[sizeof(argbuf) - 1] = 0;
                if (argbuf[0] == 0) {
                    argbuf[0] = 'H';
                    argbuf[1] = 'e';
                    argbuf[2] = 'l';
                    argbuf[3] = 'l';
                    argbuf[4] = 'o';
                    argbuf[5] = 0;
                }
                alen = 0;
                while (alen < 127 && argbuf[alen]) alen++;
                // ??????: [string\0][padding][NULL][ptr_to_string][1(argc)]
                uint32_t str_va = user_esp - ((alen + 1 + 3) / 4 * 4);
                uint32_t argv_va = str_va - 8;
                user_esp = argv_va - 12;
                // ?????????????????????
                for (int i = 0; i <= alen; i++) {
                    ((char*)str_va)[i] = argbuf[i];
                    // ????????????????????????
                }
                // ?????? argv ???argc
                *(uint32_t*)(argv_va + 0) = str_va;
                *(uint32_t*)(argv_va + 4) = 0;
                *(uint32_t*)(user_esp + 0) = 0;
                *(uint32_t*)(user_esp + 4) = 1;
                *(uint32_t*)(user_esp + 8) = argv_va;
                printf("[spawn] args: '%s' argv=0x%x argv0=0x%x esp=0x%x\n",
                       argbuf, argv_va, str_va, user_esp);
            }

            // 7. ?????? trapframe
            newt->tf->eip = seh->e_entry;
            newt->tf->cs = 0x1B; newt->tf->ds = 0x23; newt->tf->es = 0x23;
            newt->tf->fs = 0x23; newt->tf->gs = 0x23; newt->tf->ss = 0x23;
            newt->tf->eflags = 0x202;
            newt->tf->esp = user_esp;
            newt->tf->eax = 0; newt->tf->ebp = 0;
            newt->pde = (uint32_t*)kernel_page_directory_phys;
            newt->cr3 = (uint32_t*)kernel_page_directory_phys;
            newt->user_stack = (uint32_t*)1;  /* ????????????????????? ?????????*/

            // 8. ???????????????
            /* ELF ??????????????? kfree ?????????????????????????????? spawn ?????? */
            tf->eax = (int)newt->pid;
            extern struct trapframe *saved_desktop_tf;
            extern struct task_t *saved_desktop_task;
            saved_desktop_tf = tf;
            saved_desktop_task = current_task[logical_cpu_id()];
            extern void task_to_user_mode_with_task_wrapper(struct task_t*);
            task_to_user_mode_with_task_wrapper(newt);
            /* ?????????*/
            spawn_done:
            break;
        }
        case SYS_LSPCI: {
            // ???? lspci - ????????????PCI ?????????????????????????????????
            printf("\n=== PCI Device List ===\n\n");

            pci_dev_t **pci_devices = pci_get_devices();
            pci_dev_t *network_devices[16];  // ??????????????????
            int net_count = 0;
            int total_count = 0;

            // ??????????????????????????????
            for (int i = 0; pci_devices[i] != NULL; i++) {
                total_count++;
            }

            // ?????????????????????????????????????????????????????????
            printf("[Non-Network Devices]\n");
            for (int i = 0; pci_devices[i] != NULL; i++) {
                pci_dev_t *pci = pci_devices[i];

                // ???? ???????????????????????? class code ??????
                // PCI Class Code ?????? offset 0x08-0x0B
                // ???????????????????????????dword
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

                // ??????????????????????????? (Base Class = 0x02)
                // ????????????????????????????????????
                int is_network_v1 = (base_class == 0x02);  // ???????????????class ??????
                int is_network_v2 = ((raw_class_dword >> 24) == 0x02);  // ????????????
                int is_network_v3 = ((raw_class_dword >> 16) == 0x02);  // ????????????
                int is_network_v4 = ((raw_class_dword >> 8) == 0x02);   // ????????????

                int is_network = is_network_v1 || is_network_v2 || is_network_v3 || is_network_v4;

                // ???? ??????????????????????????????????????????
                static int debug_shown = 0;
                if (!debug_shown && i < 3) {
                    printf("[DEBUG] Device[%d]: raw_dword=0x%08x rev=0x%02x class=0x%02x sub=0x%02x prog=0x%02x\n",
                           i, raw_class_dword, revision, base_class, subclass, prog_if);
                    printf("[DEBUG]   Network checks: v1=%d v2=%d v3=%d v4=%d final=%d\n",
                           is_network_v1, is_network_v2, is_network_v3, is_network_v4, is_network);
                    if (i == 2) debug_shown = 1;
                }

                // ????????????????????????????????????????????????
                if (is_network) {
                    if (net_count < 16) {
                        network_devices[net_count++] = pci;
                    }
                    continue;
                }

                // ?????????????????????
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

            // ????????????????????????
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

                    // ???? ???????????? E1000 ?????????????????????????????? e1000_init_dev ??????
                    // if (pci->header.vendor_id == 0x8086 && pci->header.device_id == 0x1502) {
                    //     printf("[lspci] E1000 82579LM detected!\n");
                    //
                    //     // ???? ?????? pci_read_config_dword ?????? IRQ???offset 0x3C???
                    //     extern uint32_t pci_read_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
                    //     uint32_t irq_value = pci_read_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, 0x3C);
                    //     uint8_t irq = irq_value & 0xFF;  // ???????????????
                    //
                    //     printf("[lspci] E1000 IRQ from PCI (offset 0x3C): %d\n", irq);
                    //
                    //     // ???? ?????? IRQ ???0 ???0xFF??????????????????11
                    //     if (irq == 0 || irq == 0xFF) {
                    //         irq = 11;
                    //         printf("[lspci] IRQ not configured, using default: %d\n", irq);
                    //     }
                    //
                    //     // ???????????????????????????IOAPIC
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
            // ???? ?????????RTL8139 ??????
            extern int rtl8139_init(void);
            int ret = rtl8139_init();
            tf->eax = ret;
            break;
        }
        case SYS_NET_INIT_E1000: {
            // ???? ?????????E1000 ??????
            // ??????: tf->ebx = ?????????????????? "eth0", "eth1"???
            const char *dev_name_user = (const char *)tf->ebx;

            if (dev_name_user == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] E1000 init: device=%s\n", dev_name_user);

            extern int e1000_init(const char *dev_name);
            int ret = e1000_init(dev_name_user);
            tf->eax = ret;
            break;
        }
        case SYS_NET_SEND_UDP: {
            // ???? ??????UDP ???
            // ??????: tf->ebx = IP??????????????? tf->ecx = ??????, tf->edx = ????????????, tf->esi = ????????????
            const char *ip_str = (const char *)tf->ebx;
            int port = (int)tf->ecx;
            const char *data = (const char *)tf->edx;
            int len = (int)tf->esi;

            printf("[syscall] Send UDP: ip_str='%s' (len=%d), port=%d, len=%d\n", ip_str, strlen(ip_str), port, len);

            // 1. ?????? IP ?????????????????? 32 ?????????
            uint32_t dst_ip = 0;
            uint8_t octets[4];
            int octet_idx = 0;
            uint32_t current = 0;

            // ??????????????????
            const char *p = ip_str;
            while (*p == ' ') p++;

            for (; *p != '\0'; p++) {
                if (*p == '.') {
                    octets[octet_idx++] = (uint8_t)current;
                    current = 0;
                } else if (*p >= '0' && *p <= '9') {
                    current = current * 10 + (*p - '0');
                } else if (*p == ' ') {
                    // ???????????????????????????
                    break;
                }
            }
            octets[octet_idx] = (uint8_t)current;

            dst_ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];

            printf("[syscall] Parsed IP: %d.%d.%d.%d -> 0x%08X\n",
                   octets[0], octets[1], octets[2], octets[3], dst_ip);

            // 2. ??????????????????
            extern net_device_t *net_device_get_default(void);
            extern int net_get_device_count(void);
            extern net_device_t **net_get_all_devices(void);

            net_device_t *dev = NULL;
            int count = net_get_device_count();
            net_device_t **devices = net_get_all_devices();

            // ???????????????????????????????????????????????????
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
                // ?????????????????????????????????loopback???????????????????????????
                for (int i = 0; i < count; i++) {
                    if (devices[i] && devices[i]->send != NULL) {
                        // ???????????????????????????loopback
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

            // 3. ?????? UDP ????????????
            extern int udp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
                                 uint16_t dst_port, uint8_t *data, uint32_t len);

            // ???? ?????????????????????
            // ?????????????????????????????????(49152-65535)
            // ??????????????????????????? + (?????????????????????
            // ???????????????????????????????????????????????????????????? NAT ??????
            static uint16_t udp_src_port_counter = 0;
            uint16_t src_port = 49152 + ((udp_src_port_counter++ + port) % 16384);

            printf("[syscall] Using src port=%d, dst port=%d\n", src_port, port);

            int ret = udp_output(dev, dst_ip, src_port, port, (uint8_t *)data, len);
            tf->eax = ret;
            break;
        }
        case SYS_NET_SET_DEVICE: {
            // ???? ?????????????????????????????????
            // ??????: tf->ebx = ???????????????????????????
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL || strcmp(dev_name, "auto") == 0) {
                // ???????????????????????????????????????
                current_net_device[0] = '\0';
                printf("[syscall] Device selection: auto\n");
                tf->eax = 0;
            } else {
                // ???? ??????????????????????????????????????????????????????????????????????????????
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
            // ???? ??????RX??????????????????
            // ???????????????loopback??????
            extern net_device_t *net_device_get_default(void);
            extern int net_get_device_count(void);
            extern net_device_t **net_get_all_devices(void);

            net_device_t *dev = NULL;
            int count = net_get_device_count();
            net_device_t **devices = net_get_all_devices();

            // ?????????????????????????????????loopback???????????????????????????
            for (int i = 0; i < count; i++) {
                if (devices[i] && devices[i]->send != NULL) {
                    if (strcmp(devices[i]->name, "lo") != 0) {
                        dev = devices[i];
                        printf("[syscall] Polling on device: %s\n", dev->name);
                        break;
                    }
                }
            }

            if (dev) {
                // ???????????????????????????????????????e1000???
                extern void e1000_poll_rx(net_device_t *dev);
                e1000_poll_rx(dev);
            } else {
                printf("[syscall] No network device available\n");
            }

            tf->eax = 0;
            break;
        }
        case SYS_NET_DUMP_REGS: {
            // ???? ???????????????????????????
            // ??????: tf->ebx = ?????????????????? "eth0", "eth1"???
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] Dumping registers for device: %s\n", dev_name);

            // ???????????????????????????????????????????????? dump ??????
            // ???????????????E1000???eth0, eth1 ??????
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
            // ???? ARP ?????? - ??????/?????? ARP ??????
            // arg1 (ebx) = ????????????
            // arg2 (ecx) = scan ?????? (1=??????????????? 0=?????????
            const char *dev_name = (const char *)tf->ebx;
            int scan = (int)tf->ecx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            // ????????????
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
            // ???? ?????? RX ?????????????????????
            // ??????: tf->ebx = ?????????????????? "eth0", "eth1"???
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] Dumping RX registers for device: %s\n", dev_name);

            // ????????????
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

            // ???? ?????? net_dump_rx_regs?????????????????????????????? ARP ???
            extern void net_dump_rx_regs(net_device_t *dev);
            net_dump_rx_regs(dev);

            tf->eax = 0;
            break;
        }
        case SYS_NET_IFUP: {
            // ???? ??????????????????
            // arg1 (ebx) = ???????????????????????????
            const char *dev_name = (const char *)tf->ebx;

            if (dev_name == NULL) {
                printf("[syscall] ERROR: Device name is NULL\n");
                tf->eax = -1;
                break;
            }

            printf("[syscall] IFUP: device=%s\n", dev_name);

            // ?????? e1000_ifup
            extern int e1000_ifup(const char *dev_name);
            int ret = e1000_ifup(dev_name);
            tf->eax = ret;
            break;
        }
        case SYS_MSI_TEST: {
            // ???? MSI ?????? - ???????????? MSI ?????????????????????
            extern void msi_test_full_path(void);
            msi_test_full_path();
            tf->eax = 0;
            break;
        }
        case SYS_NET_LOOPBACK_TEST: {
            // ???? E1000 ?????? loopback ?????? - ?????? TX/RX/DMA??????????????????
            extern int e1000_loopback_test(void);
            int ret = e1000_loopback_test();
            tf->eax = ret;
            break;
        }
        case SYS_NET_LOOPBACK_TEST_INT: {
            // ???? E1000 ?????? loopback ?????? - ?????? TX/RX/MSI/DMA??????????????????
            extern int e1000_loopback_test_interrupt(void);
            int ret = e1000_loopback_test_interrupt();
            tf->eax = ret;
            break;
        }
        // ==================== GUI ???????????? ====================
        case SYS_GUI_FB_INFO: {
            // ????????????????????????
            // ?????????ebx = fb_info_t* (???????????????
            // ?????????eax = 0 ?????????1 ??????

            // ?????? VBE ???????????????????????????????????????
            extern int vbe_is_available(void);
            extern uint32_t vbe_get_framebuffer(void);
            extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
            extern uint8_t vbe_get_bpp(void);
            extern uint16_t vbe_get_pitch(void);

            // ???????????????????????????????????????????????????????????????
            struct fb_info {
                void *fb_addr;
                uint32_t width;
                uint32_t height;
                uint32_t pitch;
                uint32_t bpp;
            } info;

            // ??????VBE ????????????
            if (!vbe_is_available()) {
                printf("[GUI FB INFO] VBE not available\n");
                tf->eax = -1;
                break;
            }

            // ????????????????????????
            uint16_t width, height;
            vbe_get_resolution(&width, &height);

            uint32_t fb_phys = vbe_get_framebuffer();
            info.fb_addr = (void *)0xF0000000;  // ???????????????????????????
            info.width = width;
            info.height = height;
            info.pitch = vbe_get_pitch();
            info.bpp = vbe_get_bpp();

            printf("[GUI FB INFO] fb_phys=0x%x, fb_virt=0x%x, %d x %d, pitch=%d, bpp=%d\n",
                   fb_phys, info.fb_addr, info.width, info.height, info.pitch, info.bpp);

            // ???? ??????: ???????????????????????????????????????VBE ??????????????????
            // vbe.c ???vbe_init_from_multiboot ???????????????0xF0000000 ?????????????????????
            // ?????????????????????USER ??????????????????????????????????????????
            printf("[GUI FB INFO] Framebuffer already mapped at 0x%x (by VBE driver)\n", 0xF0000000);


            // ?????????????????????
            struct fb_info *user_info = (struct fb_info *)tf->ebx;
            if (copy_to_user(user_info, &info, sizeof(info)) != 0) {
                tf->eax = -1;
                break;
            }

            tf->eax = 0;
            break;
        }
        case SYS_GUI_FB_BLIT: {
            // ???????????????????????????
            // ?????????ebx = x, ecx = y, edx = width, esi = height, edi = data (???????????????
            // ?????????eax = 0 ?????????1 ??????

            int x = (int)tf->ebx;
            int y = (int)tf->ecx;
            int width = (int)tf->edx;
            int height = (int)tf->esi;
            const uint16_t *data = (const uint16_t *)tf->edi;

            // ????????????
            if (width <= 0 || height <= 0 || data == NULL) {
                tf->eax = -1;
                break;
            }

            // ?????? VBE ??????????????????????????????
            extern int vbe_is_available(void);
            extern uint32_t vbe_get_framebuffer(void);
            extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
            extern uint16_t vbe_get_pitch(void);

            // ??????VBE ????????????
            if (!vbe_is_available()) {
                tf->eax = -1;
                break;
            }

            // ????????????????????????
            uint32_t fb_addr = vbe_get_framebuffer();
            uint16_t fb_width, fb_height;
            vbe_get_resolution(&fb_width, &fb_height);
            uint16_t fb_pitch = vbe_get_pitch();

            // ??????????????????
            uint16_t *fb = (uint16_t *)fb_addr;
            uint16_t *dst = fb + y * (fb_pitch / 2) + x;

            // ?????????????????????pitch???
            for (int i = 0; i < height && (y + i) < fb_height; i++) {
                for (int j = 0; j < width && (x + j) < fb_width; j++) {
                    // ?????????????????????????????????
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

            // ?????????????????????????????????????????????            // ?????????ebx = input_event_t* (???????????????
            //         ecx = ???????????? (1=??????, 2=??????)
            // ?????????eax = 1 ????????? 0 ?????????

            static int mouse_call_count = 0;

            // ?????????????????????????????????????????????????????????
            struct input_event {
                uint32_t type;      // 1=??????, 2=??????
                int x;             // ?????? X ?????????
                int y;             // ?????? Y ?????????
                uint32_t pressed;  // ?????????????????????
            } event;

            // ???????????????????????????
            uint32_t event_type = tf->ecx;

            if (event_type == 1) {
                // ???????????? - ???????????????????????????
                extern int keyboard_scancode_available(void);
                extern int keyboard_get_scancode_nonblock(void);

                if (keyboard_scancode_available()) {
                    int scancode = keyboard_get_scancode_nonblock();
                    // F1 toggle: kernel debug overlay
                    static int f1_overlay_visible = 0;
                    if (scancode == 0x3B) {
                        if (f1_overlay_visible) {
                            // Toggle OFF: pass F1 through to userspace (editor redraws)
                            f1_overlay_visible = 0;
                            // fall through to normal event handling below
                        } else {
                            // Toggle ON: draw EHCI enumeration data overlay
                            f1_overlay_visible = 1;
                            volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
                            extern uint16_t vbe_get_pitch(void);
                            int pitch = vbe_get_pitch();
                            if (pitch > 0) {
                                int ppitch = pitch / 4;
                                // Clear screen
                                for (int dy = 0; dy < 768; dy++)
                                    for (int dx = 0; dx < 1024; dx++)
                                        fb[dy*ppitch + dx] = 0x00000000;
                                extern const unsigned char font8x8_data[95][8];
                                // === Draw USB Status ===
                                {
                                    extern int g_using_ehci, num_uhci_controllers;
                                    extern uint32_t g_ehci_qh_phys;
                                    printf("[F1] ehci=%d uhci=%d\n", g_using_ehci, num_uhci_controllers);
                                    extern int usb_mouse_get_count(void);
                                    char buf[256]; int p = 0;
                                    p = 0;
                                    { const char *t="HillsonOS USB\nUHCI="; while(*t&&p<250)buf[p++]=*t++;
                                      int v=num_uhci_controllers; if(!v)buf[p++]='0'; else{char d[8];int i=0;
                                      while(v){d[i++]='0'+v%10;v/=10;} while(i)buf[p++]=d[--i];}
                                      buf[p++]='\n'; }
                                    { const char *t="EHCI="; while(*t&&p<250)buf[p++]=*t++;
                                      int v=g_using_ehci?1:0; if(!v)buf[p++]='0'; else{char d[8];int i=0;
                                      while(v){d[i++]='0'+v%10;v/=10;} while(i)buf[p++]=d[--i];}
                                      buf[p++]='\n'; }
                                    { extern uint32_t g_ehci_cmd, g_ehci_sts;
                                      const char *t="EHCI cmd="; while(*t&&p<250)buf[p++]=*t++;
                                      unsigned v=g_ehci_cmd; char d[9];int i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(v&&i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="EHCI sts="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_sts; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(v&&i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      { extern unsigned g_ehci_eecp, g_ehci_legsup;
                                        const char *t="EECP="; while(*t&&p<250)buf[p++]=*t++;
                                        unsigned v=g_ehci_eecp; char d[9];int i=0;
                                        do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<2);
                                        while(i&&p<250)buf[p++]=d[--i];
                                        t=" LEG="; while(*t&&p<250)buf[p++]=*t++;
                                        v=g_ehci_legsup; i=0;
                                        do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                        while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n'; }
                                      t="FR="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_phys; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(v&&i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="CP="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_caps; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(v&&i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="C2="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_caps2; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(v&&i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="P="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_mouse_port; if(!v)buf[p++]='0'; else{char dd[4];int ii=0;
                                      while(v){dd[ii++]='0'+v%10;v/=10;} while(ii)buf[p++]=dd[--ii];}
                                      t=" own="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_port_owner; buf[p++]='0'+v; buf[p++]=' ';
                                      t="en="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_port_enabled; buf[p++]='0'+v; buf[p++]=' ';
                                      t="ls="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_port_speed; buf[p++]='0'+v;
                                      buf[p++]='\n'; }
                                    { extern unsigned g_ehci_portsc, g_ehci_qh_ovl,
                                        g_ehci_qh_nxt, g_ehci_qh_alt, g_ehci_td_nxt;
                                      const char *t="PSC="; while(*t&&p<250)buf[p++]=*t++;
                                      unsigned v=g_ehci_portsc; char d[9];int i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="QNX="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_nxt; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];
                                      t=" QAL="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_alt; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];
                                      t=" QOT="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_ovl; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];
                                      t=" QB0="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_b0; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];
                                      t=" QCU="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_qh_cur; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n';
                                      t="TNX="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_td_nxt; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n'; }
                                    { extern int g_ehci_fl_entry, g_ehci_td0_token;
                                      const char *t="FL0="; while(*t&&p<250)buf[p++]=*t++;
                                      unsigned v=g_ehci_fl_entry; char d[9];int i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];
                                      t=" TD0="; while(*t&&p<250)buf[p++]=*t++;
                                      v=g_ehci_td0_token; i=0;
                                      do{d[i++]="0123456789ABCDEF"[v&0xF];v>>=4;}while(i<8);
                                      while(i&&p<250)buf[p++]=d[--i];buf[p++]='\n'; }
                                    { extern int g_mouse_report_count;
                                      const char *t="Rpt="; while(*t&&p<250)buf[p++]=*t++;
                                      int v=g_mouse_report_count; if(!v)buf[p++]='0'; else{char d[8];int i=0;
                                      while(v){d[i++]='0'+v%10;v/=10;} while(i)buf[p++]=d[--i];}
                                      buf[p++]='\n'; }
                                    { extern int usb_mouse_get_count(void); int v=usb_mouse_get_count();
                                      const char *t="Mouse="; while(*t&&p<250)buf[p++]=*t++;
                                      if(!v)buf[p++]='0'; else{char d[8];int i=0;
                                      while(v){d[i++]='0'+v%10;v/=10;} while(i)buf[p++]=d[--i];}
                                      buf[p++]='\n'; }
                                    buf[p] = 0;
                                    int ly = 0;
                                    char *ep = buf;
                                    while (*ep && ly < 760) {
                                        char *ee = ep;
                                        while (*ee && *ee != '\n') ee++;
                                        for (int i = 0; i < (ee-ep) && i < 120; i++) {
                                            char ch = ep[i];
                                            if (ch < 32 || ch > 126) ch = ' ';
                                            const unsigned char *g = font8x8_data[(unsigned char)ch - 32];
                                            for (int r = 0; r < 8; r++) {
                                                unsigned char bits = g[r];
                                                for (int c = 0; c < 8; c++)
                                                    if (bits & (0x80 >> c))
                                                        fb[(ly+r)*ppitch + 2 + i*8 + c] = 0x0000FF00;
                                            }
                                        }
                                        ly += 10;
                                        ep = ee + 1;
                                    }
                                }
                            }
                            event.type = 0; event.x = 0; event.y = 0; event.pressed = 0;
                            copy_to_user((void*)tf->ebx, &event, sizeof(event));
                            tf->eax = 0;
                            break;
                        } // end else (overlay ON)
                    }
                    // F2/F key removed ???use netlog command instead
                    event.type = 1;
                    event.x = scancode;
                    event.y = 0;
                    event.pressed = 1;

                    // ?????????????????????
                    struct input_event *user_event = (struct input_event *)tf->ebx;
                    if (copy_to_user(user_event, &event, sizeof(event)) != 0) {
                        tf->eax = -1;
                        break;
                    }

                    tf->eax = 1;  // ?????????
                } else {
                    tf->eax = 0;  // ?????????
                }
            } else if (event_type == 2) {
                // ???? ??????B???????????????- ????????? + ????????????
                extern int usb_mouse_get_count(void);
                extern int usb_mouse_read(int mouse_index, void *report);
                extern int usb_mouse_data_available(int mouse_index);
                extern int usb_mouse_is_absolute(void);
                extern int usb_mouse_get_absolute(int *x, int *y, uint8_t *buttons);

                int new_data_from_usb = 0;

                /* Try to read USB mouse data without blocking. */
                if (usb_mouse_get_count() > 0) {
                    if (usb_mouse_data_available(0)) {
                        struct {
                            uint8_t buttons;
                            int8_t x;
                            int8_t y;
                            int8_t wheel;
                            uint8_t reserved[4];
                        } mouse_report;

                        int bytes = usb_mouse_read(0, &mouse_report);

                        if (bytes > 0) {
                            // ??????USB???????????????????????????
                            int old_buttons = usb_mouse_buttons;

                            if (usb_mouse_is_absolute()) {
                                uint8_t abs_buttons = 0;
                                usb_mouse_get_absolute(&usb_mouse_x, &usb_mouse_y,
                                                       &abs_buttons);
                                usb_mouse_buttons = abs_buttons;
                            } else {
                                usb_mouse_x += mouse_report.x;
                                usb_mouse_y -= mouse_report.y;
                                usb_mouse_buttons = mouse_report.buttons;
                            }

                            // ????????????
                            if (usb_mouse_x < 0) usb_mouse_x = 0;
                            if (usb_mouse_y < 0) usb_mouse_y = 0;
                            if (usb_mouse_x >= 1024) usb_mouse_x = 1023;
                            if (usb_mouse_y >= 768) usb_mouse_y = 767;

                            new_data_from_usb = 1;

                            (void)old_buttons;
                        } else {
                            /* no report */
                        }
                    } else {
                        /* no cached USB mouse report */
                    }
                }

                // PS/2 mouse fallback: only use it when no USB mouse/tablet
                // is present. Otherwise QEMU's default PS/2 mouse fights the
                // USB tablet absolute coordinates and makes the cursor drift.
                if (usb_mouse_get_count() == 0) {
                    int ps2_x=0, ps2_y=0, ps2_btn=0;
                    extern int ps2mouse_poll(int *x, int *y, int *btn);
                    if(ps2mouse_poll(&ps2_x, &ps2_y, &ps2_btn)){
                        event.type = 2;
                        event.x = ps2_x; event.y = ps2_y;
                        event.pressed = ps2_btn;
                        goto send_event;
                    }
                }
                // USB mouse
                if (usb_mouse_x != mouse_last_x ||
                    usb_mouse_y != mouse_last_y ||
                    (int)usb_mouse_buttons != mouse_last_buttons) {

                    event.type = 2;
                    event.x = usb_mouse_x;
                    event.y = usb_mouse_y;
                    event.pressed = usb_mouse_buttons & 0x07;  // bits: L=1, R=2, M=4
                    mouse_last_x = usb_mouse_x;
                    mouse_last_y = usb_mouse_y;
                    mouse_last_buttons = (int)usb_mouse_buttons;
                    goto send_event;
                }
                tf->eax = 0; // no USB data
                goto no_event;

            send_event:
                { struct input_event *ue = (struct input_event *)tf->ebx;
                  if(copy_to_user(ue, &event, sizeof(event))==0) tf->eax=1; else {tf->eax=-1; break;} }
                break;
            no_event:
                tf->eax = 0;
                break;
            } else {
                // ??????????????????????????????
                tf->eax = -1;
            }
            break;
        }
        case SYS_USB_MOUSE_POLL: {
            // ?????? USB ???????????????????????????????????????
            // ?????????ebx = usb_mouse_report_t* (???????????????
            // ?????????eax = 1 ????????? 0 ????????? -1 ??????

            extern int usb_mouse_get_count(void);
            extern int usb_mouse_read(int mouse_index, void *report);
            extern int usb_mouse_data_available(int mouse_index);

            static int poll_count = 0;
            poll_count++;

            if (usb_mouse_get_count() == 0) {
                tf->eax = -1;  // ????????????
                break;
            }

            int avail = usb_mouse_data_available(0);
            if (!avail) {
                tf->eax = 0;  // ?????????                break;
            }

            // ??????????????????
            struct {
                uint8_t buttons;
                int8_t x;
                int8_t y;
                int8_t wheel;
                uint8_t reserved[4];
            } mouse_report;

            int bytes = usb_mouse_read(0, &mouse_report);
            if (bytes <= 0) {
                tf->eax = -1;
                break;
            }

            // ?????????????????????
            void *user_report = (void *)tf->ebx;
            if (copy_to_user(user_report, &mouse_report, sizeof(mouse_report)) != 0) {
                tf->eax = -1;
                break;
            }

            tf->eax = 1;  // ?????????
            break;
        }
        case SYS_USB_MOUSE_INFO: {
            // SYS_USB_MOUSE_INFO (76): ?????? USB ??????????????????
            // ??????: ebx=ep??????, ecx=maxpkt??????, edx=interval??????
            extern uint8_t g_usb_mouse_ep;
            extern uint8_t g_usb_mouse_maxpkt;
            extern uint8_t g_usb_mouse_interval;
            extern uint32_t g_td_ctrl;
            extern uint8_t g_dma_bytes[8];
            extern int g_usb_setproto_result;
            if (tf->ebx == 1) {
                uint32_t d = g_dma_bytes[0]|(g_dma_bytes[1]<<8)|(g_dma_bytes[2]<<16)|(g_dma_bytes[3]<<24);
                tf->eax = (int)d;
            } else if (tf->ebx == 3) {
                extern uint32_t g_ehci_fl_phys;
                tf->eax = (int)g_ehci_fl_phys;
            } else if (tf->ebx == 4) {
                extern uint32_t g_ehci_qh_phys;
                tf->eax = (int)g_ehci_qh_phys;
            } else if (tf->ebx == 2) {
                // Return DMA bytes 4-7
                uint32_t d = g_dma_bytes[4]|(g_dma_bytes[5]<<8)|(g_dma_bytes[6]<<16)|(g_dma_bytes[7]<<24);
                tf->eax = (int)d;
            } else {
                // ebx=0: return packed USB info
                uint32_t info = g_usb_mouse_ep | (g_usb_mouse_maxpkt<<8) | (g_usb_mouse_interval<<16) | ((g_usb_setproto_result & 0xFF) << 24);
                tf->eax = (int)info;
            }
            break;
        }
        case 77: {
            // SYS_KLOG_READ: ecx=0???read klog, ecx=1???read ehci, ecx=2???draw to FB
            extern void klog_read(char *buf, int max);
            /* A full page keeps this temporary buffer on the reclaimable
             * buddy-backed kmalloc path instead of the early allocation pool. */
            char *buf = (char *)kmalloc(4096);
            if (!buf) {
                tf->eax = -1;
                break;
            }
            if (tf->ecx == 2) {
                // Draw klog to framebuffer line by line (kernel space, safe)
                klog_read(buf, 2048);
                volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
                extern uint16_t vbe_get_pitch(void);
                int pitch = vbe_get_pitch();
                int ppitch = pitch / 4;  // pitch is bytes, ppitch is pixels per row
                int start_y = 0;
                // Clear top half
                for (int dy = 0; dy < 420; dy++)
                    for (int dx = 0; dx < 1024; dx++)
                        fb[(start_y+dy)*ppitch + dx] = 0x00000000;
                // Display lines from bottom of klog (show last ~40 lines)
                int line_y = start_y;
                int pos = 0;
                while (buf[pos] && line_y < 400) {
                    // Find end of line
                    int end = pos;
                    while (buf[end] && buf[end] != '\n' && buf[end] != '\r') end++;
                    // Draw this line
                    for (int i = 0; i < (end-pos) && i < 120; i++) {
                        char ch = buf[pos+i];
                        if (ch < 32 || ch > 126) ch = ' ';
                        const unsigned char *g = font8x8_data[(unsigned char)ch - 32];
                        for (int r = 0; r < 8; r++) {
                            unsigned char bits = g[r];
                            for (int c = 0; c < 8; c++)
                                if (bits & (0x80 >> c))
                                    fb[(line_y+r)*ppitch + 2 + i*8 + c] = 0x0000FF00;
                        }
                    }
                    line_y += 10;
                    // Skip to next line
                    pos = end;
                    while (buf[pos] == '\n' || buf[pos] == '\r') pos++;
                }
                tf->eax = 0;
            } else if (tf->ecx == 1) {
                extern char *g_ehci_debug_buf;
                extern int g_ehci_debug_len;
                int len = g_ehci_debug_len;
                if (len > 2047) len = 2047;
                if (g_ehci_debug_buf && len > 0) {
                    memcpy(buf, g_ehci_debug_buf, len);
                    buf[len] = 0;
                } else {
                    buf[0] = 0;
                }
                copy_to_user((void*)tf->ebx, buf, 2048);
                tf->eax = 0;
            } else {
                klog_read(buf, 2048);
                copy_to_user((void*)tf->ebx, buf, 2048);
                tf->eax = 0;
            }
            kfree(buf);
            break;
        }
        case SYS_NET_BIND: {
            // SYS_NET_BIND (52): ?????? UDP ??????
            // ??????: ebx = ??????????????????????????????
            // ??????: eax = 0 ??????, -1 ??????

            uint16_t port = (uint16_t)tf->ebx;
            printf("[syscall] SYS_NET_BIND: port=%d\n", port);

            extern void net_bind_udp_port(uint16_t port);
            net_bind_udp_port(port);

            tf->eax = 0;
            break;
        }
        case SYS_NET_RECV_UDP: {
            // SYS_NET_RECV_UDP (53): ?????? UDP ??????
            // ??????: ebx = ??????????????? ecx = ????????????
            // ??????: eax = ????????????????????? -1 ????????? -2 ??????

            printf("[syscall] [SYS_NET_RECV_UDP] 1\n");

            extern int net_recv_udp_internal(char *buf, int len, int *port);

            char *user_buf = (char *)tf->ebx;
            int max_len = (int)tf->ecx;
            int temp_port = 0;

            // ?????????????????????
            if (max_len <= 0 || max_len > UDP_RX_BUF_SIZE) {
                tf->eax = -2;
                break;
            }
            uint32_t temp_alloc_size = (max_len < 4096) ? 4096 : (uint32_t)max_len;
            char *temp_buf = (char *)kmalloc(temp_alloc_size);
            if (!temp_buf) {
                tf->eax = -2;
                break;
            }

            int ret = net_recv_udp_internal(temp_buf, max_len, &temp_port);

            printf("[syscall] [SYS_NET_RECV_UDP] 2 ret=%d\n", ret);

            if (ret <= 0) {
                kfree(temp_buf);
                tf->eax = ret;
                break;
            }

            // ???????????????????????????
            if (copy_to_user(user_buf, temp_buf, ret) != 0) {
                kfree(temp_buf);
                tf->eax = -2;
                break;
            }

            printf("[syscall] [SYS_NET_RECV_UDP] 3\n");

            kfree(temp_buf);
            tf->eax = ret;
            break;
        }
        case 78: { /* SYS_CHERRYUSB_MOUSE_READ ???removed, use SYS_USB_MOUSE_POLL instead */
            tf->eax = -1;
            break;
        }
        case SYS_LS_DISK: {
            /* arg1 = user buf, arg2 = max, arg3 = path string */
            char *kbuf = (char *)kmalloc(4096);
            char kpath[128];
            int outmax = arg2;
            int ret = -1;
            if (!kbuf) {
                tf->eax = -1;
                break;
            }
            if (outmax <= 0 || outmax > 4096) outmax = 4096;
            kbuf[0] = '\0';
            const char *path = "/";
            if (arg3) {
                copy_from_user(kpath, (const char*)arg3, sizeof(kpath) - 1);
                kpath[sizeof(kpath) - 1] = '\0';
                path = kpath;
            }
            extern int vfs_list_dir(const char *path, char *buf, int max);
            ret = vfs_list_dir(path, kbuf, outmax);
            printf("[lsdisk] path='%s' ret=%d first='%c'\n",
                   path, ret, kbuf[0] ? kbuf[0] : '.');
            if (ret < 0) {
                int p = 0;
                const char *s = "ls: cannot access ";
                while (s[p] && p < outmax - 1) { kbuf[p] = s[p]; p++; }
                for (int i = 0; path[i] && p < outmax - 1; i++) kbuf[p++] = path[i];
                if (p < outmax - 1) kbuf[p++] = '\n';
                kbuf[p] = '\0';
            } else {
                if (ret >= outmax) ret = outmax - 1;
                kbuf[ret] = '\0';
            }
            if(arg1) copy_to_user((void*)arg1, kbuf, outmax);
            kfree(kbuf);
            tf->eax = ret;
            break;
        }
        case 80: { /* SYS_NETLOG: netlog <ip> <port> ???send console via UDP */
            uint32_t ebx = tf->ebx;
            uint32_t ecx = tf->ecx;
            printf("[netlog] ebx=0x%x ecx=%d\n", ebx, ecx);
            if (ebx == 0) {
                printf("[netlog] disabled\n");
                tf->eax = 0;
                break;
            }
            /* Parse IP */
            const char *ip_str = (const char *)ebx;
            printf("[netlog] ip_str='%s'\n", ip_str);
            int port = (int)ecx;
            if (port <= 0 || port > 65535) port = 9999;
            uint32_t dst_ip = 0;
            uint8_t octets[4]; int oi = 0; uint32_t cur = 0;
            const char *p = ip_str; while (*p == ' ') p++;
            for (; *p; p++) {
                if (*p == '.') { octets[oi++] = (uint8_t)cur; cur = 0; }
                else if (*p >= '0' && *p <= '9') cur = cur * 10 + (*p - '0');
                else if (*p == ' ') break;
            }
            octets[oi] = (uint8_t)cur;
            dst_ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
            printf("[netlog] dst=%d.%d.%d.%d:%d\n", octets[0], octets[1], octets[2], octets[3], port);

            /* Get console data */
            extern void *console_get_buf(void);
            extern int  console_get_len(void);
            char *data = (char *)console_get_buf();
            int len = console_get_len();
            printf("[netlog] console buf=%p len=%d first=%02x%02x%02x\n",
                   data, len, data?data[0]:0, data?data[1]:0, data?data[2]:0);
            if (!data || len <= 0) { printf("[netlog] console empty\n"); tf->eax = -1; break; }

            /* Get device ???copy case 45 */
            extern int net_get_device_count(void);
            extern net_device_t **net_get_all_devices(void);
            int cnt = net_get_device_count();
            net_device_t **devs = net_get_all_devices();
            printf("[netlog] net devs=%d\n", cnt);
            net_device_t *dev = NULL;
            for (int i = 0; i < cnt; i++)
                if (devs[i] && devs[i]->send && strcmp(devs[i]->name, "lo") != 0)
                    { dev = devs[i]; printf("[netlog] picked %s\n", dev->name); break; }
            if (!dev) { printf("[netlog] no device\n"); tf->eax = -2; break; }

            /* Send */
            extern int udp_output(net_device_t *d, uint32_t ip,
                                  uint16_t sp, uint16_t dp, uint8_t *b, uint32_t l);
            printf("[netlog] calling udp_output dev=%s dst=0x%x port=%d len=%d data=%p\n",
                   dev->name, dst_ip, port, len, data);
            int ret = udp_output(dev, dst_ip, port, port, (uint8_t *)data, len);
            printf("[netlog] udp_output returned %d\n", ret);
            tf->eax = (ret >= 0) ? 0 : -4;
            break;
        }
        case 81: { /* klog.flush */
            extern int klog_flush(void);
            tf->eax = klog_flush();
            break;
        }
        case 82: { /* SYS_GET_USB_LOG ???filter by keyword in edx, or all if empty */
            char *ubuf = (char *)tf->ebx;
            int   maxlen = (int)tf->ecx;
            const char *kw = (const char *)tf->edx; /* filter keyword, "" or NULL = all */
            int kwlen = 0; if(kw) while(kw[kwlen]) kwlen++;
            extern void usb_log_snapshot(void);
            extern void *usb_log_get_buf(void);
            extern int   usb_log_get_size(void);
            usb_log_snapshot();
            char *kbuf = (char *)usb_log_get_buf();
            int   klen = usb_log_get_size();
            if (!ubuf || maxlen <= 0) { tf->eax = -1; break; }
            if (!kbuf || klen <= 0) { tf->eax = 0; break; }

            int out = 0, ls = 0;
            for (int i = 0; i < klen && out < maxlen - 1; i++) {
                if (kbuf[i] != '\n' && i < klen - 1) continue;
                int ok = (kwlen == 0); /* no filter = match all */
                if (!ok) {
                    for (int j = ls; j <= i - kwlen; j++) {
                        int m = 1;
                        for (int k = 0; k < kwlen; k++)
                            if (kbuf[j+k] != kw[k]) { m=0; break; }
                        if (m) { ok = 1; break; }
                    }
                }
                if (ok) {
                    for (int j = ls; j <= i && out < maxlen - 1; j++)
                        ubuf[out++] = kbuf[j];
                }
                ls = i + 1;
            }
            ubuf[out] = 0;
            tf->eax = out;
            break;
        }
        case SYS_USB_INIT: {
            extern int usb_get_init_state(void);
            tf->eax = usb_get_init_state();
            printf("[syscall] USB init syscall is status-only, state=%d\n", tf->eax);
            break;
        }
        case SYS_USB_LOG_SAVE: {
            extern int fat32_save_log(void);
            tf->eax = fat32_save_log();
            printf("[syscall] USB log save %s\n",
                   tf->eax == 0 ? "OK" : "failed");
            break;
        }
        case SYS_USB_STATUS: {
            extern int usb_get_init_state(void);
            tf->eax = usb_get_init_state();
            break;
        }
        case SYS_USB_LOG_DISPLAY: {
            extern void usb_log_set_console(int enabled);
            extern int usb_log_get_console(void);
            if ((int)tf->ebx >= 0)
                usb_log_set_console(tf->ebx ? 1 : 0);
            tf->eax = usb_log_get_console();
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

// ==================== ???????????????????????? ====================

/**
 * @brief syscall_net_ifconfig() - ????????????????????????
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
 * @brief syscall_lspci() - ????????????PCI ??????
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
 * @brief net_send_udp() - ??????UDP ???
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


