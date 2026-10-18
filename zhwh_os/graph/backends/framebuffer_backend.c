#include "framebuffer/framebuffer.h"
#include "vbe.h"

/* Platform hook. The kernel display driver should replace these stubs. */
int framebuffer_init(void) { return vbe_is_available() ? 0 : -1; }
int framebuffer_get_info(framebuffer_info_t* info) {
    uint16_t width, height;
    if (!info || !vbe_is_available()) return -1;
    vbe_get_resolution(&width, &height);
    /* vbe_init_from_multiboot maps the physical framebuffer at this address. */
    info->physical_address = vbe_get_framebuffer();
    info->virtual_address = (void*)(uintptr_t)0xF0000000u;
    info->width = width; info->height = height; info->pitch = vbe_get_pitch();
    info->bytes_per_pixel = vbe_get_bpp() / 8;
    return info->virtual_address ? 0 : -1;
}
int framebuffer_present(void) { return 0; }
int framebuffer_set_mode(uint32_t width, uint32_t height) {
    return vbe_set_graphics_mode((uint16_t)width, (uint16_t)height, 32);
}
