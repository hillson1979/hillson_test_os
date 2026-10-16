#ifndef HILLSON_GL_H
#define HILLSON_GL_H

#include <stdint.h>
#include "graph/context.h"

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;

#define GL_COLOR_BUFFER_BIT 0x00004000u
#define GL_DEPTH_BUFFER_BIT 0x00000100u
#define GL_DEPTH_TEST 0x0B71u
#define GL_TRIANGLES 0x0004u

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClear(GLenum mask);
void glEnable(GLenum capability);
void glViewport(int x, int y, int width, int height);
void glDrawArrays(GLenum mode, int first, GLsizei count);
void glSetContext(graph_context_t* context);

#endif
