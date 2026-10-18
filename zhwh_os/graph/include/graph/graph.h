#ifndef HILLSON_GRAPH_H
#define HILLSON_GRAPH_H

#include <stdint.h>

typedef struct {
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_pixels;
} graph_framebuffer_t;

typedef struct {
    uint32_t *values;
    uint32_t width;
    uint32_t height;
} graph_depth_buffer_t;

typedef struct {
    graph_framebuffer_t framebuffer;
    graph_depth_buffer_t depth;
} graph_render_context_t;

int graph_init(graph_render_context_t *ctx, graph_framebuffer_t framebuffer,
               uint32_t *depth_values);
void graph_shutdown(graph_render_context_t *ctx);
void graph_clear(graph_render_context_t *ctx, uint32_t color, uint32_t depth);
void graph_put_pixel(graph_render_context_t *ctx, int x, int y, uint32_t color);
void graph_fill_rect(graph_render_context_t *ctx, int x, int y, int width, int height,
                     uint32_t color);
void graph_fill_triangle(graph_render_context_t *ctx, int x0, int y0, int x1, int y1,
                         int x2, int y2, uint32_t color);
int graph_depth_test_and_write(graph_render_context_t *ctx, int x, int y, uint32_t depth);
int graph_present(graph_render_context_t *ctx);

#endif
