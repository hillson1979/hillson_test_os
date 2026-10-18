#ifndef HILLSON_FRAMEBUFFER_H
#define HILLSON_FRAMEBUFFER_H

#include <stdint.h>

typedef struct {
    uint32_t physical_address;
    void* virtual_address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bytes_per_pixel;
} framebuffer_info_t;

int framebuffer_init(void);
int framebuffer_get_info(framebuffer_info_t* info);
int framebuffer_present(void);
int framebuffer_set_mode(uint32_t width, uint32_t height);

#endif
