#include "GL/gl.h"
static GLfloat clear_red, clear_green, clear_blue, clear_alpha;
static graph_context_t* current_context;
void glSetContext(graph_context_t* context) { current_context = context; }
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { clear_red=r; clear_green=g; clear_blue=b; clear_alpha=a; }
void glClear(GLenum mask) {
    (void)mask;
    if (current_context) {
        uint32_t r=(uint32_t)(clear_red*255.0f), g=(uint32_t)(clear_green*255.0f), b=(uint32_t)(clear_blue*255.0f);
        graph_context_clear(current_context, 0xFF000000u | (r<<16) | (g<<8) | b);
    }
}
void glEnable(GLenum capability) { (void)capability; }
void glViewport(int x, int y, int width, int height) { if (current_context) graph_context_set_viewport(current_context, x, y, width, height); }
void glDrawArrays(GLenum mode, int first, GLsizei count) { (void)mode; (void)first; (void)count; (void)clear_red; (void)clear_green; (void)clear_blue; (void)clear_alpha; }
