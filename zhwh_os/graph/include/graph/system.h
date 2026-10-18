#ifndef HILLSON_GRAPH_SYSTEM_H
#define HILLSON_GRAPH_SYSTEM_H

#include "graph/context.h"

int graph_system_init(void);
int graph_system_get_context(graph_context_t **context);
void graph_system_shutdown(void);

#endif
