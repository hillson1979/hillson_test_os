/*
 * vbe.c - VESA BIOS Extensions 驱动
 * 在内核中实现 VBE 调用,通过系统调用接口提供给用户进程
 */

#include "types.h"
#include "vbe.h"
#include "x86/io.h"

// VBE 控制器信息结构
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

// VBE 模式信息结构
typedef struct {
    uint16_t mode_attributes; // 模式属性
    uint8_t  win_a_attributes;
    uint8_t  win_b_attributes;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment;
    uint16_t win_b_segment;
    uint32_t win_function_ptr;
    uint16_t bytes_per_scanline;
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t  x_char_size;
    uint8_t  y_char_size;
    uint8_t  number_of_planes;
    uint8_t  bits_per_pixel;
    uint8_t  number_of_banks;
    uint8_t  memory_model;
    uint8_t  bank_size;
    uint8_t  number_of_image_pages;
    uint8_t  reserved0;
    uint8_t  red_mask_size;
    uint8_t  red_field_position;
    uint8_t  green_mask_size;
    uint8_t  green_field_position;
    uint8_t  blue_mask_size;
    uint8_t  blue_field_position;
    uint8_t  rsvd_mask_size;
    uint8_t  rsvd_field_position;
    uint8_t  direct_color_mode_info;
    uint32_t phys_base_ptr;       // 物理帧缓冲区指针 (关键!)
    uint32_t offscreen_mem_offset;
    uint16_t offscreen_mem_size;
    uint8_t  reserved1[206];
} vbe_mode_info_t;
#pragma pack()

// VBE 状态
static int vbe_available = 0;
static uint32_t vbe_framebuffer = 0;  // 线性帧缓冲区物理地址
static uint16_t vbe_width = 0;
static uint16_t vbe_height = 0;
static uint8_t vbe_bpp = 0;
static uint16_t vbe_pitch = 0;

/**
 * VBE BIOS 调用 (在内核中执行)
 * VBE 使用 ES:DI 指向缓冲区
 * 注意:BIOS 运行在实模式,需要物理地址,但通过段:偏移方式
 */
static uint16_t vbe_bios_call(uint16_t ax, uint16_t bx, uint16_t cx, void *buffer) {
    uint16_t ret_ax;
    uint32_t buf_addr = (uint32_t)buffer;

    // 内核虚拟地址直接转换为物理地址的低32位
    // 因为我们使用恒等映射,虚拟地址 - 0xC0000000 = 物理地址
    uint32_t phys_addr = buf_addr - 0xC0000000;
    uint16_t es = (phys_addr >> 4) & 0xF000;  // 段选择子
    uint16_t di = phys_addr & 0xFFFF;           // 偏移

    asm volatile (
        "movw %w4, %%es\n\t"
        "movw %w5, %%di\n\t"
        "int $0x10\n\t"
        : "=a" (ret_ax)
        : "a" (ax), "b" (bx), "c" (cx), "r" (es), "r" (di)
        : "di", "memory"
    );
    return ret_ax;
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
    uint16_t ax = vbe_bios_call(0x4F00, 0, 0, &info);

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
    uint16_t ax = vbe_bios_call(0x4F01, 0, mode, &mode_info_temp);

    if (ax != 0x004F) {
        return -1;
    }

    // 拷贝到用户提供的缓冲区
    if (info) {
        vbe_mode_info_user_t *user_info = (vbe_mode_info_user_t *)info;
        user_info->x_resolution = mode_info_temp.x_resolution;
        user_info->y_resolution = mode_info_temp.y_resolution;
        user_info->bits_per_pixel = mode_info_temp.bits_per_pixel;
        user_info->phys_base_ptr = mode_info_temp.phys_base_ptr;
        user_info->bytes_per_scanline = mode_info_temp.bytes_per_scanline;
        user_info->mode_attributes = mode_info_temp.mode_attributes;
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
    uint16_t ax = vbe_bios_call(0x4F02, mode_with_lfb, 0, 0);

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
