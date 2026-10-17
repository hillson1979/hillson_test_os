#include "GL/gl.h"
#include "graph_user.h"
static GLfloat clear_red, clear_green, clear_blue, clear_alpha;
static graph_display_info_t display;
static uint32_t framebuffer[1024 * 768];
static int initialized;
static void ensure_init(void) {
    if (!initialized) initialized = graph_get_display_info(&display) == 0;
}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { clear_red=r; clear_green=g; clear_blue=b; clear_alpha=a; }
void glClear(GLenum mask) {
    (void)mask;
    ensure_init();
    if (initialized && (mask & GL_COLOR_BUFFER_BIT)) {
        uint32_t r=(uint32_t)(clear_red*255.0f), g=(uint32_t)(clear_green*255.0f), b=(uint32_t)(clear_blue*255.0f);
        uint32_t y, x, color = 0xFF000000u | (r<<16) | (g<<8) | b;
        if (display.width > 1024 || display.height > 768) return;
        for (y=0; y<display.height; ++y) for (x=0; x<display.width; ++x) framebuffer[y*1024+x] = color;
    }
}
void glEnable(GLenum capability) { (void)capability; }
void glViewport(int x, int y, int width, int height) { (void)x; (void)y; (void)width; (void)height; ensure_init(); }
void glDrawArrays(GLenum mode, int first, GLsizei count) { (void)mode; (void)first; (void)count; (void)clear_red; (void)clear_green; (void)clear_blue; (void)clear_alpha; }
int glPresent(void) { ensure_init(); return initialized ? graph_present_frame(framebuffer, display.width, display.height, 1024 * 4) : -1; }
