#include "graph/context.h"
int graph_context_init(graph_context_t* c, framebuffer_info_t fb) {
    if (!c || !fb.virtual_address || !fb.width || !fb.height || fb.bytes_per_pixel != 4) return -1;
    c->framebuffer = fb; c->clear_color = 0; c->viewport_x = 0; c->viewport_y = 0;
    c->viewport_width = (int)fb.width; c->viewport_height = (int)fb.height; return 0;
}
void graph_context_clear(graph_context_t* c, uint32_t color) {
    uint32_t y, x; uint8_t* base;
    if (!c || !c->framebuffer.virtual_address || c->framebuffer.bytes_per_pixel != 4) return;
    c->clear_color = color; base = (uint8_t*)c->framebuffer.virtual_address;
    for (y=0; y<c->framebuffer.height; ++y) for (x=0; x<c->framebuffer.width; ++x)
        ((uint32_t*)(base + y*c->framebuffer.pitch))[x] = color;
}
void graph_context_set_viewport(graph_context_t* c, int x, int y, int w, int h) {
    if (!c) return; c->viewport_x=x; c->viewport_y=y; c->viewport_width=w; c->viewport_height=h;
}
int graph_context_present(graph_context_t* c) { (void)c; return framebuffer_present(); }
