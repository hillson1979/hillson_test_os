#include "graph/system.h"
#include "framebuffer/framebuffer.h"

static graph_context_t system_context;
static int system_ready;

int graph_system_init(void) {
    framebuffer_info_t info;
    if (system_ready) return 0;
    if (framebuffer_init() != 0 || framebuffer_get_info(&info) != 0) return -1;
    if (graph_context_init(&system_context, info) != 0) return -1;
    system_ready = 1;
    return 0;
}

int graph_system_get_context(graph_context_t **context) {
    if (!context || !system_ready) return -1;
    *context = &system_context;
    return 0;
}

void graph_system_shutdown(void) {
    system_context.framebuffer.address = 0;
    system_ready = 0;
}
