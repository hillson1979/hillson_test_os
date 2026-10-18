#include "graph/selftest.h"
#include "graph/system.h"
#include "graph/graph.h"

int graph_selftest(void) {
    graph_context_t *context;
    graph_framebuffer_t framebuffer;
    graph_render_context_t render;
    if (graph_system_init() != 0 || graph_system_get_context(&context) != 0) return -1;
    framebuffer.pixels = (uint32_t*)context->framebuffer.virtual_address;
    framebuffer.width = context->framebuffer.width;
    framebuffer.height = context->framebuffer.height;
    framebuffer.pitch_pixels = context->framebuffer.pitch / 4;
    if (graph_init(&render, framebuffer, 0) != 0) return -1;
    graph_clear(&render, 0xFF202830, 0xFFFFFFFFu);
    graph_fill_rect(&render, 40, 40, 240, 120, 0xFF20A060);
    graph_fill_triangle(&render, 400, 120, 520, 320, 280, 320, 0xFFE0C040);
    return graph_context_present(context);
}
