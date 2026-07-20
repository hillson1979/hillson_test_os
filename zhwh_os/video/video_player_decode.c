#include "video_player.h"
#include "stb_image_os.h"

// 禁用assert，使用空操作
#define STBI_ASSERT(x) ((void)0)

// 定义内存分配宏
#define STBI_MALLOC(sz)    kmalloc(sz)
#define STBI_FREE(p)       kfree(p)
#define STBI_REALLOC(p,s)  stbi__realloc(p,s)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* ====== JPEG解码到XRGB8888 (32位颜色) ====== */
int jpeg_decode_to_rgb565(uint8_t *jpeg, int len, uint16_t *out)
{
    int w, h, ch;
    uint8_t *rgb;
    int i;
    uint32_t *out32 = (uint32_t *)out;  // 强制转换为 32 位指针

    /* 从内存解码JPEG */
    rgb = stbi_load_from_memory(jpeg, len, &w, &h, &ch, 3);
    if (!rgb) {
        return -1;  // 解码失败
    }

    /* 检查尺寸 */
    if (w != VIDEO_W || h != VIDEO_H) {
        stbi_image_free(rgb);
        return -2;  // 尺寸不匹配
    }

    /* 转换RGB888到XRGB8888 (32位颜色) */
    for (i = 0; i < w * h; i++) {
        uint8_t r = rgb[i * 3 + 0];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        /* XRGB8888: 0x00RRGGBB */
        out32[i] = 0xFF000000 | (r << 16) | (g << 8) | b;  // 0xFF为不透明
    }

    stbi_image_free(rgb);
    return 0;
}

/* ====== 高性能优化版本（可选） ====== */
#if 0
int jpeg_decode_to_rgb565_fast(uint8_t *jpeg, int len, uint16_t *out)
{
    /* 使用picojpeg等更轻量的解码器 */
    /* 或者使用硬件加速（如果平台支持） */
    return jpeg_decode_to_rgb565(jpeg, len, out);
}
#endif

/* ====== 获取解码器版本信息 ====== */
const char* jpeg_decoder_version(void)
{
    return "stb_image 2.x";
}
