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
#define VBE_MODE_118 0x118  // 640x480x24
#define VBE_MODE_111 0x111  // 640x480x15

// VBE 函数 (内核中使用)
int vbe_detect(void);
int vbe_set_mode(uint16_t mode);
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
