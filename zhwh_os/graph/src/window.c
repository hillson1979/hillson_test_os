#include "graph/window.h"
static graph_window_t static_window;
static int static_window_used;
graph_window_t* graph_window_create(uint32_t width, uint32_t height, const char* title) {
    framebuffer_info_t fb; graph_window_t* w; (void)title;
    if (framebuffer_set_mode(width, height) != 0 || framebuffer_get_info(&fb) != 0) return 0;
    if (static_window_used) return 0;
    w = &static_window; static_window_used = 1;
    w->should_close = 0;
    if (graph_context_init(&w->context, fb) != 0) { static_window_used = 0; return NULL; }
    return w;
}
void graph_window_destroy(graph_window_t* w) { if (w == &static_window) static_window_used = 0; }
graph_context_t* graph_window_context(graph_window_t* w) { return w ? &w->context : 0; }
int graph_window_should_close(const graph_window_t* w) { return !w || w->should_close; }
void graph_window_request_close(graph_window_t* w) { if (w) w->should_close = 1; }
int graph_poll_events(void) { return 0; }
int graph_swap_buffers(graph_window_t* w) { return w ? graph_context_present(&w->context) : -1; }
