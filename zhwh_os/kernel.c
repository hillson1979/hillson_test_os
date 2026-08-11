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
#include "fbtext.h"
//#include "ebda.h"
#include "segment.h"
#include "interrupt.h"
#include "mm.h"
#include "kmalloc.h"
#include "task.h"
#include "sched.h"
#include "x86/io.h"
#include "net/wifi/atheros.h"
#include "fs.h"

/* Keep physical NIC access out of the critical desktop boot path. */
#define BOOT_AUTO_NETWORK 0
#define BOOT_AUTO_USB 1
/* Temporary physical-machine diagnostics. Set to 0 after xHCI mouse input is
 * stable so runtime USB status lines cannot overwrite the desktop. */
#define USB_RUNTIME_SCREEN_DEBUG 0

// Forward declarations for task types
typedef struct {
    uint32_t *phys;
    uint32_t *virt;
} page_t;

extern void copy_kernel_mappings_to_pd(uint32_t *pd_user);
volatile void panic(const char *str);

static void usb_boot_delay(int us)
{
        volatile int i;
        for (i = 0; i < us * 100; i++)
                __asm__ volatile("pause" : : : "memory");
}

extern void* _kernel_start_virtual;
extern void* _kernel_end_virtual;
extern void* data;

/* 杞�?multiboot2 鐗╃悊鍦板潃鍒拌櫄鎷熷湴鍧€ */
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

    // 閬嶅巻鎵€鏈夋爣�?
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
        // 绉诲姩鍒颁笅涓€涓爣绛?(澶у皬蹇呴』鏄?瀛楄妭瀵归�?
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (!found_modules) {
        printf("no modules found\n");
    }
}


uint32_t get_esp(void) {
    uint32_t esp;
    __asm__ volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

static uint32_t boot_fb_pitch;
static uint32_t boot_fb_height;
static int boot_fb_ready;
static int usb_boot_screen_last_state = -99;
static uint32_t usb_boot_last_tick = 0;
static int usb_boot_line = 0;
static int usb_boot_started = 0;
static uint32_t usb_boot_text_color = 0x00000000;
static uint32_t usb_boot_line_color = 0x00E0E000;
#define USB_STICKY_LINES 5
static void usb_boot_sticky_error(const char *stage, const char *detail);
//
static void usb_boot_status_screen(const char *stage, const char *detail)
{
        extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
        extern uint16_t vbe_get_pitch(void);
        char line[80];
        extern int snprintf(char *, unsigned int, const char *, ...);
        static volatile uint32_t *usb_fb = NULL;
        static int usb_fb_pitch_px = 0;
        static uint16_t usb_fb_width = 0;
        static uint16_t usb_fb_height = 0;
        static int usb_fb_ready = 0;

        if (!usb_fb_ready) {
                usb_fb = (volatile uint32_t *)0xF0000000;
                vbe_get_resolution(&usb_fb_width, &usb_fb_height);
                usb_fb_pitch_px = vbe_get_pitch() / 4;
                usb_fb_ready = (usb_fb && usb_fb_pitch_px > 0 && usb_fb_width > 0 && usb_fb_height > 0);
                if (usb_fb_ready) {
                        for (uint32_t y = 0; y < usb_fb_height; y++) {
                                for (uint32_t x = 0; x < usb_fb_width; x++) {
                                        usb_fb[y * usb_fb_pitch_px + x] = 0x00E0E000; /* yellow-ish */
                                }
                        }
                        fb_draw_text(usb_fb, 16, 16, "USB INIT", 0x00000000, usb_fb_pitch_px);
                        fb_draw_text(usb_fb, 16, 32, "Yellow boot log", 0x00000000, usb_fb_pitch_px);
                        fb_draw_text(usb_fb, 16, 48, "One line per second", 0x00000000, usb_fb_pitch_px);
                        fb_draw_text(usb_fb, 16, 64, "Kernel-owned USB init", 0x00000000, usb_fb_pitch_px);
                }
        }

        if (!usb_boot_started) {
                usb_boot_started = 1;
                usb_boot_line = 7;
        }
        snprintf(line, sizeof(line), "[%s] %s", stage ? stage : "unknown", detail ? detail : "");
        if (usb_fb_ready) {
                const int first_log_y = 7 * 16;
                const int line_height = 16;
                int max_lines = usb_fb_height / line_height - USB_STICKY_LINES;
                if (max_lines < 8) max_lines = usb_fb_height / line_height;

                if (usb_boot_line >= max_lines) {
                        /* Preserve the header and scroll only the USB log area. */
                        for (int y = first_log_y;
                             y < max_lines * line_height - line_height; y++) {
                                for (uint32_t x = 0; x < usb_fb_width; x++) {
                                        usb_fb[y * usb_fb_pitch_px + x] =
                                                usb_fb[(y + line_height) *
                                                       usb_fb_pitch_px + x];
                                }
                        }
                        for (int y = max_lines * line_height - line_height;
                             y < max_lines * line_height; y++) {
                                for (uint32_t x = 0; x < usb_fb_width; x++) {
                                        usb_fb[y * usb_fb_pitch_px + x] =
                                                0x00E0E000;
                                }
                        }
                        usb_boot_line = max_lines - 1;
                }

                int y = usb_boot_line * 16;
                for (int py = y; py < y + line_height; py++) {
                        for (uint32_t x = 0; x < usb_fb_width; x++) {
                                usb_fb[py * usb_fb_pitch_px + x] =
                                        usb_boot_line_color;
                        }
                }
                fb_draw_text(usb_fb, 16, y, line,
                             usb_boot_text_color, usb_fb_pitch_px);
                usb_boot_line++;
        }
}

void kernel_usb_status_line(const char *stage, const char *detail)
{
        extern int usb_get_init_state(void);
        if (!USB_RUNTIME_SCREEN_DEBUG && usb_get_init_state() == 4)
                return;
        usb_boot_status_screen(stage, detail);
        usb_boot_sticky_error(stage, detail);
}

void kernel_usb_msc_status_line(const char *stage, const char *detail)
{
        extern int usb_get_init_state(void);
        if (!USB_RUNTIME_SCREEN_DEBUG && usb_get_init_state() == 4)
                return;
        usb_boot_text_color = 0x00000000;
        usb_boot_line_color = 0x0060A8FF;
        usb_boot_status_screen(stage, detail);
        usb_boot_sticky_error(stage, detail);
        usb_boot_line_color = 0x00E0E000;
}


static void usb_boot_sticky_error(const char *stage, const char *detail)
{
        extern void vbe_get_resolution(uint16_t *width, uint16_t *height);
        extern uint16_t vbe_get_pitch(void);
        extern int snprintf(char *, unsigned int, const char *, ...);
        volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
        uint16_t width = 0, height = 0;
        uint32_t pitch_px;
        static char sticky[USB_STICKY_LINES][96];
        static int sticky_count = 0;
        char line[96];

        vbe_get_resolution(&width, &height);
        pitch_px = vbe_get_pitch() / 4;
        if (!fb || !pitch_px || !width || height < (USB_STICKY_LINES * 16 + 16))
                return;

        snprintf(line, sizeof(line), "[%s] %s",
                 stage ? stage : "unknown", detail ? detail : "");

        for (int i = 0; i < USB_STICKY_LINES - 1; i++) {
                for (int j = 0; j < 96; j++)
                        sticky[i][j] = sticky[i + 1][j];
        }
        for (int j = 0; j < 96; j++)
                sticky[USB_STICKY_LINES - 1][j] = line[j];
        if (sticky_count < USB_STICKY_LINES)
                sticky_count++;

        int y0 = height - USB_STICKY_LINES * 16;
        for (int y = y0; y < (int)height; y++) {
                for (uint32_t x = 0; x < width; x++)
                        fb[y * pitch_px + x] = 0x00000080;
        }
        for (int i = 0; i < sticky_count; i++) {
                int src = USB_STICKY_LINES - sticky_count + i;
                fb_draw_text(fb, 16, y0 + i * 16,
                             sticky[src], 0x00FFFFFF, pitch_px);
        }
}
void kernel_usb_error_line(const char *stage, const char *detail)
{
        extern int usb_get_init_state(void);
        if (!USB_RUNTIME_SCREEN_DEBUG && usb_get_init_state() == 4)
                return;
        usb_boot_text_color = 0x00FF0000;
        usb_boot_status_screen(stage, detail);
        usb_boot_sticky_error(stage, detail);
        usb_boot_text_color = 0x00000000;
}
static void usb_boot_status_refresh(void)
{
        extern int usb_get_init_state(void);
        extern uint32_t ticks;
        int state = usb_get_init_state();
        if (state == usb_boot_screen_last_state && (ticks - usb_boot_last_tick) < 100)
                return;
        usb_boot_screen_last_state = state;
        usb_boot_last_tick = ticks;

        switch (state) {
        case 0:
            usb_boot_status_screen("idle", "USB init not started");
            break;
        case 1:
            usb_boot_status_screen("scan", "Scanning cached PCI USB controllers");
            break;
        case 2:
            usb_boot_status_screen("start", "xHCI found, controller start pending");
            break;
        case 3:
            usb_boot_status_screen("ready", "xHCI ready, scanning root ports");
            break;
        case 4:
            usb_boot_status_screen("done", "USB init complete");
            break;
        default:
            usb_boot_status_screen("error", "USB init failed");
            break;
        }
}

/*
 * Kernel-owned USB init gateway.
 *
 * All USB bring-up is requested and advanced here. User syscalls may query
 * state, but must not drive controller reset/root-port enumeration. This keeps
 * the terminal and desktop input path away from long hardware init work.
 */
static int kernel_usb_init_requested = 0;
static int kernel_usb_init_done_reported = 0;

static int kernel_usb_init_start(const char *reason)
{
        extern int usb_request_init(void);
        extern int usb_get_init_state(void);
        int ret;

        if (kernel_usb_init_requested)
                return usb_get_init_state();

        kernel_usb_init_requested = 1;
        printf("Initializing USB via kernel gateway (%s)...\n",
               reason ? reason : "boot");
        usb_boot_status_screen("request", reason ? reason : "kernel boot");

        ret = usb_request_init();
        printf("USB init request accepted by kernel, state=%d ret=%d\n",
               usb_get_init_state(), ret);
        usb_boot_status_refresh();
        return ret;
}

static void kernel_usb_init_finish_once(void)
{
        extern int usb_mouse_get_count(void);
        extern int usb_msc_is_ok(void);
        int mouse_count;
        int msc_ok;

        if (kernel_usb_init_done_reported)
                return;
        kernel_usb_init_done_reported = 1;

        mouse_count = usb_mouse_get_count();
        msc_ok = usb_msc_is_ok();
        printf("USB init complete: mouse_count=%d msc_ok=%d\n",
               mouse_count, msc_ok);
        usb_boot_status_screen("done", "USB init complete; devices may now be used");
}

static int kernel_usb_init_service(void)
{
        extern void usb_init_poll(void);
        extern int usb_get_init_state(void);
        int state;

        if (!kernel_usb_init_requested)
                return 0;

        state = usb_get_init_state();
        if (state > 0 && state < 4)
                usb_init_poll();

        usb_boot_status_refresh();

        state = usb_get_init_state();
        if (state == 4)
                kernel_usb_init_finish_once();

        return state;
}

static int kernel_usb_init_boot_wait(void)
{
        extern int usb_get_init_state(void);
        int state = usb_get_init_state();

        /* Boot owns initial USB bring-up.  Pump the state machine here so the
         * desktop and user USB tests cannot run while xHCI is only "found".
         * Runtime hotplug still uses the small idle-service path below. */
        for (int i = 0; i < 20000; i++) {
                int old_state = state;
                state = kernel_usb_init_service();
                if (state != old_state || (i % 1000) == 0) {
                        printf("USB boot wait step=%d state=%d\n", i, state);
                        usb_boot_status_refresh();
                }
                if (state == 4 || state < 0)
                        break;
                usb_boot_delay(1000);
        }

        state = usb_get_init_state();
        printf("USB boot wait finished, state=%d\n", state);
        if (state != 4) {
                usb_boot_status_screen("panic", "USB init did not complete in kernel");
                panic("USB init did not complete in kernel boot");
        }
        return state;
}
/* Fill a narrow band at the top. Each later stage overwrites it. */
static void boot_color(uint32_t stage, uint32_t rgb)
{
        if (!boot_fb_ready) return;
        volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
        uint32_t pixels = boot_fb_pitch / 4;
        uint32_t y0 = (stage - 1) * 96;
        if (y0 >= boot_fb_height) return;
        uint32_t rows = boot_fb_height - y0 < 96 ? boot_fb_height - y0 : 96;
        for (uint32_t y = 0; y < rows; y++)
                for (uint32_t x = 0; x < pixels; x++)
                        fb[(y0 + y) * pixels + x] = rgb;
}

volatile void panic(const char *str)
{
        extern void vga_panic_screen(const char *, const char *, const char *, const char *);
        if (!str) str = "kernel panic";
        vga_panic_screen("KERNEL PANIC", str,
                         "System halted. Press reset or power cycle.",
                         NULL);
        __asm__ volatile("cli; hlt; jmp .");
}

static void boot_framebuffer_init(uint32_t mb_info_addr)
{
        uint32_t *mb = (uint32_t *)mb2_phys_to_virt(mb_info_addr);
        multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb + 8);

        while (tag->type != MULTIBOOT_TAG_TYPE_END) {
                if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                        multiboot_tag_framebuffer_t *fb =
                                (multiboot_tag_framebuffer_t *)tag;
                        if ((fb->framebuffer_addr >> 32) == 0 &&
                            fb->framebuffer_type == 1 &&
                            fb->framebuffer_bpp == 32 && fb->framebuffer_pitch) {
                                vbe_init_from_multiboot(fb->framebuffer_addr,
                                        fb->framebuffer_width, fb->framebuffer_height,
                                        fb->framebuffer_pitch, fb->framebuffer_bpp);
                                boot_fb_pitch = fb->framebuffer_pitch;
                                boot_fb_height = fb->framebuffer_height;
                                boot_fb_ready = 1;
                        }
                        return;
                }
                tag = (multiboot_tag_t *)((uint8_t *)tag +
                                          ((tag->size + 7) & ~7));
        }
}

int
kernel_main(uint32_t mb_magic, uint32_t mb_info_addr)
{
        // 馃敟 鐪熸満璋冭瘯锛氭渶鏃￢GA杈撳嚭锛屽啓鍦╲ga_init娓呭睆涔嬪墠
        volatile uint16_t *vega = (volatile uint16_t *)0xC00B8000;
        for (int i = 0; i < 80*25; i++) vega[i] = (0x2F << 8) | ' ';
        vega[0] = (0x4F << 8) | 'K';
        vega[1] = (0x4F << 8) | 'O';
        vega[2] = (0x4F << 8) | 'K';


        // 馃敟 鍐呮牳鏍堟孩鍑烘娴嬶細鍦ㄦ爤搴曡缃摠鍏?
        extern uint32_t stack_base;
        extern uint32_t stack_top;
        uint32_t *stack_sentinel = &stack_base;
        *stack_sentinel = 0xDEADBEEF;  // 鍝ㄥ叺鍊?

        // 鈿狅�?淇濆瓨鍐呮牳椤电洰褰曠墿鐞嗗湴鍧€锛堝湪鍒囨崲鍒扮敤鎴疯繘绋嬩箣鍓嶏�?
        extern uint32_t kernel_page_directory_phys;
        uint32_t cr3_value;
        __asm__ volatile("movl %%cr3, %0" : "=r"(cr3_value));
        kernel_page_directory_phys = cr3_value & ~0xFFF;


        //printf("[kernel_main] Saved kernel CR3 phys=0x%x\n", kernel_page_directory_phys);


        // 鏄剧ず鍐呮牳鏍堜俊鎭?
        printf("[kernel_main] Stack: base=0x%x top=0x%x size=%u KB\n",
               (uint32_t)&stack_base, (uint32_t)&stack_top,
               ((uint32_t)&stack_top - (uint32_t)&stack_base) / 1024);

        // 馃敟 涓存椂绂佺敤鎵€�?UART 鍜屽鏉傚垵濮嬪寲锛屾祴璇曟渶灏忓惎�?
        // uart_init();
        // uart_puts("[UART] Serial port initialized at 115200 baud\n");

        // 馃敟 鍒濆鍖栦互澶綉璋冭瘯鎺ュ�?
        //netdebug_init();

        

        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted with Multiboot 2!\n");

        // ================================
        // 鎵撳�?TASK_IFRAME 鍊硷紙璋冭瘯鍋忕Щ閲忎竴鑷存€э�?
        // ================================
        printf("==================== TASK_IFRAME OFFSET CHECK ====================\n");
        printf("[include/task.h]  TASK_IFRAME (OFFSETOF) = %d\n", TASK_IFRAME);
        printf("[task_impl.s]     .set TASK_IFRAME, 156\n");
        printf("[interrupt_exit.s].set TASK_TF, 156\n");
        printf("[task_offsets.s]  .set TASK_IFRAME, 156\n");
        printf("================================================================\n");
        if (TASK_IFRAME != 156) {
            printf("WARNING: TASK_IFRAME mismatch! Expected 156, got %d\n", TASK_IFRAME);
        } else {
            printf("OK: All TASK_IFRAME values are consistent (156)\n");
        }
        printf("================================================================\n\n");

        

        // uart_puts("[KERNEL] Booted with Multiboot 2!\n");
        printf("Magic: 0x%x\n", mb_magic);
        printf("Info addr: 0x%x\n", mb_info_addr);
        // uart_debug("[KERNEL] Magic: 0x%x, Info: 0x%x\n", mb_magic, mb_info_addr);

        // 楠岃�?multiboot2 榄旀�?
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            printf("ERROR: Invalid multiboot2 magic: 0x%x\n", mb_magic);
            // uart_panic("ERROR: Invalid multiboot2 magic!");  // 馃敟 娉ㄩ噴鎺夛紝閬垮厤鏈垵濮嬪寲鐨勪覆鍙ｈ闂鑷撮噸鍚?
            return -1;
        }

        mpinit();
        // 馃敟 鍏抽敭淇锛氬繀椤诲�?init_highmem_mapping() 涔嬪墠璁剧疆 GDT
        // 鍥犱�?init_highmem_mapping() 浼氳皟鐢?kmalloc_early �?map_4k_page
        // 杩欎簺鍑芥暟璋冪敤闇€瑕佹纭殑娈甸€夋嫨�?
        

        init_highmem_mapping();
        //acpi_init();

        // 淇濆�?multiboot2 淇℃�?渚涘唴瀛樼鐞嗕娇�?
        multiboot2_info_addr = mb_info_addr;

        // 瑙ｆ瀽鍩烘湰鍐呭瓨淇℃伅
        uint32_t mem_lower = 0, mem_upper = 0;

        // �?Multiboot 2 涓紝ebx 鎸囧悜淇℃伅缁撴瀯鐨勫紑濮嬪湴鍧�?
        // 淇℃伅缁撴瀯浠庡ぇ灏忓瓧娈靛紑�?4瀛楄�?锛岀劧鍚庢槸淇濈暀瀛楁�?4瀛楄�?锛屽啀鐒跺悗鏄爣绛炬暟缁?
        uint32_t *mb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);

        // 绗竴涓爣绛句粠鍋忕�?瀛楄妭寮€濮?
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
        boot_framebuffer_init(mb_info_addr);
        boot_color(1, 0x00FF0000); /* 1 red: memory/framebuffer */

        // 蹇呴』鍒濆�?LAPIC锛屽洜涓?logical_cpu_id() 渚濊禆�?
        lapicinit();

        // 馃敟 鍒濆鍖?IOAPIC锛堝繀椤诲湪閿洏鍒濆鍖栦箣鍓嶏紒锛?
        extern void ioapicinit(void);
        ioapicinit();
        printf("IOAPIC initialized\n");
        boot_color(2, 0x00FFFF00); /* 2 yellow: APIC */

        printf("Before seginit (early GDT setup)\n");
        seginit();
        printf("After seginit\n");

        printf("Before tss_init\n");
        tss_init();
        printf("After tss_init\n");
        boot_color(3, 0x0000FFFF); /* 3 cyan: segmentation/TSS */

        printf("Before tvinit\n");
        // 馃敟 璋冭瘯锛氬湪 tvinit() 鍓嶅悗妫€鏌?vectors[128]
        extern uint32_t vectors[];
        printf("[KERNEL] BEFORE tvinit: vectors[127] = 0x%x, vectors[128] = 0x%x, vectors[129] = 0x%x\n",
               vectors[127], vectors[128], vectors[129]);
        tvinit();
        boot_color(4, 0x0000FF00); /* 4 green: IDT gates populated */
        printf("[KERNEL] AFTER tvinit: vectors[127] = 0x%x, vectors[128] = 0x%x, vectors[129] = 0x%x\n",
               vectors[127], vectors[128], vectors[129]);
        printf("After tvinit\n");
        
        idtinit();
        boot_color(5, 0x000000FF); /* 5 blue: IDTR loaded */
        printf("segment idt init is ok\n");

        // 馃敟馃敟 鍦ㄥ紑涓柇鍓嶅啀娆＄‘淇?FPU 宸插垵濮嬪寲锛堥槻姝?Trap 19�?
        __asm__ volatile("fninit");
        __asm__ volatile("fnclex");
        uint32_t cr0_check;
        __asm__ volatile("movl %%cr0, %0" : "=r"(cr0_check));
        cr0_check &= ~(1 << 3);  // 娓呴�?TS
        cr0_check &= ~(1 << 2);  // 娓呴�?EM
        __asm__ volatile("movl %0, %%cr0" : : "r"(cr0_check));
        boot_color(6, 0x00FF00FF); /* 6 magenta: FPU/CR0 ready */

        // 馃敟 璋冭瘯锛氭墦鍗板綋鍓嶆爤鎸囬�?
        uint32_t current_esp;
        uint32_t current_ebp;
        __asm__ volatile("movl %%esp, %0" : "=r"(current_esp));
        __asm__ volatile("movl %%ebp, %0" : "=r"(current_ebp));
        printf("[DEBUG] Current ESP=0x%x, EBP=0x%x\n", current_esp, current_ebp);
        printf("[FPU] Re-initialized before STI\n");

        // 鍚敤鍏ㄥ眬涓柇锛堥噸瑕侊紒USB 榧犳爣闇€瑕佷腑鏂�?
        /* Keep every legacy PIC source masked while the IDT and kernel
         * runtime are still coming up. On AC power, a pending firmware
         * power/SCI event can otherwise arrive immediately after STI. */
        /* BIOS leaves the 8259 PIC at vectors 0x08/0x70. Remap it before
         * STI so a legacy IRQ cannot enter a CPU exception vector. */
        outb(0x20, 0x11);
        outb(0x80, 0);
        outb(0xA0, 0x11);
        outb(0x80, 0);
        outb(0x21, 0x20);
        outb(0x80, 0);
        outb(0xA1, 0x28);
        outb(0x80, 0);
        outb(0x21, 0x04);
        outb(0x80, 0);
        outb(0xA1, 0x02);
        outb(0x80, 0);
        outb(0x21, 0x01);
        outb(0x80, 0);
        outb(0xA1, 0x01);
        outb(0x80, 0);
        outb(0x21, 0xFF);
        outb(0xA1, 0xFF);

        // 鍦ㄥ惎鐢ㄤ腑鏂悗鍒濆鍖栭敭鐩橀┍鍔?
        extern void keyboard_init(void);
        keyboard_init();
        printf("Keyboard driver initialized\n");

        // 鈿狅笍鈿狅笍鈿狅�?鍏抽敭淇锛氬湪鍚敤涓柇鍚庨噸鏂伴厤缃甈IC
        // 鍘熷洜锛歅IC鍙兘鍦ㄥ垵濮嬪寲杩囩▼涓閲嶇疆
        printf("Re-configuring PIC after enabling interrupts...\n");
        unsigned char mask1_after = inb(0x21);
        printf("PIC mask before keyboard enable: 0x%x\n", mask1_after);
        mask1_after &= 0xFD;  // 娓呴櫎bit 1 (IRQ1)锛屼娇鐢?0xFD = 11111101

        // 鈿狅笍鈿狅笍鈿狅�?寮哄埗浣跨敤鍐呰仈姹囩紪锛岀‘淇?outb 涓嶄細琚紭�?
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a" (mask1_after), "dN" ((uint16_t)0x21)
            : "memory"
        );

        // 鍐嶆楠岃瘉
        unsigned char mask1_final = inb(0x21);
        printf("PIC mask after keyboard enable: 0x%x (expected: 0x%x)\n", mask1_final, mask1_after);
        printf("IRQ1 (keyboard) %s\n", (mask1_final & 0x02) ? "DISABLED" : "ENABLED");

        // 濡傛灉杩樻槸娌″彉锛岃鏄?outb 瀹屽叏涓嶅伐�?
        if (mask1_final == mask1_after && (mask1_final & 0x02)) {
            printf("鈿狅笍鈿狅笍鈿狅�?WARNING: outb() is not working! PIC mask unchanged!\n");
        } else if (mask1_final != mask1_after) {
            printf("鈿狅笍鈿狅笍鈿狅�?WARNING: PIC mask changed unexpectedly!\n");
        }

        // 馃敟馃敟馃敟 鍏抽敭淇锛氱鐢?8254 PIT 瀹氭椂鍣?
        // PIT 鐨勮緭鍑鸿繛鎺ュ�?IRQ 0锛屽嵆浣?LAPIC Timer 琚鐢紝PIT 浠嶄細瑙﹀彂涓柇
        /* Interrupt handlers and controller masks are now configured. */
        boot_color(7, 0x00FFFFFF); /* 7 white: ready to enable interrupts */
        __asm__ volatile("sti; nop" ::: "memory");
        printf("Global interrupts ENABLED\n");

        printf("Disabling 8254 PIT Timer...\n");
        // 璇诲彇褰撳墠 PIT 閰嶇�?
        unsigned char pit_ctrl = inb(0x43);
        printf("PIT control port (0x43): 0x%x\n", pit_ctrl);

        // 绂佺�?PIT 閫氶�?0锛堣繛鎺ュ埌 IRQ 0�?
        // 鍐欏叆鎺у埗瀛楋細閫氶亾 0锛屼綆瀛楄�?楂樺瓧鑺傦紝妯″紡 0锛堝崟娆′腑鏂�?
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a" ((unsigned char)0x30), "dN" ((uint16_t)0x43)  // 0x30 = 00110000b
            : "memory"
        );

        // 璁剧疆璁℃暟鍣ㄤ�?0锛堝仠姝㈣鏁帮�?
        __asm__ volatile (
            "outb %0, %1\n"
            "outb %0, %1"
            :
            : "a" ((unsigned char)0x00), "dN" ((uint16_t)0x40)
            : "memory"
        );

        // 鍐嶆璇诲彇 PIT 鎺у埗绔彛楠岃瘉
        unsigned char pit_ctrl_after = inb(0x43);
        printf("PIT control after disable: 0x%x\n", pit_ctrl_after);
        printf("8254 PIT Timer disabled 鉁匼n");

        // 馃敟馃敟馃敟 棰濆淇濋櫓锛氱‘淇?LAPIC Timer 涔熷畬鍏ㄧ�?
        // 娉ㄩ噴鎺?�?鐗╃悊鏈哄啓 LAPIC timer 瀵勫瓨鍣ㄥ彲鑳藉崱浣忥紝�?timer 宸叉�?masked
        /* extern volatile uint32_t* lapic;
        if (lapic) {
            printf("Disabling LAPIC Timer (additional safety)...\n");
            volatile uint32_t *lapic_timer = lapic + 0x320/4;
            volatile uint32_t *lapic_ticr = lapic + 0x380/4;
            uint32_t timer_conf = *lapic_timer;
            printf("LAPIC Timer config: 0x%x\n", timer_conf);
            *lapic_timer = 0x10000;
            *lapic_ticr = 0;
            uint32_t timer_after = *lapic_timer;
            printf("LAPIC Timer after disable: 0x%x\n", timer_after);
            printf("LAPIC Timer disabled 鉁匼n");
        } */

        // 鍒濆鍖栨枃浠剁郴缁?
        extern void fs_init(void);
        extern void vfs_set_root(struct super_block *sb);
        extern struct super_block *ramfs_mount(void);

        printf("Initializing file system...\n");
        fs_init();  // 杩欎細璋冪敤 ramfs_mount 骞惰缃牴鏂囦欢绯荤粺
        printf("File system initialized\n");

        // 鍒濆鍖?PCI 鎬荤�?
        extern int pci_init(void);
        printf("Initializing PCI...\n");
        pci_init();
        printf("PCI initialized\n");

        // USB init moved to after network (so UDP log works)
        // Save klog to ramfs so editor can open /kern.log
        extern void klog_save_to_ramfs(void);
        klog_save_to_ramfs();

#if BOOT_AUTO_USB
        {
            extern int usb_init(void);
            int usb_ret;
            usb_boot_status_screen("sync", "kernel usb_init starting");
            printf("Initializing USB synchronously in kernel...\n");
            usb_ret = usb_init();
            printf("Kernel USB synchronous init returned %d state=%d\n", usb_ret, usb_get_init_state());
            usb_boot_status_refresh();
            if (usb_get_init_state() != 4)
                panic("USB init did not complete in kernel usb_init");
        }
#else
        printf("[usb] hardware initialization deferred; continuing without USB\n");
        boot_color(5, 0x000000FF); /* 5 blue: safe boot passed USB stage */
#endif

        /* USB stack initialized */
        /* 鐢ㄦ埛鎺у埗鍙拌緭�?*/
        {
            extern void *console_get_buf(void);
            extern int console_get_len(void);
            struct inode *root = path_lookup("/");
            if (root) {
                struct dentry *cd;
                extern int ramfs_create(struct inode*, const char*, int, struct dentry**);
                if (ramfs_create(root, "console.log", 0644|S_IFREG, &cd) == 0 && cd) {
                    cd->d_inode->i_data = console_get_buf();
                    cd->d_inode->i_size = 8192;
                    cd->d_inode->i_nlink = 99;
                    printf("[fs] Created /console.log\n");
                }
            }
        }

        // ---- �?multiboot 妯″潡瀵煎�?ramfs ----
        // 閬嶅巻鎵€鏈夋ā鍧楁爣�? �?cmdline 浣滀负璺緞, 妯″潡鏁版嵁浣滀负鏂囦欢鍐呭�?
        {
            extern uint32_t multiboot2_info_addr;
            extern void *map_highmem_physical(uint32_t, uint32_t, uint32_t);
            extern struct inode *ramfs_alloc_inode(struct super_block*, int);
            extern struct dentry *d_alloc(struct inode*, const char*);
            extern void d_add(struct dentry*, struct inode*);
            extern void *kmalloc(unsigned int);
            extern void kfree(void*);

            // 纭�?/boot 鐩綍瀛樺�?
            {
                struct inode *root = path_lookup("/");
                if (root && !path_lookup("/boot")) {
                    extern int ramfs_mkdir(struct inode*, const char*, int);
                    ramfs_mkdir(root, "boot", 0755);
                    printf("[ramfs] Created /boot directory\n");
                }
            }

            printf("[ramfs] Module import: multiboot2_info_addr=0x%x\n", multiboot2_info_addr);
            if (multiboot2_info_addr) {
                uint32_t *mb_ptr = (uint32_t*)phys_to_virt(multiboot2_info_addr);
                uint32_t total_size = *mb_ptr;
                multiboot_tag_t *tag = (multiboot_tag_t*)((uint8_t*)mb_ptr + 8);
                printf("[ramfs] Module import: total_size=%u, first_tag at %p type=%u\n",
                       total_size, (void*)tag, tag->type);

                while (tag->type != MULTIBOOT_TAG_TYPE_END) {
                    printf("[ramfs] Module import: tag type=%u size=%u\n", tag->type, tag->size);
                    if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
                        multiboot_tag_module_t *mod = (multiboot_tag_module_t*)tag;
                        // cmdline 绱ц窡鍦ㄦā鍧楀ご閮ㄧ粨鏋勪箣鍚?
                        char *cmdline = (char*)mod + sizeof(multiboot_tag_module_t);
                        uint8_t *raw = (uint8_t*)cmdline;
                        printf("[ramfs] Module: start=0x%x end=0x%x size=%u sizeof=%u\n",
                               mod->mod_start, mod->mod_end,
                               mod->mod_end - mod->mod_start,
                               (uint32_t)sizeof(multiboot_tag_module_t));
                        printf("[ramfs] Module: cmdline at %p, first 32 bytes:",
                               (void*)cmdline);
                        for (int di = 0; di < 32; di++) printf(" %02x", raw[di]);
                        printf("\n");

                        if (cmdline && cmdline[0]) {
                            uint32_t mod_size = mod->mod_end - mod->mod_start;
                            printf("[ramfs] Importing module: '%s' (%u bytes)\n",
                                   cmdline, mod_size);

                            // 瑙ｆ瀽瀹屾暣璺緞: "java/classes/HelloWorld.class"
                            // 鍘绘帀鏈熬鐨勭┖�?鍙傛�?
                            char path[256];
                            int pi = 0;
                            for (char *p = cmdline; *p && *p != ' ' && pi < 254; p++)
                                path[pi++] = *p;
                            path[pi] = 0;

                            if (pi == 0) goto next_module;

                            // 閫愮骇鍒涘缓鐩�?
                            struct inode *dir = path_lookup("/");
                            if (!dir) goto next_module;

                            char *part = path;
                            char *next = part;
                            while (*next) {
                                if (*next == '/') {
                                    *next = 0;
                                    if (*(next + 1)) {  // 闈炴湯灏? 鍒涘缓鐩綍
                                        struct inode *sub = path_lookup(part[0]=='/' ? part+1 : part); // TODO: 绠€�?
                                        // 鐩存帴浣跨敤瀹屾暣璺緞閫愮�?mkdir
                                    }
                                    *next = '/';
                                }
                                next++;
                            }

                            // 绠€�? 浠庢牴寮€濮? 閫愮骇鍒涘缓璺緞涓殑鐩�?
                            {
                                char tmp[256];
                                int ti = 0;
                                dir = path_lookup("/");
                                char *s = path;
                                if (*s == '/') s++; // skip leading /

                                while (*s && dir) {
                                    // 澶嶅埗鍒颁笅涓€�?/
                                    ti = 0;
                                    while (s[ti] && s[ti] != '/') { tmp[ti] = s[ti]; ti++; }
                                    tmp[ti] = 0;

                                    if (s[ti] == '/') {
                                        // 杩欐槸鐩綍
                                        struct inode *sub = path_lookup(tmp); // FIXME: 闇€瑕佸畬鏁磋矾�?
                                        // 鐢ㄧ畝鏄撴柟�? �?dir 涓嬫煡鎵炬垨鍒涘�?
                                        struct dentry *de;
                                        extern int ramfs_lookup(struct inode*, const char*, struct dentry**);
                                        if (ramfs_lookup(dir, tmp, &de) == 0 && de && de->d_inode) {
                                            dir = de->d_inode;
                                        } else {
                                            extern int ramfs_mkdir(struct inode*, const char*, int);
                                            ramfs_mkdir(dir, tmp, 0755);
                                            // 閲嶆柊lookup
                                            struct dentry *de2;
                                            if (ramfs_lookup(dir, tmp, &de2) == 0 && de2 && de2->d_inode) {
                                                dir = de2->d_inode;
                                            } else {
                                                dir = NULL;
                                            }
                                        }
                                        s += ti + 1;
                                    } else {
                                        // 杩欐槸鏂囦欢�?�?鍒涘缓鏂囦欢
                                        struct dentry *de;
                                        extern int ramfs_create(struct inode*, const char*, int, struct dentry**);
                                        if (ramfs_create(dir, tmp, 0644|S_IFREG, &de) == 0 && de) {
                                            de->d_inode->i_data = phys_to_virt(mod->mod_start);
                                            de->d_inode->i_size = mod_size;
                                            de->d_inode->i_nlink = 2;
                                            printf("[ramfs] Imported /%s (%u bytes)\n", path, mod_size);
                                        } else {
                                            printf("[ramfs] Failed to create /%s\n", path);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        next_module:;
                    }
                    tag = (multiboot_tag_t*)((uint8_t*)tag + ((tag->size + 7) & ~7));
                }
            }
            printf("[ramfs] Module import done\n");
        }

        // PS/2 touchpad hangs on some systems �?disabled
        // extern int ps2mouse_init(void); ps2mouse_init();
        // USB mouse/MSC tests moved to after USB init

        // 鍒濆鍖栫綉缁滃崗璁爤
        extern void net_init(void);
        extern int loopback_init(void);
        extern int loopback_send_test(void);
        net_init();
        loopback_init();
        /* 鑷姩鍒濆鍖栫綉鍗?�?klog UDP闇€瑕佺綉缁? 鐩存帴鎵弿PCI */
        int network_online = 0;
#if BOOT_AUTO_NETWORK
        {
            extern int e1000_init(const char *dev_name);
            printf("[klog] auto-initializing network...\n");
            printf("USB tests deferred until init completes\n");
        }
        // 鍙戦€佺綉缁滄祴璇曞�?
#else
        printf("[net] hardware network initialization deferred\n");
#endif

        printf("\n=== Network Test ===\n");

        // 馃敟 妫€鏌ュ唴鏍告爤婧㈠�?
        extern uint32_t stack_base;
        if (*(uint32_t*)&stack_base != 0xDEADBEEF) {
            printf("鈿狅笍鈿狅笍鈿狅�?WARNING: Stack overflow detected! Sentinel corrupted!\n");
            printf("Expected 0xDEADBEEF, got 0x%x\n", *(uint32_t*)&stack_base);
        } else {
            printf("�?Stack sentinel OK\n");
        }

        loopback_send_test();
        printf("=== Network Test Complete ===\n\n");

        // 鈿狅笍鈿狅笍鈿狅�?娉ㄩ噴鎺?WiFi 鑷姩娴嬭瘯锛岄伩鍏嶆湭鍒濆鍖栬闂鑷撮噸�?
        // WiFi 鍒濆鍖栧繀椤荤敱鐢ㄦ埛绋嬪簭閫氳繃 syscall 鎵嬪姩瑙﹀�?
        /*
        // WiFi 鏁版嵁鍖呮祴�?
        extern int wifi_send_test_packet(void);
        extern void wifi_show_stats(void);
        printf("\n=== WiFi Data Packet Test ===\n");
        wifi_show_stats();
        printf("\nSending test packet...\n");
        wifi_send_test_packet();
        printf("=== WiFi Test Complete ===\n\n");
        */

        // 鈿狅笍鈿狅笍鈿狅�?娉ㄦ剰锛歅IC宸茬粡鍦ㄥ惎鐢ㄤ腑鏂悗閰嶇疆瀹屾�?
        // 涓嶈鍦ㄨ繖閲岄噸澶嶉厤缃紝閬垮厤瑕嗙洊涔嬪墠鐨勮缃?

        // 鍦╒GA涓婃樉绀烘祴璇曟秷鎭紝纭绯荤粺姝ｅ父杩愯
        volatile uint16_t* vga = (volatile uint16_t*)0xC00B8000;
        vga[10] = (0x0E << 8) | 'T';
        vga[11] = (0x0E << 8) | 'E';
        vga[12] = (0x0E << 8) | 'S';
        vga[13] = (0x0E << 8) | 'T';
        printf("VGA test: wrote TEST to screen at position 10-13\n");

        // 瀹為�?锛氭娴嬪苟鍒濆鍖?VBE/Framebuffer锛堜�?Multiboot2 璇诲彇锛?
        printf("\n=== VBE/Framebuffer Initialization ===\n");

        // 閬嶅�?multiboot2 鏍囩鏌ユ壘 framebuffer 淇℃�?
        {
            // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
            uint32_t *fb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);
            multiboot_tag_t *fb_tag = (multiboot_tag_t *)((uint8_t *)fb_info_ptr + 8);
            int found_framebuffer = 0;

            while (fb_tag->type != MULTIBOOT_TAG_TYPE_END) {
                if (fb_tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                    multiboot_tag_framebuffer_t *fb = (multiboot_tag_framebuffer_t *)fb_tag;
                    printf("�?Framebuffer info available!\n");
                    printf("=== Framebuffer Tag Structure (Hex Dump) ===\n");

                    // 鎵撳嵃鏁翠釜缁撴瀯浣撶殑鍗佸叚杩涘埗dump
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

                    // 鎵嬪姩瑙ｆ瀽姣忎釜瀛楁鏉ラ獙�?
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

                    // 缁撴瀯浣撹В鏋愬€?
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

                    // 鈿狅�?涓存椂淇锛氭墜鍔ㄨВ鏋愬瓧�?閬垮厤缁撴瀯浣撳榻愰棶棰?
                    uint64_t fb_addr = *(uint64_t*)&fb_bytes[8];
                    uint32_t fb_pitch = *(uint32_t*)&fb_bytes[16];
                    uint32_t fb_width = *(uint32_t*)&fb_bytes[20];
                    uint32_t fb_height = *(uint32_t*)&fb_bytes[24];
                    uint8_t fb_bpp = fb_bytes[28];
                    uint8_t fb_type = fb_bytes[29];

                    printf("�?Manual parsing successful!\n");
                    printf("  Parsed values: addr_hi=0x%x addr_lo=0x%x, %dx%d, bpp=%d, type=%d\n",
                           (uint32_t)(fb_addr >> 32), (uint32_t)fb_addr,
                           fb_width, fb_height, fb_bpp, fb_type);

                    // 妫€娴嬫槸鍚︿负鏂囨湰妯″紡 (浣跨敤鎵嬪姩瑙ｆ瀽鐨勬纭�?
                    if (fb_type != 1) {  // 蹇呴』鏄?RGB 鍥惧舰妯″紡
                        printf("�?ERROR: Not RGB framebuffer mode!\n");
                        printf("  framebuffer_type=%d (must be 1 for RGB)\n", fb_type);
                        printf("  Type meanings:\n");
                        printf("    0 = Indexed color\n");
                        printf("    1 = RGB (required for graphics) 鉁匼n");
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

                    // 棰濆妫€鏌ワ細bpp �?pitch 蹇呴』鏈夋晥
                    if (fb_bpp == 0 || fb_pitch == 0) {
                        printf("�?ERROR: Invalid framebuffer parameters!\n");
                        printf("  bpp=%d (must be > 0)\n", fb_bpp);
                        printf("  pitch=%d (must be > 0)\n", fb_pitch);
                        found_framebuffer = 0;
                        break;
                    }

                    printf("�?RGB framebuffer mode detected!\n");

                    found_framebuffer = 1;

                    // 鍒濆鍖?VBE 椹卞姩锛堜娇鐢ㄦ墜鍔ㄨВ鏋愮殑姝ｇ‘鍊硷級
                    extern void vbe_init_from_multiboot(uint64_t fb_addr, uint32_t width,
                                                        uint32_t height, uint32_t pitch, uint8_t bpp);
                    vbe_init_from_multiboot(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
                    printf("�?VBE driver initialized from Multiboot2 info\n");

                    // 馃敟 鍐呮牳鎵ц杩涘害鎸囩�?
                    // 闃舵�?锛歏BE鍒濆鍖栨垚�?- 鏄剧ず缁胯壊�?
                    if (vbe_is_available()) {
                        volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
                        uint16_t w, h;
                        vbe_get_resolution(&w, &h);
                        uint16_t pitch = vbe_get_pitch();
                        uint32_t pitch_pixels = pitch / 4;

                        // 缁樺埗灏忕殑缁胯壊鏂瑰潡鍦ㄥ彸涓婅锛岃〃绀篤BE鍒濆鍖栨垚�?
                        for (int y = 0; y < 30; y++) {
                            for (int x = w - 30; x < w; x++) {
                                fb[y * pitch_pixels + x] = 0xFF00FF00;  // 缁胯�?
                            }
                        }
                        printf("�?Progress indicator: VBE initialized (green square)\n");
                    }
                    break;
                }
                fb_tag = (multiboot_tag_t *)((uint8_t *)fb_tag + ((fb_tag->size + 7) & ~7));
            }

            if (!found_framebuffer) {
                printf("�?No valid framebuffer info available from GRUB\n");
                printf("  Note: GRUB did not provide RGB framebuffer (type=1)\n");
                printf("\n");
                printf("�?WARNING: GUI functions will NOT work!\n");
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

        // 鈿狅笍鈿狅笍鈿狅�?涓存椂淇锛氭敞閲婃帀 PID=1 鍐呮牳浠诲姟锛岄伩鍏嶅垏鎹㈠洖瀹冩椂宕╂簝
        // 鍘熷洜锛歅ID=1 鍐呮牳浠诲姟鐨勬爤绌洪棿涓嶈冻锛屽垏鎹㈠洖瀹冩椂浼氳Е鍙?triple fault
        // 璇﹁锛歅ID1_CRASH_FIX.md
        // task_t *th_k=init_task(0);
        // printf("start kernel task\n");
        // start_task_kernel(th_k,kernel_task_main);

        // 鐩存帴鍒涘缓鐢ㄦ埛浠诲姟浣滀负绗竴涓换鍔?
        task_t *th_u=init_task(1);

        // 娉ㄩ噴鎺塳malloc娴嬭�?閬垮厤褰卞搷鐢ㄦ埛杩涚▼鍔犺�?
        /*
        // 杈撳嚭鍐呭瓨妫€娴嬬粨鏋?
        print_memory_detection_result();

        // 杈撳�?PMM 缁熻淇℃伅
        pmm_print_stats();

        // 娴嬭�?kmalloc �?kfree 鍔熻�?
        printf("\n=== Testing kmalloc/kfree functionality ===\n");

        // 娴嬭�?1: 灏忓唴瀛樺垎閰嶏紙浣跨�?early pool�?
        printf("\nTest 1: Small allocation (early pool)\n");
        void *ptr1 = kmalloc(128);
        printf("  Allocated 128 bytes at: 0x%x\n", (uint32_t)ptr1);

        // 娴嬭�?2: 涓瓑鍐呭瓨鍒嗛厤锛堜娇�?early pool�?
        printf("\nTest 2: Medium allocation (early pool)\n");
        void *ptr2 = kmalloc(1024);
        printf("  Allocated 1024 bytes at: 0x%x\n", (uint32_t)ptr2);

        // 娴嬭�?3: 澶у唴瀛樺垎閰嶏紙浣跨�?PMM�?
        printf("\nTest 3: Large allocation (PMM)\n");
        void *ptr3 = kmalloc(8192);  // 8KB = 2 pages
        printf("  Allocated 8192 bytes at: 0x%x\n", (uint32_t)ptr3);

        // 娴嬭�?4: 瓒呭ぇ鍐呭瓨鍒嗛厤锛堜娇�?PMM�?
        printf("\nTest 4: Extra large allocation (PMM)\n");
        void *ptr4 = kmalloc(16384);  // 16KB = 4 pages
        printf("  Allocated 16384 bytes at: 0x%x\n", (uint32_t)ptr4);

        // 娴嬭�?5: kzalloc 鍒嗛厤骞舵竻�?
        printf("\nTest 5: kzalloc (zero-initialized)\n");
        void *ptr5 = kzalloc(512);
        printf("  Allocated 512 zero-initialized bytes at: 0x%x\n", (uint32_t)ptr5);

        // 鏄剧ず鍒嗛厤缁熻�?
        printf("\n");
        kmalloc_print_stats();

        // 娴嬭�?6: kfree 娴嬭�?
        printf("\nTest 6: Testing kfree\n");
        printf("  Freeing ptr2 (1024 bytes)\n");
        kfree(ptr2);

        printf("  Freeing ptr4 (16384 bytes)\n");
        kfree(ptr4);

        // 鍐嶆鏄剧ず缁熻�?
        printf("\nAfter freeing:\n");
        kmalloc_print_stats();

        printf("=== kmalloc/kfree tests completed ===\n\n");
        */

        // 馃敟 鍐呮牳鎵ц杩涘害鎸囩�?
        // 闃舵�?锛氬噯澶囧惎鍔ㄧ敤鎴蜂换�?- 鏄剧ず钃濊壊�?
        if (vbe_is_available()) {
            volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
            uint16_t w, h;
            vbe_get_resolution(&w, &h);
            uint16_t pitch = vbe_get_pitch();
            uint32_t pitch_pixels = pitch / 4;

            // 缁樺埗灏忕殑钃濊壊鏂瑰潡鍦ㄥ乏涓嬭锛岃〃绀哄噯澶囧惎鍔ㄧ敤鎴蜂换鍔?
            for (int y = h - 30; y < h; y++) {
                for (int x = 0; x < 30; x++) {
                    fb[y * pitch_pixels + x] = 0xFF0000FF;  // 钃濊�?
                }
            }
            printf("�?Progress indicator: Ready to start user task (blue square)\n");
        }

        // 鍚姩鐢ㄦ埛杩涚�?
        printf("start user task \n");
        // 鍚姩鐢ㄦ埛杩涚�?
        printf("start user task \n");

        // 璋冭瘯锛氳緭鍑簃ultiboot2妯″潡淇℃伅锛堝凡绂佺敤锛岄伩鍏嶇郴缁熷穿婧冿�?
        // dump_multiboot2_modules(mb_info_addr);

        // 閲嶈锛氬厛鎵嬪姩璋冪敤 user_task_main 杩涜鍒濆鍖栵紙椤佃〃銆佹ā鍧楀姞杞界瓑�?
        // 鍒濆鍖栧畬鎴愬悗锛寀ser_task_main 浼氳繑鍥?
        user_task_main(th_u);

        // 鍒濆鍖栧畬鎴愬悗锛屽皢鐢ㄦ埛浠诲姟娣诲姞鍒拌皟搴﹀櫒闃熷垪
        // start_task 浼氬�?th_u 娣诲姞鍒?sched_root 閾捐〃锛岃繖鏍疯皟搴﹀櫒鎵嶈兘鎵惧埌�?
        start_task(th_u, user_task_main);

        // 璁剧疆浠诲姟鐘舵€佷�?PS_CREATED锛岃璋冨害鍣ㄧ煡閬撻渶瑕佽烦杞埌鐢ㄦ埛�?
        th_u->state = PS_CREATED;
        printf("[kernel_main] User task initialized, state=PS_CREATED\n");

        // 馃敟 鍐呮牳鎵ц杩涘害鎸囩�?
        // 闃舵�?锛氱敤鎴蜂换鍔″惎鍔ㄦ垚鍔?- 鏄剧ず榛勮壊�?
        if (vbe_is_available()) {
            volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
            uint16_t w, h;
            vbe_get_resolution(&w, &h);
            uint16_t pitch = vbe_get_pitch();
            uint32_t pitch_pixels = pitch / 4;

            // 缁樺埗灏忕殑榛勮壊鏂瑰潡鍦ㄥ彸涓嬭锛岃〃绀虹敤鎴蜂换鍔″惎鍔ㄦ垚鍔?
            for (int y = h - 30; y < h; y++) {
                for (int x = w - 30; x < w; x++) {
                    fb[y * pitch_pixels + x] = 0xFFFFFF00;  // 榛勮�?
                }
            }
            printf("�?Progress indicator: User task ready (yellow square)\n");
        }

        /*
        // 鍒涘缓绗簩涓敤鎴疯繘绋嬶紙娴嬭瘯璋冨害锛? 宸插純鐢?
        // 鐜板湪浣跨敤 fork() 绯荤粺璋冪敤鏇夸�?
        printf("=== Creating second user task for scheduler testing ===\n");
        printf("th_u = 0x%x, th_u->pde = 0x%x, th_u->cr3 = 0x%x\n",
               th_u, th_u->pde, th_u->cr3);

        task_t *th_u2 = init_task(true);  // 鍒涘缓绗簩涓换鍔?
        if (th_u2) {
            printf("th_u2 created: 0x%x\n", th_u2);

            // 瀹炵幇绠€鍗曠殑 fork 鏈哄埗锛氫负绗簩涓繘绋嬪垱寤虹嫭绔嬬殑椤电洰�?
            // 鍙傝€冿�?
            // - Brown University CS: "fork() clones user-space processes with full address space copies"
            // - Reddit r/osdev: "fork() and copy-on-write in Linux"
            //
            // 鍏抽敭鐐癸細
            // 1. 姣忎釜杩涚▼闇€瑕佺嫭绔嬬殑椤电洰褰曪紙CR3�?
            // 2. 椤电洰褰曢」鎸囧悜鐩稿悓鐨勭墿鐞嗛〉锛堝叡浜唴瀛橈�?
            // 3. 鍐呮牳鏄犲皠�?xC0000000+锛夐渶瑕佸鍒跺埌姣忎釜椤电洰褰?

            extern page_t alloc_page_table_();
            extern void copy_kernel_mappings_to_pd(uint32_t *pd_user);

            extern uint32_t pmm_alloc_page(void);

            // 1. 鍒嗛厤鏂扮殑椤电洰褰?
            page_t pde2 = alloc_page_table_();
            th_u2->pde = (uint32_t*)pde2.phys;
            th_u2->cr3 = (uint32_t*)pde2.phys;  // CR3 瀛楁瀛樺偍鐗╃悊鍦板�?
            uint32_t *pd_user2 = pde2.virt;

            printf("[fork] Allocated new page dir: phys=0x%x, virt=0x%x\n",
                   pde2.phys, pd_user2);

            // 2. 澶嶅埗鍐呮牳楂樼鏄犲皠�?68-1023锛屽�?0xC0000000-0xFFFFFFFF�?
            // 杩欐牱鍐呮牳浠ｇ爜鍦ㄦ墍鏈夎繘绋嬩腑閮藉彲璁块棶
            copy_kernel_mappings_to_pd(pd_user2);
            printf("[fork] Copied kernel mappings to new page dir\n");

            // 3. 澶嶅埗鐢ㄦ埛绌洪棿鏄犲皠�?-767锛屽�?0x00000000-0xBFFFFFFF�?
            // 杩欐槸绠€鍗曠殑椤佃〃澶嶅埗锛屼笉鏄?Copy-on-Write
            // 涓や釜杩涚▼浼氬叡浜浉鍚岀殑鐗╃悊椤碉紝浣嗘湁鐙珛鐨勯〉琛ㄧ粨�?

            // 鑾峰彇绗竴涓换鍔＄殑椤电洰褰曡櫄鎷熷湴鍧�?
            extern uint32_t pd[];
            uint32_t *pd_user1 = (uint32_t*)phys_to_virt((uint32_t)th_u->pde);

            int copied_entries = 0;
            for (int i = 0; i < 768; i++) {  // 鐢ㄦ埛绌洪棿�?-767
                if (pd_user1[i] & PAGE_PRESENT) {
                    pd_user2[i] = pd_user1[i];  // 澶嶅埗椤佃〃椤规寚�?
                    copied_entries++;
                }
            }
            printf("[fork] Copied %d user space page table entries\n", copied_entries);

            // 4. 鍒嗛厤鏂扮殑鍐呮牳鏍?
            uint32_t kstack2 = pmm_alloc_page();
            th_u2->kstack = (uint32_t*)kstack2;
            th_u2->esp0 = (uint32_t)(kstack2 + PAGE_SIZE);

            printf("[fork] Task2: pde=0x%x, cr3=0x%x, kstack=0x%x\n",
                   th_u2->pde, th_u2->cr3, th_u2->kstack);

            // 5. 澶嶅�?trapframe
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

        // 鍚姩璋冨害�?
        // printf("Starting scheduler with multiple tasks...\n");
        efficient_scheduler_loop();
        // printf("Kernel main completed successfully!\n");
	return (42);
}

/*
 * usb_periodic_poll_callback �?called from the scheduler idle loop
 * (handle_idle_state in task.c) to service USB host controllers.
 *
 * Replaces the old fork_poller() hack that tried to spawn a kernel
 * thread via start_kernel_task (which is a stub).
 */
void usb_periodic_poll_callback(void)
{
    extern int usb_get_init_state(void);
    extern void usb_process_deferred_clears(void);
    extern int usb_hcd_poll_hotplug(int controller_id);
    extern int num_uhci_controllers;
    static uint32_t usb_idle_div;
    static uint32_t usb_hotplug_div;

    /* Advance USB bring-up only through the kernel gateway. */
    if ((++usb_idle_div & 3) == 0)
        kernel_usb_init_service();
    else
        usb_boot_status_refresh();

    /* During xHCI startup/root-port enumeration, keep the idle hook tiny.
     * Hotplug and interrupt polling are enabled after init reaches the
     * steady state, so GUI input cannot be starved by USB bring-up. */
    if (usb_get_init_state() > 0 && usb_get_init_state() < 4)
        return;

    /* Phase 1: send CLEAR_FEATURE for any STALLed endpoints.
     * Must run BEFORE HC poll so the device-side halt is cleared
     * before the HC re-arms the transfer ring and sends IN tokens. */
    usb_process_deferred_clears();

    /* Hotplug / interrupt polling is useful, but it does not need to run
     * on every single idle iteration.  Throttle it a bit so PS/2 input and
     * the rest of the scheduler have more breathing room on physical HW. */
    if ((++usb_hotplug_div & 7) != 0)
        return;

    int max_ctrl = num_uhci_controllers;
    if (max_ctrl < 1) max_ctrl = 1;

    for (int i = 0; i < max_ctrl; i++)
        usb_hcd_poll_hotplug(i);
}

