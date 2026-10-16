#ifndef HILLSON_FRAMEBUFFER_H
#define HILLSON_FRAMEBUFFER_H

#include <stdint.h>

typedef enum {
    FRAMEBUFFER_FORMAT_XRGB8888 = 1,
    FRAMEBUFFER_FORMAT_RGB565 = 2
} framebuffer_format_t;

typedef struct {
    void* address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bytes_per_pixel;
    framebuffer_format_t format;
} framebuffer_info_t;

int framebuffer_init(void);
int framebuffer_get_info(framebuffer_info_t* info);
int framebuffer_present(void);
int framebuffer_set_mode(uint32_t width, uint32_t height);

#endif
