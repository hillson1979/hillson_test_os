#include "minecraft/world.hpp"
#include "minecraft/mesh.hpp"
#include "GL/gl.h"
#include "../user/libuser.h"

int main() {
  minecraft::World world;
  minecraft::Mesh mesh = minecraft::buildChunkMesh(world.starterChunk());
  GLVertex3D vertices[36];
  int count = 0;
  for (std::size_t i = 0; i < mesh.vertices.size() && count < 36; ++i) {
    const minecraft::MeshVertex& v = mesh.vertices[i];
    vertices[count++] = {v.x * 16 - 120, v.y * 16 - 40, v.z * 16 + 320,
                         v.color, v.u, v.v};
  }
  glClearColor(0.10f, 0.25f, 0.55f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glPerspectiveFixed(420);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  while (1) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glSetRotationStep(0);
    glDrawTriangles3D(vertices, count);
    if (glPresent() != 0) return -1;
    yield();
  }
  return 0;
}
