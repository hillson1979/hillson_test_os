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
#include "graph/selftest.h"

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

/* æžî„å´?multiboot2 é—â•ƒæ‚Šé¦æ¿æ½ƒé’æ‹Œæ«„éŽ·ç†·æ¹´é§â‚¬ */
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

    // é–¬å¶…å·»éŽµâ‚¬éˆå¤‹çˆ£ç»?
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
        // ç»‰è¯²å§©é’é¢ç¬…æ¶“â‚¬æ¶“î…çˆ£ç»›?(æ¾¶Ñƒçš¬è¹‡å‘´ã€é„?ç€›æ¥„å¦­ç€µå½’ç¶?
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

static int boot_log_contains_ci(const char *text, const char *word)
{
        if (!text || !word || !*word)
                return 0;
        for (; *text; text++) {
                const char *a = text;
                const char *b = word;
                while (*a && *b) {
                        char ca = *a;
                        char cb = *b;
                        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + ('a' - 'A'));
                        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + ('a' - 'A'));
                        if (ca != cb) break;
                        a++; b++;
                }
                if (!*b) return 1;
        }
        return 0;
}
void kernel_usb_status_line(const char *stage, const char *detail)
{
        extern int usb_get_init_state(void);
        uint32_t saved_text_color = usb_boot_text_color;
        if (!USB_RUNTIME_SCREEN_DEBUG && usb_get_init_state() == 4)
                return;
        if (boot_log_contains_ci(detail, "not ready") ||
            boot_log_contains_ci(detail, "not-ready") ||
            boot_log_contains_ci(detail, "failed") ||
            boot_log_contains_ci(detail, "failure") ||
            boot_log_contains_ci(detail, "error") ||
            boot_log_contains_ci(detail, "timeout"))
                usb_boot_text_color = 0x00CC0000;
        else if (boot_log_contains_ci(detail, "ok") ||
                 boot_log_contains_ci(detail, "success") ||
                 boot_log_contains_ci(detail, "ready") ||
                 boot_log_contains_ci(detail, "complete"))
                usb_boot_text_color = 0x00008000;
        else
                usb_boot_text_color = 0x00000000;
        usb_boot_status_screen(stage, detail);
        usb_boot_sticky_error(stage, detail);
        usb_boot_text_color = saved_text_color;
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
                if (state >= 3 || state < 0)
                        break;
                usb_boot_delay(1000);
        }

        state = usb_get_init_state();
        printf("USB controller boot wait finished, state=%d; port enumeration deferred\n", state);
        if (state < 3) {
                usb_boot_status_screen("panic", "USB controller init did not complete in kernel");
                panic("USB controller init did not complete in kernel boot");
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
        // é¦ƒæ•Ÿ éªç†¸æº€ç’‹å†­ç˜¯é”›æ°­æ¸¶éƒï¿¢GAæˆæ’³åš­é”›å±½å•“é¦â•²ga_initå¨“å‘­ç†æ¶”å¬ªå¢ 
        volatile uint16_t *vega = (volatile uint16_t *)0xC00B8000;
        for (int i = 0; i < 80*25; i++) vega[i] = (0x2F << 8) | ' ';
        vega[0] = (0x4F << 8) | 'K';
        vega[1] = (0x4F << 8) | 'O';
        vega[2] = (0x4F << 8) | 'K';


        // é¦ƒæ•Ÿ éå‘®ç‰³éå Ÿå­©é‘çƒ˜î—…å¨´å¬¶ç´°é¦ã„¦çˆ¤æ´æ›¡î†•ç¼ƒî†¼æ‘ é?
        extern uint32_t stack_base;
        extern uint32_t stack_top;
        uint32_t *stack_sentinel = &stack_base;
        *stack_sentinel = 0xDEADBEEF;  // éã„¥åºéŠ?

        // éˆ¿ç‹…ç¬?æ·‡æ¿†ç“¨éå‘®ç‰³æ¤¤ç”µæ´°è¤°æ› å¢¿éžå——æ¹´é§â‚¬é”›å æ¹ªé’å›¨å´²é’æ‰®æ•¤éŽ´ç–¯ç¹˜ç»‹å¬©ç®£é“å¶ç´?
        extern uint32_t kernel_page_directory_phys;
        uint32_t cr3_value;
        __asm__ volatile("movl %%cr3, %0" : "=r"(cr3_value));
        kernel_page_directory_phys = cr3_value & ~0xFFF;


        //printf("[kernel_main] Saved kernel CR3 phys=0x%x\n", kernel_page_directory_phys);


        // é„å‰§ãšéå‘®ç‰³éå œä¿ŠéŽ­?
        printf("[kernel_main] Stack: base=0x%x top=0x%x size=%u KB\n",
               (uint32_t)&stack_base, (uint32_t)&stack_top,
               ((uint32_t)&stack_top - (uint32_t)&stack_base) / 1024);

        // é¦ƒæ•Ÿ æ¶“å­˜æ¤‚ç»‚ä½ºæ•¤éŽµâ‚¬é?UART éœå±½î˜²é‰å‚šåžµæ¿®å¬ªå¯²é”›å±¾ç¥´ç’‡æ›Ÿæ¸¶çå¿“æƒŽé?
        // uart_init();
        // uart_puts("[UART] Serial port initialized at 115200 baud\n");

        // é¦ƒæ•Ÿ é’æ¿†îé–æ ¦äº’æ¾¶î†ç¶‰ç’‹å†­ç˜¯éŽºãƒ¥å½?
        //netdebug_init();

        

        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted with Multiboot 2!\n");

        // ================================
        // éŽµæ’³åµ?TASK_IFRAME éŠç¡·ç´™ç’‹å†­ç˜¯é‹å¿•Ð©é–²å¿Žç«´é‘·å­˜â‚¬Ñç´?
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

        // æ¥ å²ƒç˜?multiboot2 æ¦„æ—€æš?
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            printf("ERROR: Invalid multiboot2 magic: 0x%x\n", mb_magic);
            // uart_panic("ERROR: Invalid multiboot2 magic!");  // é¦ƒæ•Ÿ å¨‰ã„©å™´éŽºå¤›ç´é–¬åž®åŽ¤éˆî„åžµæ¿®å¬ªå¯²é¨å‹ªè¦†é™ï½ˆî†–é—‚î†¼î‡±é‘·æ’®å™¸éš?
            return -1;
        }

        mpinit();
        // é¦ƒæ•Ÿ éæŠ½æ•­æ·‡î†¼î˜²é”›æ°¬ç¹€æ¤¤è¯²æ¹?init_highmem_mapping() æ¶”å¬ªå¢ ç’å‰§ç–† GDT
        // é¥çŠ±è´?init_highmem_mapping() æµ¼æ°³çšŸé¢?kmalloc_early é?map_4k_page
        // æ©æ¬Žç°ºé‘èŠ¥æšŸç’‹å†ªæ•¤é—‡â‚¬ç‘•ä½¹î„œçº­î†¾æ®‘å¨ˆç”¸â‚¬å¤‹å«¨é?
        

        init_highmem_mapping();
        //acpi_init();

        // æ·‡æ¿†ç“?multiboot2 æ·‡â„ƒä¼?æ¸šæ¶˜å”´ç€›æ¨¼î…¸éžå—•å¨‡é?
        multiboot2_info_addr = mb_info_addr;

        // ç‘™ï½†ç€½é©çƒ˜æ¹°éå‘­ç“¨æ·‡â„ƒä¼…
        uint32_t mem_lower = 0, mem_upper = 0;

        // é?Multiboot 2 æ¶“î…¨ç´ebx éŽ¸å›§æ‚œæ·‡â„ƒä¼…ç¼æ’´ç€¯é¨å‹«ç´‘æ¿®å¬ªæ¹´é§â‚?
        // æ·‡â„ƒä¼…ç¼æ’´ç€¯æµ åº¡ã‡çå¿“ç“§å¨ˆé›ç´‘æ¿?4ç€›æ¥„å¦?é”›å²€åŠ§éšåº¢æ§¸æ·‡æ¿ˆæš€ç€›æ¥î†?4ç€›æ¥„å¦?é”›å±½å•€é’è·ºæ‚—é„îˆ›çˆ£ç»›ç‚¬æšŸç¼?
        uint32_t *mb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);

        // ç»—îƒ¿ç«´æ¶“î…çˆ£ç»›å¥ç² é‹å¿•Ð?ç€›æ¥„å¦­å¯®â‚¬æ¿®?
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
#if GRAPH_ENABLE_SELFTEST
        if (graph_selftest() != 0) {
                printf("[graph] selftest failed\n");
        } else {
                printf("[graph] selftest passed\n");
        }
#endif
        boot_color(1, 0x00FF0000); /* 1 red: memory/framebuffer */

        // è¹‡å‘´ã€é’æ¿†îé?LAPICé”›å±½æ´œæ¶“?logical_cpu_id() æ¸šæ¿Šç¦†ç€?
        lapicinit();

        // é¦ƒæ•Ÿ é’æ¿†îé–?IOAPICé”›å ç¹€æ¤¤è¯²æ¹ªé–¿î†¾æ´é’æ¿†îé–æ ¦ç®£é“å¶ç´’é”›?
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
        // é¦ƒæ•Ÿ ç’‹å†­ç˜¯é”›æ°¬æ¹ª tvinit() é“å¶…æ‚—å¦«â‚¬éŒ?vectors[128]
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

        // é¦ƒæ•Ÿé¦ƒæ•Ÿ é¦ã„¥ç´‘æ¶“î…ŸæŸ‡é“å¶…å•€å¨†ï¼„â€˜æ·‡?FPU å®¸æ’åžµæ¿®å¬ªå¯²é”›å ¥æ§»å§?Trap 19é”?
        __asm__ volatile("fninit");
        __asm__ volatile("fnclex");
        uint32_t cr0_check;
        __asm__ volatile("movl %%cr0, %0" : "=r"(cr0_check));
        cr0_check &= ~(1 << 3);  // å¨“å‘´æ«?TS
        cr0_check &= ~(1 << 2);  // å¨“å‘´æ«?EM
        __asm__ volatile("movl %0, %%cr0" : : "r"(cr0_check));
        boot_color(6, 0x00FF00FF); /* 6 magenta: FPU/CR0 ready */

        // é¦ƒæ•Ÿ ç’‹å†­ç˜¯é”›æ°­å¢¦é—æ¿ç¶‹é“å¶†çˆ¤éŽ¸å›¬æ‹?
        uint32_t current_esp;
        uint32_t current_ebp;
        __asm__ volatile("movl %%esp, %0" : "=r"(current_esp));
        __asm__ volatile("movl %%ebp, %0" : "=r"(current_ebp));
        printf("[DEBUG] Current ESP=0x%x, EBP=0x%x\n", current_esp, current_ebp);
        printf("[FPU] Re-initialized before STI\n");

        // éšîˆœæ•¤éã„¥çœ¬æ¶“î…ŸæŸ‡é”›å ¥å™¸ç‘•ä¾Šç´’USB æ¦§çŠ³çˆ£é—‡â‚¬ç‘•ä½·è…‘é‚î…¨ç´?
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

        // é¦ã„¥æƒŽé¢ã„¤è…‘é‚î…žæ‚—é’æ¿†îé–æ ­æ•­é©æ©€â”é”?
        extern void keyboard_init(void);
        keyboard_init();
        printf("Keyboard driver initialized\n");

        // éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?éæŠ½æ•­æ·‡î†¼î˜²é”›æ°¬æ¹ªéšîˆœæ•¤æ¶“î…ŸæŸ‡éšåº¨å™¸é‚ä¼´åŽ¤ç¼ƒç”ˆIC
        // é˜ç†·æ´œé”›æ­…ICé™îˆå…˜é¦ã„¥åžµæ¿®å¬ªå¯²æ©å›©â–¼æ¶“î…¡î¦é–²å¶‡ç–†
        printf("Re-configuring PIC after enabling interrupts...\n");
        unsigned char mask1_after = inb(0x21);
        printf("PIC mask before keyboard enable: 0x%x\n", mask1_after);
        mask1_after &= 0xFD;  // å¨“å‘´æ«Žbit 1 (IRQ1)é”›å±¼å¨‡é¢?0xFD = 11111101

        // éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?å¯®å“„åŸ—æµ£è·¨æ•¤éå‘°ä»ˆå§¹å›©ç´ªé”›å²€â€˜æ·‡?outb æ¶“å¶„ç´°çšî‚¡ç´­é?
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a" (mask1_after), "dN" ((uint16_t)0x21)
            : "memory"
        );

        // éå¶†î‚¼æ¥ å²ƒç˜‰
        unsigned char mask1_final = inb(0x21);
        printf("PIC mask after keyboard enable: 0x%x (expected: 0x%x)\n", mask1_final, mask1_after);
        printf("IRQ1 (keyboard) %s\n", (mask1_final & 0x02) ? "DISABLED" : "ENABLED");

        // æ¿¡å‚›ç‰æ©æ¨»æ§¸å¨Œâ€³å½‰é”›å²ƒî‡©é„?outb ç€¹å±½åæ¶“å¶…ä¼æµ?
        if (mask1_final == mask1_after && (mask1_final & 0x02)) {
            printf("éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?WARNING: outb() is not working! PIC mask unchanged!\n");
        } else if (mask1_final != mask1_after) {
            printf("éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?WARNING: PIC mask changed unexpectedly!\n");
        }

        // é¦ƒæ•Ÿé¦ƒæ•Ÿé¦ƒæ•Ÿ éæŠ½æ•­æ·‡î†¼î˜²é”›æ°±î›¦é¢?8254 PIT ç€¹æ°­æ¤‚é£?
        // PIT é¨å‹®ç·­é‘é¸¿ç¹›éŽºãƒ¥åŸ?IRQ 0é”›å±½åµ†æµ£?LAPIC Timer çšî‚¤î›¦é¢îŸ’ç´PIT æµ å¶„ç´°ç‘™ï¹€å½‚æ¶“î…ŸæŸ‡
        /* Interrupt handlers and controller masks are now configured. */
        boot_color(7, 0x00FFFFFF); /* 7 white: ready to enable interrupts */
        __asm__ volatile("sti; nop" ::: "memory");
        printf("Global interrupts ENABLED\n");

        printf("Disabling 8254 PIT Timer...\n");
        // ç’‡è¯²å½‡è¤°æ’³å¢  PIT é–°å¶‡ç–?
        unsigned char pit_ctrl = inb(0x43);
        printf("PIT control port (0x43): 0x%x\n", pit_ctrl);

        // ç»‚ä½ºæ•?PIT é–«æ°¶äº?0é”›å £ç¹›éŽºãƒ¥åŸŒ IRQ 0é”?
        // éæ¬å†éŽºÑƒåŸ—ç€›æ¥‹ç´°é–«æ°¶äº¾ 0é”›å±¼ç¶†ç€›æ¥„å¦?æ¥‚æ¨ºç“§é‘ºå‚¦ç´å¦¯â€³ç´¡ 0é”›å å´Ÿå¨†â€²è…‘é‚î…¨ç´?
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a" ((unsigned char)0x30), "dN" ((uint16_t)0x43)  // 0x30 = 00110000b
            : "memory"
        );

        // ç’å‰§ç–†ç’â„ƒæšŸé£ã„¤è´?0é”›å ä» å§ãˆ£î…¸éå¸®ç´?
        __asm__ volatile (
            "outb %0, %1\n"
            "outb %0, %1"
            :
            : "a" ((unsigned char)0x00), "dN" ((uint16_t)0x40)
            : "memory"
        );

        // éå¶†î‚¼ç’‡è¯²å½‡ PIT éŽºÑƒåŸ—ç»”îˆšå½›æ¥ å²ƒç˜‰
        unsigned char pit_ctrl_after = inb(0x43);
        printf("PIT control after disable: 0x%x\n", pit_ctrl_after);
        printf("8254 PIT Timer disabled é‰åŒ¼n");

        // é¦ƒæ•Ÿé¦ƒæ•Ÿé¦ƒæ•Ÿ æ£°æ¿†î˜»æ·‡æ¿‹æ«“é”›æ°±â€˜æ·‡?LAPIC Timer æ¶”ç†·ç•¬éã„§î›¦é?
        // å¨‰ã„©å™´éŽº?éˆ?é—â•ƒæ‚Šéˆå“„å•“ LAPIC timer ç€µå‹«ç“¨é£ã„¥å½²é‘³è—‰å´±æµ£å¿¥ç´æ¶?timer å®¸å‰æ§?masked
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
            printf("LAPIC Timer disabled é‰åŒ¼n");
        } */

        // é’æ¿†îé–æ ¨æžƒæµ å‰éƒ´ç¼?
        extern void fs_init(void);
        extern void vfs_set_root(struct super_block *sb);
        extern struct super_block *ramfs_mount(void);

        printf("Initializing file system...\n");
        fs_init();  // æ©æ¬Žç´°ç’‹å†ªæ•¤ ramfs_mount éªžæƒ°î†•ç¼ƒî†½ç‰´é‚å›¦æ¬¢ç»¯è¤ç²º
        printf("File system initialized\n");

        // é’æ¿†îé–?PCI éŽ¬è¤åš?
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
            int usb_ret;
            usb_boot_status_screen("async", "kernel USB controller start");
            printf("Initializing USB controller through kernel gateway...\n");
            usb_ret = kernel_usb_init_start("kernel boot");
            if (usb_ret < 0) {
                printf("Kernel USB init request failed %d state=%d\n",
                       usb_ret, usb_get_init_state());
                panic("USB controller init request failed");
            }
            kernel_usb_init_boot_wait();
            printf("Kernel USB controller ready state=%d; port enumeration deferred\n",
                   usb_get_init_state());
            usb_boot_status_refresh();
        }
#else
        printf("[usb] hardware initialization deferred; continuing without USB\n");
        boot_color(5, 0x000000FF); /* 5 blue: safe boot passed USB stage */
#endif

        /* USB stack initialized */
        /* é¢ã„¦åŸ›éŽºÑƒåŸ—é™æ‹Œç·­é?*/
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

        // ---- ç?multiboot å¦¯â€³æ½¡ç€µç…Žå?ramfs ----
        // é–¬å¶…å·»éŽµâ‚¬éˆå¤‹Äé§æ¥çˆ£ç»? ç?cmdline æµ£æ»€è´Ÿç’ºîˆšç·ž, å¦¯â€³æ½¡éç‰ˆåµæµ£æ»€è´Ÿé‚å›¦æ¬¢éå‘­î†?
        {
            extern uint32_t multiboot2_info_addr;
            extern void *map_highmem_physical(uint32_t, uint32_t, uint32_t);
            extern struct inode *ramfs_alloc_inode(struct super_block*, int);
            extern struct dentry *d_alloc(struct inode*, const char*);
            extern void d_add(struct dentry*, struct inode*);
            extern void *kmalloc(unsigned int);
            extern void kfree(void*);

            // çº­î†»ç¹?/boot é©î†¼ç¶ç€›æ¨ºæ¹?
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
                        // cmdline ç»±Ñ†çª¡é¦ã„¦Äé§æ¥€ã”é–®ã„§ç²¨é‹å‹ªç®£éš?
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

                            // ç‘™ï½†ç€½ç€¹å±¾æš£ç’ºîˆšç·ž: "java/classes/HelloWorld.class"
                            // é˜ç»˜å¸€éˆî‚¢ç†¬é¨å‹­â”–é?é™å‚›æš?
                            char path[256];
                            int pi = 0;
                            for (char *p = cmdline; *p && *p != ' ' && pi < 254; p++)
                                path[pi++] = *p;
                            path[pi] = 0;

                            if (pi == 0) goto next_module;

                            // é–«æ„®éª‡é’æ¶˜ç¼“é©î†¼ç¶?
                            struct inode *dir = path_lookup("/");
                            if (!dir) goto next_module;

                            char *part = path;
                            char *next = part;
                            while (*next) {
                                if (*next == '/') {
                                    *next = 0;
                                    if (*(next + 1)) {  // é—ˆç‚´æ¹¯ç? é’æ¶˜ç¼“é©î†¼ç¶
                                        struct inode *sub = path_lookup(part[0]=='/' ? part+1 : part); // TODO: ç» â‚¬é?
                                        // é©å­˜å¸´æµ£è·¨æ•¤ç€¹å±¾æš£ç’ºîˆšç·žé–«æ„®éª?mkdir
                                    }
                                    *next = '/';
                                }
                                next++;
                            }

                            // ç» â‚¬é? æµ åº¢ç‰´å¯®â‚¬æ¿®? é–«æ„®éª‡é’æ¶˜ç¼“ç’ºîˆšç·žæ¶“î… æ®‘é©î†¼ç¶?
                            {
                                char tmp[256];
                                int ti = 0;
                                dir = path_lookup("/");
                                char *s = path;
                                if (*s == '/') s++; // skip leading /

                                while (*s && dir) {
                                    // æ¾¶å¶…åŸ—é’é¢ç¬…æ¶“â‚¬æ¶?/
                                    ti = 0;
                                    while (s[ti] && s[ti] != '/') { tmp[ti] = s[ti]; ti++; }
                                    tmp[ti] = 0;

                                    if (s[ti] == '/') {
                                        // æ©æ¬æ§¸é©î†¼ç¶
                                        struct inode *sub = path_lookup(tmp); // FIXME: é—‡â‚¬ç‘•ä½¸ç•¬éç£‹çŸ¾å¯?
                                        // é¢ã„§ç•é„æ’´æŸŸå¯? é?dir æ¶“å¬«ç…¡éŽµç‚¬åž¨é’æ¶˜ç¼?
                                        struct dentry *de;
                                        extern int ramfs_lookup(struct inode*, const char*, struct dentry**);
                                        if (ramfs_lookup(dir, tmp, &de) == 0 && de && de->d_inode) {
                                            dir = de->d_inode;
                                        } else {
                                            extern int ramfs_mkdir(struct inode*, const char*, int);
                                            ramfs_mkdir(dir, tmp, 0755);
                                            // é–²å¶†æŸŠlookup
                                            struct dentry *de2;
                                            if (ramfs_lookup(dir, tmp, &de2) == 0 && de2 && de2->d_inode) {
                                                dir = de2->d_inode;
                                            } else {
                                                dir = NULL;
                                            }
                                        }
                                        s += ti + 1;
                                    } else {
                                        // æ©æ¬æ§¸é‚å›¦æ¬¢é?éˆ?é’æ¶˜ç¼“é‚å›¦æ¬¢
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

        // PS/2 touchpad hangs on some systems éˆ?disabled
        // extern int ps2mouse_init(void); ps2mouse_init();
        // USB mouse/MSC tests moved to after USB init

        // é’æ¿†îé–æ «ç¶‰ç¼æ»ƒå´—ç’î†½çˆ¤
        extern void net_init(void);
        extern int loopback_init(void);
        extern int loopback_send_test(void);
        net_init();
        loopback_init();
        /* é‘·î„å§©é’æ¿†îé–æ «ç¶‰é—?éˆ?klog UDPé—‡â‚¬ç‘•ä½ºç¶‰ç¼? é©å­˜å¸´éŽµî‚£å¼¿PCI */
        int network_online = 0;
#if BOOT_AUTO_NETWORK
        {
            extern int e1000_init(const char *dev_name);
            printf("[klog] auto-initializing network...\n");
            printf("USB tests deferred until init completes\n");
        }
        // é™æˆ¦â‚¬ä½ºç¶‰ç¼æ»„ç¥´ç’‡æ›žå¯?
#else
        printf("[net] hardware network initialization deferred\n");
#endif

        printf("\n=== Network Test ===\n");

        // é¦ƒæ•Ÿ å¦«â‚¬éŒãƒ¥å”´éå‘Šçˆ¤å©§ãˆ åš?
        extern uint32_t stack_base;
        if (*(uint32_t*)&stack_base != 0xDEADBEEF) {
            printf("éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?WARNING: Stack overflow detected! Sentinel corrupted!\n");
            printf("Expected 0xDEADBEEF, got 0x%x\n", *(uint32_t*)&stack_base);
        } else {
            printf("é‰?Stack sentinel OK\n");
        }

        loopback_send_test();
        printf("=== Network Test Complete ===\n\n");

        // éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?å¨‰ã„©å™´éŽº?WiFi é‘·î„å§©å¨´å¬­ç˜¯é”›å²„ä¼©éå¶†æ¹­é’æ¿†îé–æ ¬î†–é—‚î†¼î‡±é‘·æ’®å™¸é?
        // WiFi é’æ¿†îé–æ §ç¹€æ¤¤è¤æ•±é¢ã„¦åŸ›ç»‹å¬ªç°­é–«æ°³ç¹ƒ syscall éŽµå¬ªå§©ç‘™ï¹€å½?
        /*
        // WiFi éç‰ˆåµé–å‘®ç¥´ç’?
        extern int wifi_send_test_packet(void);
        extern void wifi_show_stats(void);
        printf("\n=== WiFi Data Packet Test ===\n");
        wifi_show_stats();
        printf("\nSending test packet...\n");
        wifi_send_test_packet();
        printf("=== WiFi Test Complete ===\n\n");
        */

        // éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?å¨‰ã„¦å‰°é”›æ­…ICå®¸èŒ¬ç²¡é¦ã„¥æƒŽé¢ã„¤è…‘é‚î…žæ‚—é–°å¶‡ç–†ç€¹å±¾ç˜?
        // æ¶“å¶ˆî›¦é¦ã„¨ç¹–é–²å²„å™¸æ¾¶å¶‰åŽ¤ç¼ƒî‡†ç´é–¬åž®åŽ¤ç‘•å—™æ´Šæ¶”å¬ªå¢ é¨å‹®î†•ç¼ƒ?

        // é¦â•’GAæ¶“å©ƒæ¨‰ç»€çƒ˜ç¥´ç’‡æ›Ÿç§·éŽ­îˆ¤ç´çº­î†¿î…»ç»¯è¤ç²ºå§ï½…çˆ¶æ©æ„¯î”‘
        volatile uint16_t* vga = (volatile uint16_t*)0xC00B8000;
        vga[10] = (0x0E << 8) | 'T';
        vga[11] = (0x0E << 8) | 'E';
        vga[12] = (0x0E << 8) | 'S';
        vga[13] = (0x0E << 8) | 'T';
        printf("VGA test: wrote TEST to screen at position 10-13\n");

        // ç€¹ç‚ºç?é”›æ°­î—…å¨´å¬ªè‹Ÿé’æ¿†îé–?VBE/Framebufferé”›å œç²?Multiboot2 ç’‡è¯²å½‡é”›?
        printf("\n=== VBE/Framebuffer Initialization ===\n");

        // é–¬å¶…å·?multiboot2 éå›©î„·éŒãƒ¦å£˜ framebuffer æ·‡â„ƒä¼?
        {
            // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
            uint32_t *fb_info_ptr = (uint32_t *)mb2_phys_to_virt(mb_info_addr);
            multiboot_tag_t *fb_tag = (multiboot_tag_t *)((uint8_t *)fb_info_ptr + 8);
            int found_framebuffer = 0;

            while (fb_tag->type != MULTIBOOT_TAG_TYPE_END) {
                if (fb_tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                    multiboot_tag_framebuffer_t *fb = (multiboot_tag_framebuffer_t *)fb_tag;
                    printf("é‰?Framebuffer info available!\n");
                    printf("=== Framebuffer Tag Structure (Hex Dump) ===\n");

                    // éŽµæ’³åµƒéç¿ é‡œç¼æ’´ç€¯æµ£æ’¶æ®‘é—ä½¸åšæ©æ¶˜åŸ—dump
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

                    // éŽµå¬ªå§©ç‘™ï½†ç€½å§£å¿Žé‡œç€›æ¥î†Œé‰ãƒ©ç™ç’?
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

                    // ç¼æ’´ç€¯æµ£æ’¹Ð’é‹æ„¬â‚¬?
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

                    // éˆ¿ç‹…ç¬?æ¶“å­˜æ¤‚æ·‡î†¼î˜²é”›æ°­å¢œé”ã„¨Ð’é‹æ„¬ç“§å¨?é–¬åž®åŽ¤ç¼æ’´ç€¯æµ£æ’³î‡®æ¦»æ„°æ£¶æ£°?
                    uint64_t fb_addr = *(uint64_t*)&fb_bytes[8];
                    uint32_t fb_pitch = *(uint32_t*)&fb_bytes[16];
                    uint32_t fb_width = *(uint32_t*)&fb_bytes[20];
                    uint32_t fb_height = *(uint32_t*)&fb_bytes[24];
                    uint8_t fb_bpp = fb_bytes[28];
                    uint8_t fb_type = fb_bytes[29];

                    printf("é‰?Manual parsing successful!\n");
                    printf("  Parsed values: addr_hi=0x%x addr_lo=0x%x, %dx%d, bpp=%d, type=%d\n",
                           (uint32_t)(fb_addr >> 32), (uint32_t)fb_addr,
                           fb_width, fb_height, fb_bpp, fb_type);

                    // å¦«â‚¬å¨´å¬«æ§¸éšï¸¿è´Ÿé‚å›¨æ¹°å¦¯â€³ç´¡ (æµ£è·¨æ•¤éŽµå¬ªå§©ç‘™ï½†ç€½é¨å‹¬î„œçº­î†¼â‚?
                    if (fb_type != 1) {  // è¹‡å‘´ã€é„?RGB é¥æƒ§èˆ°å¦¯â€³ç´¡
                        printf("é‰?ERROR: Not RGB framebuffer mode!\n");
                        printf("  framebuffer_type=%d (must be 1 for RGB)\n", fb_type);
                        printf("  Type meanings:\n");
                        printf("    0 = Indexed color\n");
                        printf("    1 = RGB (required for graphics) é‰åŒ¼n");
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

                    // æ£°æ¿†î˜»å¦«â‚¬éŒãƒ¯ç´°bpp é?pitch è¹‡å‘´ã€éˆå¤‹æ™¥
                    if (fb_bpp == 0 || fb_pitch == 0) {
                        printf("é‰?ERROR: Invalid framebuffer parameters!\n");
                        printf("  bpp=%d (must be > 0)\n", fb_bpp);
                        printf("  pitch=%d (must be > 0)\n", fb_pitch);
                        found_framebuffer = 0;
                        break;
                    }

                    printf("é‰?RGB framebuffer mode detected!\n");

                    found_framebuffer = 1;

                    // é’æ¿†îé–?VBE æ¤¹åžå§©é”›å œå¨‡é¢ã„¦å¢œé”ã„¨Ð’é‹æ„®æ®‘å§ï½‡â€˜éŠç¡·ç´š
                    extern void vbe_init_from_multiboot(uint64_t fb_addr, uint32_t width,
                                                        uint32_t height, uint32_t pitch, uint8_t bpp);
                    vbe_init_from_multiboot(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
                    printf("é‰?VBE driver initialized from Multiboot2 info\n");

                    // é¦ƒæ•Ÿ éå‘®ç‰³éŽµÑ†î”‘æ©æ¶˜å®³éŽ¸å›©ã?
                    // é—ƒèˆµî†?é”›æ­BEé’æ¿†îé–æ ¨åžšé?- é„å‰§ãšç¼èƒ¯å£Šé?
                    if (vbe_is_available()) {
                        volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
                        uint16_t w, h;
                        vbe_get_resolution(&w, &h);
                        uint16_t pitch = vbe_get_pitch();
                        uint32_t pitch_pixels = pitch / 4;

                        // ç¼æ¨ºåŸ—çå¿•æ®‘ç¼èƒ¯å£Šé‚ç‘°æ½¡é¦ã„¥å½¸æ¶“å©…î—é”›å²ƒã€ƒç»€ç¯¤BEé’æ¿†îé–æ ¨åžšé?
                        for (int y = 0; y < 30; y++) {
                            for (int x = w - 30; x < w; x++) {
                                fb[y * pitch_pixels + x] = 0xFF00FF00;  // ç¼èƒ¯å£?
                            }
                        }
                        printf("é‰?Progress indicator: VBE initialized (green square)\n");
                    }
                    break;
                }
                fb_tag = (multiboot_tag_t *)((uint8_t *)fb_tag + ((fb_tag->size + 7) & ~7));
            }

            if (!found_framebuffer) {
                printf("é‰?No valid framebuffer info available from GRUB\n");
                printf("  Note: GRUB did not provide RGB framebuffer (type=1)\n");
                printf("\n");
                printf("éˆ?WARNING: GUI functions will NOT work!\n");
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

        // éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬éˆ¿ç‹…ç¬?æ¶“å­˜æ¤‚æ·‡î†¼î˜²é”›æ°­æ•žé–²å©ƒå¸€ PID=1 éå‘®ç‰³æµ è¯²å§Ÿé”›å²„ä¼©éå¶…åžéŽ¹ãˆ æ´–ç€¹å†©æ¤‚å®•â•‚ç°
        // é˜ç†·æ´œé”›æ­…ID=1 éå‘®ç‰³æµ è¯²å§Ÿé¨å‹¬çˆ¤ç»Œæ´ªæ£¿æ¶“å¶ˆå†»é”›å±½åžéŽ¹ãˆ æ´–ç€¹å†©æ¤‚æµ¼æ°³Ð•é™?triple fault
        // ç’‡ï¹î†é”›æ­…ID1_CRASH_FIX.md
        // task_t *th_k=init_task(0);
        // printf("start kernel task\n");
        // start_task_kernel(th_k,kernel_task_main);

        // é©å­˜å¸´é’æ¶˜ç¼“é¢ã„¦åŸ›æµ è¯²å§Ÿæµ£æ»€è´Ÿç»—îƒ¿ç«´æ¶“îƒæ¢é”?
        task_t *th_u=init_task(1);

        // å¨‰ã„©å™´éŽºå¡³mallocå¨´å¬­ç˜?é–¬åž®åŽ¤è¤°åžæ·é¢ã„¦åŸ›æ©æ¶šâ–¼é”çŠºæµ?
        /*
        // æˆæ’³åš­éå‘­ç“¨å¦«â‚¬å¨´å¬¬ç²¨é‹?
        print_memory_detection_result();

        // æˆæ’³åš?PMM ç¼ç†»î…¸æ·‡â„ƒä¼…
        pmm_print_stats();

        // å¨´å¬­ç˜?kmalloc é?kfree é”ç†»å…?
        printf("\n=== Testing kmalloc/kfree functionality ===\n");

        // å¨´å¬­ç˜?1: çå¿“å”´ç€›æ¨ºåžŽé–°å¶ç´™æµ£è·¨æ•?early poolé”?
        printf("\nTest 1: Small allocation (early pool)\n");
        void *ptr1 = kmalloc(128);
        printf("  Allocated 128 bytes at: 0x%x\n", (uint32_t)ptr1);

        // å¨´å¬­ç˜?2: æ¶“î… ç“‘éå‘­ç“¨é’å—›åŽ¤é”›å œå¨‡é?early poolé”?
        printf("\nTest 2: Medium allocation (early pool)\n");
        void *ptr2 = kmalloc(1024);
        printf("  Allocated 1024 bytes at: 0x%x\n", (uint32_t)ptr2);

        // å¨´å¬­ç˜?3: æ¾¶Ñƒå”´ç€›æ¨ºåžŽé–°å¶ç´™æµ£è·¨æ•?PMMé”?
        printf("\nTest 3: Large allocation (PMM)\n");
        void *ptr3 = kmalloc(8192);  // 8KB = 2 pages
        printf("  Allocated 8192 bytes at: 0x%x\n", (uint32_t)ptr3);

        // å¨´å¬­ç˜?4: ç“’å‘­ã‡éå‘­ç“¨é’å—›åŽ¤é”›å œå¨‡é?PMMé”?
        printf("\nTest 4: Extra large allocation (PMM)\n");
        void *ptr4 = kmalloc(16384);  // 16KB = 4 pages
        printf("  Allocated 16384 bytes at: 0x%x\n", (uint32_t)ptr4);

        // å¨´å¬­ç˜?5: kzalloc é’å—›åŽ¤éªžèˆµç«»é—?
        printf("\nTest 5: kzalloc (zero-initialized)\n");
        void *ptr5 = kzalloc(512);
        printf("  Allocated 512 zero-initialized bytes at: 0x%x\n", (uint32_t)ptr5);

        // é„å‰§ãšé’å—›åŽ¤ç¼ç†»î…?
        printf("\n");
        kmalloc_print_stats();

        // å¨´å¬­ç˜?6: kfree å¨´å¬­ç˜?
        printf("\nTest 6: Testing kfree\n");
        printf("  Freeing ptr2 (1024 bytes)\n");
        kfree(ptr2);

        printf("  Freeing ptr4 (16384 bytes)\n");
        kfree(ptr4);

        // éå¶†î‚¼é„å‰§ãšç¼ç†»î…?
        printf("\nAfter freeing:\n");
        kmalloc_print_stats();

        printf("=== kmalloc/kfree tests completed ===\n\n");
        */

        // é¦ƒæ•Ÿ éå‘®ç‰³éŽµÑ†î”‘æ©æ¶˜å®³éŽ¸å›©ã?
        // é—ƒèˆµî†?é”›æ°¬å™¯æ¾¶å›§æƒŽé”ã„§æ•¤éŽ´èœ‚æ¢é?- é„å‰§ãšé’ƒæ¿Šå£Šé?
        if (vbe_is_available()) {
            volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
            uint16_t w, h;
            vbe_get_resolution(&w, &h);
            uint16_t pitch = vbe_get_pitch();
            uint32_t pitch_pixels = pitch / 4;

            // ç¼æ¨ºåŸ—çå¿•æ®‘é’ƒæ¿Šå£Šé‚ç‘°æ½¡é¦ã„¥ä¹æ¶“å¬­î—é”›å²ƒã€ƒç»€å“„å™¯æ¾¶å›§æƒŽé”ã„§æ•¤éŽ´èœ‚æ¢é”?
            for (int y = h - 30; y < h; y++) {
                for (int x = 0; x < 30; x++) {
                    fb[y * pitch_pixels + x] = 0xFF0000FF;  // é’ƒæ¿Šå£?
                }
            }
            printf("é‰?Progress indicator: Ready to start user task (blue square)\n");
        }

        // éšîˆšå§©é¢ã„¦åŸ›æ©æ¶šâ–?
        printf("start user task \n");
        // éšîˆšå§©é¢ã„¦åŸ›æ©æ¶šâ–?
        printf("start user task \n");

        // ç’‹å†­ç˜¯é”›æ°³ç·­é‘ç°ƒultiboot2å¦¯â€³æ½¡æ·‡â„ƒä¼…é”›å å‡¡ç»‚ä½ºæ•¤é”›å²„ä¼©éå¶‡éƒ´ç¼ç†·ç©¿å©§å†¿ç´?
        // dump_multiboot2_modules(mb_info_addr);

        // é–²å¶ˆî›¦é”›æ°¬åŽ›éŽµå¬ªå§©ç’‹å†ªæ•¤ user_task_main æ©æ¶œî”‘é’æ¿†îé–æ µç´™æ¤¤ä½ƒã€ƒéŠ†ä½¹Äé§æ¥€å§žæžç•Œç“‘é”?
        // é’æ¿†îé–æ §ç•¬éŽ´æ„¬æ‚—é”›å¯€ser_task_main æµ¼æ°³ç¹‘é¥?
        user_task_main(th_u);

        // é’æ¿†îé–æ §ç•¬éŽ´æ„¬æ‚—é”›å±½çš¢é¢ã„¦åŸ›æµ è¯²å§Ÿå¨£è¯²å§žé’æ‹ŒçšŸæ´ï¹€æ«’é—ƒç†·åžª
        // start_task æµ¼æ°¬çš?th_u å¨£è¯²å§žé’?sched_root é–¾æã€ƒé”›å²ƒç¹–éç–¯çšŸæ´ï¹€æ«’éŽµå¶ˆå…˜éŽµæƒ§åŸŒç€?
        start_task(th_u, user_task_main);

        // ç’å‰§ç–†æµ è¯²å§Ÿé˜èˆµâ‚¬ä½·è´?PS_CREATEDé”›å²ƒî†€ç’‹å†¨å®³é£ã„§ç…¡é–¬æ’»æ¸¶ç‘•ä½½çƒ¦æžî„€åŸŒé¢ã„¦åŸ›éŽ?
        th_u->state = PS_CREATED;
        printf("[kernel_main] User task initialized, state=PS_CREATED\n");

        // é¦ƒæ•Ÿ éå‘®ç‰³éŽµÑ†î”‘æ©æ¶˜å®³éŽ¸å›©ã?
        // é—ƒèˆµî†?é”›æ°±æ•¤éŽ´èœ‚æ¢é”â€³æƒŽé”ã„¦åžšé”?- é„å‰§ãšæ¦›å‹®å£Šé?
        if (vbe_is_available()) {
            volatile uint32_t *fb = (volatile uint32_t *)0xF0000000;
            uint16_t w, h;
            vbe_get_resolution(&w, &h);
            uint16_t pitch = vbe_get_pitch();
            uint32_t pitch_pixels = pitch / 4;

            // ç¼æ¨ºåŸ—çå¿•æ®‘æ¦›å‹®å£Šé‚ç‘°æ½¡é¦ã„¥å½¸æ¶“å¬­î—é”›å²ƒã€ƒç»€è™¹æ•¤éŽ´èœ‚æ¢é”â€³æƒŽé”ã„¦åžšé”?
            for (int y = h - 30; y < h; y++) {
                for (int x = w - 30; x < w; x++) {
                    fb[y * pitch_pixels + x] = 0xFFFFFF00;  // æ¦›å‹®å£?
                }
            }
            printf("é‰?Progress indicator: User task ready (yellow square)\n");
        }

        /*
        // é’æ¶˜ç¼“ç»—îƒ¿ç°©æ¶“î†æ•¤éŽ´ç–¯ç¹˜ç»‹å¬¶ç´™å¨´å¬­ç˜¯ç’‹å†¨å®³é”›? å®¸æ’ç´”é¢?
        // éœæ¿æ¹ªæµ£è·¨æ•¤ fork() ç»¯è¤ç²ºç’‹å†ªæ•¤é‡å¤¸å”?
        printf("=== Creating second user task for scheduler testing ===\n");
        printf("th_u = 0x%x, th_u->pde = 0x%x, th_u->cr3 = 0x%x\n",
               th_u, th_u->pde, th_u->cr3);

        task_t *th_u2 = init_task(true);  // é’æ¶˜ç¼“ç»—îƒ¿ç°©æ¶“îƒæ¢é”?
        if (th_u2) {
            printf("th_u2 created: 0x%x\n", th_u2);

            // ç€¹ç‚µå¹‡ç» â‚¬é—æ› æ®‘ fork éˆå“„åŸ—é”›æ°«è´Ÿç»—îƒ¿ç°©æ¶“î‡ç¹˜ç»‹å¬ªåž±å¯¤è™¹å«­ç»”å¬¬æ®‘æ¤¤ç”µæ´°è¤?
            // é™å‚â‚¬å†¿ç´?
            // - Brown University CS: "fork() clones user-space processes with full address space copies"
            // - Reddit r/osdev: "fork() and copy-on-write in Linux"
            //
            // éæŠ½æ•­éç™¸ç´°
            // 1. å§£å¿Žé‡œæ©æ¶šâ–¼é—‡â‚¬ç‘•ä½ºå«­ç»”å¬¬æ®‘æ¤¤ç”µæ´°è¤°æ›ªç´™CR3é”?
            // 2. æ¤¤ç”µæ´°è¤°æ›¢ã€éŽ¸å›§æ‚œé©ç¨¿æ‚“é¨å‹­å¢¿éžå—›ã€‰é”›å å¡æµœî‚¢å”´ç€›æ©ˆç´?
            // 3. éå‘®ç‰³é„çŠ²çš é”?xC0000000+é”›å¤æ¸¶ç‘•ä½¸î˜²é’è·ºåŸŒå§£å¿Žé‡œæ¤¤ç”µæ´°è¤°?

            extern page_t alloc_page_table_();
            extern void copy_kernel_mappings_to_pd(uint32_t *pd_user);

            extern uint32_t pmm_alloc_page(void);

            // 1. é’å—›åŽ¤é‚æ‰®æ®‘æ¤¤ç”µæ´°è¤°?
            page_t pde2 = alloc_page_table_();
            th_u2->pde = (uint32_t*)pde2.phys;
            th_u2->cr3 = (uint32_t*)pde2.phys;  // CR3 ç€›æ¥î†Œç€›æ¨ºåé—â•ƒæ‚Šé¦æ¿æ½?
            uint32_t *pd_user2 = pde2.virt;

            printf("[fork] Allocated new page dir: phys=0x%x, virt=0x%x\n",
                   pde2.phys, pd_user2);

            // 2. æ¾¶å¶…åŸ—éå‘®ç‰³æ¥‚æ¨¼î¬é„çŠ²çš é”?68-1023é”›å±½åµ?0xC0000000-0xFFFFFFFFé”?
            // æ©æ¬ç‰±éå‘®ç‰³æµ ï½‡çˆœé¦ã„¦å¢éˆå¤Žç¹˜ç»‹å¬©è…‘é–®è—‰å½²ç’å—æ£¶
            copy_kernel_mappings_to_pd(pd_user2);
            printf("[fork] Copied kernel mappings to new page dir\n");

            // 3. æ¾¶å¶…åŸ—é¢ã„¦åŸ›ç»Œæ´ªæ£¿é„çŠ²çš é”?-767é”›å±½åµ?0x00000000-0xBFFFFFFFé”?
            // æ©æ¬æ§¸ç» â‚¬é—æ› æ®‘æ¤¤ä½ƒã€ƒæ¾¶å¶…åŸ—é”›å±¼ç¬‰é„?Copy-on-Write
            // æ¶“ã‚„é‡œæ©æ¶šâ–¼æµ¼æ°¬å¡æµœî‚¤æµ‰éšå²€æ®‘é—â•ƒæ‚Šæ¤¤ç¢‰ç´æµ£å—˜æ¹é™î„‚ç›é¨å‹¯ã€‰ç›ã„§ç²¨é?

            // é‘¾å³°å½‡ç»—îƒ¿ç«´æ¶“îƒæ¢é”ï¼„æ®‘æ¤¤ç”µæ´°è¤°æ›¡æ«„éŽ·ç†·æ¹´é§â‚?
            extern uint32_t pd[];
            uint32_t *pd_user1 = (uint32_t*)phys_to_virt((uint32_t)th_u->pde);

            int copied_entries = 0;
            for (int i = 0; i < 768; i++) {  // é¢ã„¦åŸ›ç»Œæ´ªæ£¿é”?-767
                if (pd_user1[i] & PAGE_PRESENT) {
                    pd_user2[i] = pd_user1[i];  // æ¾¶å¶…åŸ—æ¤¤ä½ƒã€ƒæ¤¤è§„å¯šé–?
                    copied_entries++;
                }
            }
            printf("[fork] Copied %d user space page table entries\n", copied_entries);

            // 4. é’å—›åŽ¤é‚æ‰®æ®‘éå‘®ç‰³é?
            uint32_t kstack2 = pmm_alloc_page();
            th_u2->kstack = (uint32_t*)kstack2;
            th_u2->esp0 = (uint32_t)(kstack2 + PAGE_SIZE);

            printf("[fork] Task2: pde=0x%x, cr3=0x%x, kstack=0x%x\n",
                   th_u2->pde, th_u2->cr3, th_u2->kstack);

            // 5. æ¾¶å¶…åŸ?trapframe
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

        // éšîˆšå§©ç’‹å†¨å®³é?
        // printf("Starting scheduler with multiple tasks...\n");
        efficient_scheduler_loop();
        // printf("Kernel main completed successfully!\n");
	return (42);
}

/*
 * usb_periodic_poll_callback éˆ?called from the scheduler idle loop
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

    /* RTL8188EUS keeps one Bulk IN request armed. Poll it from the same
     * single-consumer USB idle path so endpoint events cannot be stolen by
     * competing driver threads. */
    {
        extern void rtl8188eu_wifi_poll(void);
        rtl8188eu_wifi_poll();
    }

    int max_ctrl = num_uhci_controllers;
    if (max_ctrl < 1) max_ctrl = 1;

    for (int i = 0; i < max_ctrl; i++)
        usb_hcd_poll_hotplug(i);
}

