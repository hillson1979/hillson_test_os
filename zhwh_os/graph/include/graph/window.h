#ifndef HILLSON_GRAPH_WINDOW_H
#define HILLSON_GRAPH_WINDOW_H
#include <stdint.h>
#include "graph/context.h"
typedef struct graph_window { graph_context_t context; int should_close; } graph_window_t;
graph_window_t* graph_window_create(uint32_t width, uint32_t height, const char* title);
void graph_window_destroy(graph_window_t* window);
graph_context_t* graph_window_context(graph_window_t* window);
int graph_window_should_close(const graph_window_t* window);
void graph_window_request_close(graph_window_t* window);
int graph_poll_events(void);
int graph_swap_buffers(graph_window_t* window);
#endif
