#include "graph/graph.h"


int graph_init(graph_render_context_t *ctx, graph_framebuffer_t framebuffer,
               uint32_t *depth_values) {
    if (!ctx || !framebuffer.pixels || !framebuffer.width || !framebuffer.height)
        return -1;
    ctx->framebuffer = framebuffer;
    ctx->depth.values = depth_values;
    ctx->depth.width = framebuffer.width;
    ctx->depth.height = framebuffer.height;
    return 0;
}

void graph_shutdown(graph_render_context_t *ctx) {
    if (ctx) { ctx->framebuffer.pixels = 0; ctx->depth.values = 0; }
}

void graph_clear(graph_render_context_t *ctx, uint32_t color, uint32_t depth) {
    uint32_t y;
    if (!ctx || !ctx->framebuffer.pixels) return;
    for (y = 0; y < ctx->framebuffer.height; ++y) {
        uint32_t x;
        uint32_t *row = ctx->framebuffer.pixels + y * ctx->framebuffer.pitch_pixels;
        for (x = 0; x < ctx->framebuffer.width; ++x) row[x] = color;
    }
    if (ctx->depth.values)
        for (y = 0; y < ctx->depth.height * ctx->depth.width; ++y) ctx->depth.values[y] = depth;
}

void graph_put_pixel(graph_render_context_t *ctx, int x, int y, uint32_t color) {
    if (!ctx || x < 0 || y < 0 || (uint32_t)x >= ctx->framebuffer.width ||
        (uint32_t)y >= ctx->framebuffer.height) return;
    ctx->framebuffer.pixels[(uint32_t)y * ctx->framebuffer.pitch_pixels + (uint32_t)x] = color;
}

void graph_fill_rect(graph_render_context_t *ctx, int x, int y, int width, int height,
                     uint32_t color) {
    int iy, ix;
    for (iy = y; iy < y + height; ++iy) for (ix = x; ix < x + width; ++ix)
        graph_put_pixel(ctx, ix, iy, color);
}

void graph_fill_triangle(graph_render_context_t *ctx, int x0, int y0, int x1, int y1,
                         int x2, int y2, uint32_t color) {
    int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    int y, x;
    long e0, e1, e2;
    for (y = miny; y <= maxy; ++y) for (x = minx; x <= maxx; ++x) {
        e0 = (long)(x1 - x0) * (y - y0) - (long)(y1 - y0) * (x - x0);
        e1 = (long)(x2 - x1) * (y - y1) - (long)(y2 - y1) * (x - x1);
        e2 = (long)(x0 - x2) * (y - y2) - (long)(y0 - y2) * (x - x2);
        if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0))
            graph_put_pixel(ctx, x, y, color);
    }
}

int graph_depth_test_and_write(graph_render_context_t *ctx, int x, int y, uint32_t depth) {
    uint32_t index;
    if (!ctx || !ctx->depth.values || x < 0 || y < 0 ||
        (uint32_t)x >= ctx->depth.width || (uint32_t)y >= ctx->depth.height) return 0;
    index = (uint32_t)y * ctx->depth.width + (uint32_t)x;
    if (depth >= ctx->depth.values[index]) return 0;
    ctx->depth.values[index] = depth;
    return 1;
}

int graph_present(graph_render_context_t *ctx) {
    (void)ctx;
    return 0;
}
