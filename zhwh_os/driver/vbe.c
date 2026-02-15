/*
 * vbe.c - VESA BIOS Extensions 驱动
 * 在内核中实现 VBE 调用,通过系统调用接口提供给用户进程
 */

#include "types.h"
#include "vbe.h"
#include "x86/io.h"

// VBE 控制器信息结构 (内部使用)
#pragma pack(1)
typedef struct {
    char sig[4];              // "VESA" 签名
    uint16_t version;         // VBE 版本
    uint32_t oem_string;      // OEM 字符串指针
    uint32_t capabilities;    // 能力标志
    uint32_t mode_list;       // 模式列表指针
    uint16_t total_memory;    // 总内存 (以 64KB 为单位)
    uint16_t oem_rev;         // OEM 版本
    uint32_t vendor_name;     // 供应商名称
    uint32_t product_name;    // 产品名称
    uint32_t rev_name;        // 版本名称
    char reserved[222];       // 保留
    char oem_data[256];       // OEM 数据
} vbe_controller_info_t;
#pragma pack()

// VBE 状态
static int vbe_available = 0;
static uint32_t vbe_framebuffer = 0;  // 线性帧缓冲区物理地址
static uint16_t vbe_width = 0;
static uint16_t vbe_height = 0;
static uint8_t vbe_bpp = 0;
static uint16_t vbe_pitch = 0;

/**
 * VBE BIOS 调用包装函数 (已禁用)
 *
 * 注意: 实模式 thunk 暂时禁用,因为实现过于复杂。
 * 这个函数现在只返回失败,让调用者知道 VBE BIOS 调用不可用。
 */
static uint16_t vbe_bios_call_wrapper(uint16_t ax, uint16_t bx, uint16_t cx, void *buffer) {
    // 实模式 thunk 已禁用,直接返回失败
    return 0xFFFF;  // 返回错误码
}

/**
 * 检测 VBE 支持
 */
int vbe_detect(void) {
    // 在栈上分配缓冲区
    static vbe_controller_info_t info;

    // 设置签名
    info.sig[0] = 'V';
    info.sig[1] = 'B';
    info.sig[2] = 'E';
    info.sig[3] = '2';

    // 调用 VBE 功能 0x4F00,传递缓冲区指针
    uint16_t ax = vbe_bios_call_wrapper(0x4F00, 0, 0, &info);

    // 检查返回值
    if ((ax & 0xFF) != 0x4F) {
        return 0;  // VBE 不支持
    }

    if ((ax >> 8) != 0x00) {
        return 0;  // 调用失败
    }

    // 检查签名
    if (info.sig[0] != 'V' || info.sig[1] != 'E' ||
        info.sig[2] != 'S' || info.sig[3] != 'A') {
        return 0;  // 无效签名
    }

    vbe_available = 1;
    return 1;
}

/**
 * 获取 VBE 模式信息
 */
int vbe_get_mode_info(uint16_t mode, void *info) {
    if (!vbe_available) {
        return -1;
    }

    // 在栈上分配临时缓冲区
    vbe_mode_info_t mode_info_temp;

    // 调用 VBE 功能 0x4F01
    uint16_t ax = vbe_bios_call_wrapper(0x4F01, 0, mode, &mode_info_temp);

    if (ax != 0x004F) {
        return -1;
    }

    // 拷贝到用户提供的缓冲区
    if (info) {
        vbe_mode_info_user_t *user_info = (vbe_mode_info_user_t *)info;
        user_info->x_resolution = mode_info_temp.width;
        user_info->y_resolution = mode_info_temp.height;
        user_info->bits_per_pixel = mode_info_temp.bpp;
        user_info->phys_base_ptr = mode_info_temp.framebuffer;
        user_info->bytes_per_scanline = mode_info_temp.pitch;
        user_info->mode_attributes = mode_info_temp.attributes;
    }

    return 0;
}

/**
 * 设置 VBE 模式
 */
int vbe_set_mode(uint16_t mode) {
    if (!vbe_available) {
        return -1;
    }

    // 启用线性帧缓冲区 (bit 14)
    uint16_t mode_with_lfb = mode | 0x4000;

    // 调用 VBE 功能 0x4F02
    uint16_t ax = vbe_bios_call_wrapper(0x4F02, mode_with_lfb, 0, 0);

    if (ax != 0x004F) {
        return -1;
    }

    // 获取模式信息并保存
    vbe_mode_info_user_t mode_info;
    if (vbe_get_mode_info(mode, &mode_info) == 0) {
        vbe_framebuffer = mode_info.phys_base_ptr;
        vbe_width = mode_info.x_resolution;
        vbe_height = mode_info.y_resolution;
        vbe_bpp = mode_info.bits_per_pixel;
        vbe_pitch = mode_info.bytes_per_scanline;
    }

    return 0;
}

/**
 * 获取 VBE 帧缓冲区地址
 */
uint32_t vbe_get_framebuffer(void) {
    return vbe_framebuffer;
}

/**
 * 获取 VBE 分辨率
 */
void vbe_get_resolution(uint16_t *width, uint16_t *height) {
    if (width) *width = vbe_width;
    if (height) *height = vbe_height;
}

/**
 * 获取 VBE 色深
 */
uint8_t vbe_get_bpp(void) {
    return vbe_bpp;
}

/**
 * 获取 VBE pitch
 */
uint16_t vbe_get_pitch(void) {
    return vbe_pitch;
}

/**
 * 检查 VBE 是否可用
 */
int vbe_is_available(void) {
    return vbe_available;
}

/**
 * @brief 初始化 VBE 并设置图形模式
 * @return 0 成功，-1 失败
 * @note 此函数使用 BIOS 调用，只能在实模式或通过 VBE BIOS 使用
 */
int vbe_init(void) {
    // 检测 VBE
    if (!vbe_detect()) {
        return -1;
    }

    // 设置 1024x768x16 模式
    if (vbe_set_mode(0x117) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief 根据分辨率和色深自动选择并设置 VBE 图形模式
 * @param width 期望的宽度
 * @param height 期望的高度
 * @param bpp 期望的色深
 * @return 0 成功，-1 失败
 *
 * 支持的模式:
 * - 1024x768x32 (0x118)
 * - 1024x768x16 (0x117)
 * - 800x600x32 (0x115)
 * - 800x600x16 (0x114)
 * - 640x480x32 (0x112)
 * - 640x480x16 (0x111)
 */
int vbe_set_graphics_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    // 首先检测 VBE 支持
    if (!vbe_detect()) {
        printf("VBE: 检测失败\n");
        return -1;
    }

    // 根据参数选择合适的 VBE 模式
    uint16_t mode = 0;

    if (width == 1024 && height == 768) {
        if (bpp == 32) {
            mode = 0x118;
        } else if (bpp == 16) {
            mode = 0x117;
        } else if (bpp == 24) {
            mode = 0x118;  // 24位通常用32位模式
        }
    } else if (width == 800 && height == 600) {
        if (bpp == 32) {
            mode = 0x115;
        } else if (bpp == 16) {
            mode = 0x114;
        }
    } else if (width == 640 && height == 480) {
        if (bpp == 32) {
            mode = 0x112;
        } else if (bpp == 16) {
            mode = 0x111;
        }
    }

    if (mode == 0) {
        printf("VBE: 不支持的模式 %dx%dx%d\n", width, height, bpp);
        return -1;
    }

    printf("VBE: 尝试设置模式 0x%x (%dx%dx%d)...\n", mode, width, height, bpp);

    // 设置模式
    if (vbe_set_mode(mode) != 0) {
        printf("VBE: 设置模式失败\n");
        return -1;
    }

    printf("VBE: 模式设置成功!\n");
    printf("  framebuffer: 0x%x\n", vbe_framebuffer);
    printf("  分辨率: %dx%d\n", vbe_width, vbe_height);
    printf("  色深: %d bpp\n", vbe_bpp);
    printf("  pitch: %d\n", vbe_pitch);

    return 0;
}

/**
 * @brief 从 Multiboot2 帧缓冲区信息初始化 VBE 驱动
 * @param fb_addr 帧缓冲区物理地址
 * @param width 宽度
 * @param height 高度
 * @param pitch 每行字节数
 * @param bpp 色深
 * @note 这是推荐的方式，因为 GRUB 在启动时已经设置了图形模式
 */
void vbe_init_from_multiboot(uint64_t fb_addr, uint32_t width, uint32_t height,
                              uint32_t pitch, uint8_t bpp) {
    vbe_framebuffer = (uint32_t)fb_addr;
    vbe_width = (uint16_t)width;
    vbe_height = (uint16_t)height;
    vbe_pitch = (uint16_t)pitch;
    vbe_bpp = bpp;
    vbe_available = 1;

    printf("[VBE] Initializing from Multiboot2 info:\n");
    printf("[VBE]   Physical framebuffer: 0x%x\n", vbe_framebuffer);
    printf("[VBE]   Resolution: %dx%d\n", vbe_width, vbe_height);
    printf("[VBE]   BPP: %d, Pitch: %d\n", vbe_bpp, vbe_pitch);

    // ⚠️ 重要: 将framebuffer物理地址映射到虚拟地址空间
    extern uint32_t kernel_page_directory_phys;  // 内核页目录物理地址
    extern void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags);

    uint32_t fb_phys = vbe_framebuffer;
    uint32_t fb_virt = 0xF0000000;  // 使用固定的虚拟地址 0xF0000000

    // 计算需要映射的页数
    uint32_t fb_size = vbe_pitch * vbe_height;
    uint32_t num_pages = (fb_size + 4095) / 4096;

    printf("[VBE] Mapping framebuffer to virtual address 0x%x...\n", fb_virt);
    printf("[VBE]   Size: %d bytes (%d pages)\n", fb_size, num_pages);
    printf("[VBE]   Kernel PD: 0x%x\n", kernel_page_directory_phys);

    // 逐页映射 (使用内核页目录)
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t phys = fb_phys + i * 4096;
        uint32_t virt = fb_virt + i * 4096;

        // 使用内核页目录物理地址进行映射
        map_page(kernel_page_directory_phys, virt, phys, 0x3);  // WRITE | PRESENT
    }

    printf("[VBE] ✓ Framebuffer mapped successfully!\n");

    // 测试: 画4个大的彩色方块 (每个占1/4屏幕)
    volatile uint32_t *fb = (volatile uint32_t *)fb_virt;
    uint32_t half_width = vbe_width / 2;
    uint32_t half_height = vbe_height / 2;
    uint32_t pitch_pixels = vbe_pitch / 4;

    printf("[VBE] Drawing test pattern (4 colored quadrants)...\n");

    // 左上: 红色
    for (uint32_t y = 0; y < half_height; y++) {
        for (uint32_t x = 0; x < half_width; x++) {
            fb[y * pitch_pixels + x] = 0xFFFF0000;  // 红色
        }
    }

    // 右上: 绿色
    for (uint32_t y = 0; y < half_height; y++) {
        for (uint32_t x = half_width; x < vbe_width; x++) {
            fb[y * pitch_pixels + x] = 0xFF00FF00;  // 绿色
        }
    }

    // 左下: 蓝色
    for (uint32_t y = half_height; y < vbe_height; y++) {
        for (uint32_t x = 0; x < half_width; x++) {
            fb[y * pitch_pixels + x] = 0xFF0000FF;  // 蓝色
        }
    }

    // 右下: 白色
    for (uint32_t y = half_height; y < vbe_height; y++) {
        for (uint32_t x = half_width; x < vbe_width; x++) {
            fb[y * pitch_pixels + x] = 0xFFFFFFFF;  // 白色
        }
    }

    printf("[VBE] ✓ Test pattern drawn!\n");
    printf("[VBE]   Top-Left: RED\n");
    printf("[VBE]   Top-Right: GREEN\n");
    printf("[VBE]   Bottom-Left: BLUE\n");
    printf("[VBE]   Bottom-Right: WHITE\n");
}
