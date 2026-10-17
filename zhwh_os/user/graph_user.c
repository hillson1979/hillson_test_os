#include "graph_user.h"

int graph_get_display_info(graph_display_info_t *info) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret)
                     : "a"(SYS_GRAPH_GET_DISPLAY_INFO), "b"(info)
                     : "memory", "cc");
    return ret;
}

int graph_present_frame(const uint32_t *pixels, uint32_t width, uint32_t height,
                        uint32_t pitch_bytes) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret)
                     : "a"(SYS_GRAPH_PRESENT), "b"(pixels), "c"(width),
                       "d"(height), "S"(pitch_bytes)
                     : "memory", "cc");
    return ret;
}
