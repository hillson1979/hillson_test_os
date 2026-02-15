/*
 * vbe.h - VESA BIOS Extensions 驱动接口
 */

#ifndef _VBE_H
#define _VBE_H

#include "types.h"

// VBE 模式定义
#define VBE_MODE_101 0x101  // 640x480x8
#define VBE_MODE_103 0x103  // 800x600x8
#define VBE_MODE_105 0x105  // 1024x768x8
#define VBE_MODE_112 0x112  // 640x480x16
#define VBE_MODE_115 0x115  // 800x600x16
#define VBE_MODE_117 0x117  // 1024x768x16
#define VBE_MODE_118 0x118  // 1024x768x24 (或32位，取决于硬件)
#define VBE_MODE_111 0x111  // 640x480x15

// VBE BIOS 调用定义
#define VBE_GET_CONTROLLER_INFO 0x4F00
#define VBE_GET_MODE_INFO       0x4F01
#define VBE_SET_MODE            0x4F02
#define VBE_GET_CURRENT_MODE    0x4F03

// VBE 信息结构 (512字节对齐)
#pragma pack(1)
typedef struct {
    char     signature[4];       // "VESA"
    uint16_t version;            // VBE 版本
    uint32_t oem_string;         // OEM 字符串指针
    uint32_t capabilities;       // 能力标志
    uint32_t video_modes;        // 视频模式指针
    uint16_t total_memory;       // 总内存 (以64KB为单位)
    uint16_t oem_software_rev;   // OEM 软件版本
    uint32_t oem_vendor_name;    // OEM 供应商名称
    uint32_t oem_product_name;   // OEM 产品名称
    uint32_t oem_product_rev;    // OEM 产品修订
    uint8_t  reserved[222];      // 保留
    uint8_t  oem_data[256];      // OEM 数据
} vbe_info_t;

typedef struct {
    uint16_t attributes;         // 模式属性
    uint8_t  window_a;           // 窗口A属性
    uint8_t  window_b;           // 窗口B属性
    uint16_t granularity;        // 粒度
    uint16_t window_size;        // 窗口大小
    uint16_t segment_a;          // 窗口A段地址
    uint16_t segment_b;          // 窗口B段地址
    uint32_t win_func_ptr;       // 窗口函数指针
    uint16_t pitch;              // 每行字节数

    uint16_t width;              // 宽度
    uint16_t height;             // 高度
    uint8_t  w_char;             // 字符宽度
    uint8_t  y_char;             // 字符高度
    uint8_t  planes;             // 平面数
    uint8_t  bpp;                // 每像素位数
    uint8_t  banks;              // 内存bank数
    uint8_t  memory_model;       // 内存模型
    uint8_t  bank_size;          // Bank大小
    uint8_t  image_pages;        // 图像页数
    uint8_t  reserved0;          // 保留

    uint8_t  red_mask;           // 红色掩码
    uint8_t  red_position;       // 红色位置
    uint8_t  green_mask;         // 绿色掩码
    uint8_t  green_position;     // 绿色位置
    uint8_t  blue_mask;          // 蓝色掩码
    uint8_t  blue_position;      // 蓝色位置
    uint8_t  reserved_mask;      // 保留掩码
    uint8_t  reserved_position;  // 保留位置
    uint8_t  directcolor_attributes; // 直接颜色属性

    uint32_t framebuffer;        // 帧缓冲区物理地址
    uint32_t offscreen_mem_off;  // 离屏内存偏移
    uint16_t offscreen_mem_size; // 离屏内存大小
    uint8_t  reserved[206];      // 保留
} vbe_mode_info_t;
#pragma pack()

// VBE 函数 (内核中使用)
int vbe_detect(void);
int vbe_set_mode(uint16_t mode);
int vbe_init(void);
int vbe_set_graphics_mode(uint16_t width, uint16_t height, uint8_t bpp);
void vbe_init_from_multiboot(uint64_t fb_addr, uint32_t width, uint32_t height,
                              uint32_t pitch, uint8_t bpp);
uint32_t vbe_get_framebuffer(void);
void vbe_get_resolution(uint16_t *width, uint16_t *height);
uint8_t vbe_get_bpp(void);
uint16_t vbe_get_pitch(void);
int vbe_is_available(void);

// VBE 模式信息结构 (导出给用户)
#pragma pack(1)
typedef struct {
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t  bits_per_pixel;
    uint32_t phys_base_ptr;
    uint16_t bytes_per_scanline;
    uint16_t mode_attributes;
} vbe_mode_info_user_t;
#pragma pack()

#endif // _VBE_H
