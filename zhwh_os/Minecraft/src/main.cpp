#include "minecraft/world.hpp"
#include "minecraft/mesh.hpp"
#include "GL/gl.h"
extern "C" {
#include "../user/libuser.h"
}

int main() {
  minecraft::World world;
  const unsigned int capacity = 32768;
  minecraft::MeshVertex* meshStorage = (minecraft::MeshVertex*)sbrk(capacity * sizeof(minecraft::MeshVertex));
  GLVertex3D* vertices = (GLVertex3D*)sbrk(capacity * sizeof(GLVertex3D));
  if ((int)meshStorage < 0 || (int)vertices < 0) return -1;
  minecraft::Mesh mesh(meshStorage, capacity);
  if (minecraft::buildChunkMesh(world.starterChunk(), mesh) < 0) return -1;
  int count = 0;
  for (unsigned int i = 0; i < mesh.vertexCount && count < 32768; ++i) {
    const minecraft::MeshVertex& v = mesh.vertices[i];
    vertices[count++] = {v.x * 48 - 360, v.y * 48 - 120, v.z * 48 + 420,
                         v.color, v.u, v.v};
  }
  glClearColor(0.10f, 0.25f, 0.55f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glPerspectiveFixed(420);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  int frame = 0;
  while (1) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glSetRotationStep(frame >> 2);
    for (int offset = 0; offset < count; offset += 36) {
      int batch = count - offset;
      if (batch > 36) batch = 36;
      glDrawTriangles3D(vertices + offset, batch);
    }
    if (glPresent() != 0) return -1;
    yield();
    ++frame;
  }
  return 0;
}
