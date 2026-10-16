#include "framebuffer/framebuffer.h"
#include "vbe.h"

/* Platform hook. The kernel display driver should replace these stubs. */
int framebuffer_init(void) { return vbe_is_available() ? 0 : -1; }
int framebuffer_get_info(framebuffer_info_t* info) {
    uint16_t width, height;
    if (!info || !vbe_is_available()) return -1;
    vbe_get_resolution(&width, &height);
    info->address = (void*)(uintptr_t)vbe_get_framebuffer();
    info->width = width; info->height = height; info->pitch = vbe_get_pitch();
    info->bytes_per_pixel = vbe_get_bpp() / 8;
    info->format = info->bytes_per_pixel == 4 ? FRAMEBUFFER_FORMAT_XRGB8888 : FRAMEBUFFER_FORMAT_RGB565;
    return info->address ? 0 : -1;
}
int framebuffer_present(void) { return 0; }
int framebuffer_set_mode(uint32_t width, uint32_t height) {
    return vbe_set_graphics_mode((uint16_t)width, (uint16_t)height, 32);
}
