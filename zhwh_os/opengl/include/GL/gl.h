#ifndef HILLSON_GL_H
#define HILLSON_GL_H

#include <stdint.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;

#define GL_COLOR_BUFFER_BIT 0x00004000u
#define GL_DEPTH_BUFFER_BIT 0x00000100u
#define GL_DEPTH_TEST 0x0B71u
#define GL_TRIANGLES 0x0004u
#define GL_TEXTURE_2D 0x0DE1u

typedef struct {
    int x;
    int y;
    uint32_t color;
    int u;
    int v;
    uint32_t depth;
    int uq;
    int vq;
    int q;
} GLVertex2D;

typedef struct {
    int x;
    int y;
    int z;
    uint32_t color;
    int u;
    int v;
} GLVertex3D;

typedef struct { int32_t m[16]; } GLMatrix4;
#define GL_MODELVIEW 0x1700u
#define GL_PROJECTION 0x1701u

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClear(GLenum mask);
void glEnable(GLenum capability);
void glViewport(int x, int y, int width, int height);
void glDrawArrays(GLenum mode, int first, GLsizei count);
void glVertexData2D(const GLVertex2D* vertices, GLsizei count);
void glTexImage2D(GLsizei width, GLsizei height, const uint32_t* pixels);
void glDrawTriangles3D(const GLVertex3D* vertices, GLsizei count);
void glSetRotationStep(int step);
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glTranslateFixed(int x, int y, int z);
void glRotateYStep(int step);
void glPerspectiveFixed(int focal_length);
int glPresent(void);

#endif
