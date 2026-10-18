#ifndef HILLSON_GRAPH_CONTEXT_H
#define HILLSON_GRAPH_CONTEXT_H
#include <stdint.h>
#include "framebuffer/framebuffer.h"
typedef struct graph_context {
    framebuffer_info_t framebuffer;
    uint32_t clear_color;
    int viewport_x, viewport_y, viewport_width, viewport_height;
} graph_context_t;
int graph_context_init(graph_context_t* context, framebuffer_info_t framebuffer);
void graph_context_clear(graph_context_t* context, uint32_t color);
void graph_context_set_viewport(graph_context_t* context, int x, int y, int width, int height);
int graph_context_present(graph_context_t* context);
#endif
