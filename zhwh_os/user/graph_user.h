#ifndef HILLSON_GRAPH_USER_H
#define HILLSON_GRAPH_USER_H
#include "stdint.h"

#define SYS_GRAPH_GET_DISPLAY_INFO 87
#define SYS_GRAPH_PRESENT 88

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bytes_per_pixel;
} graph_display_info_t;

int graph_get_display_info(graph_display_info_t *info);
int graph_present_frame(const uint32_t *pixels, uint32_t width, uint32_t height,
                        uint32_t pitch_bytes);
#endif
