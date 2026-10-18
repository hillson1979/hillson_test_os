#pragma once
#include "minecraft/world.hpp"
namespace minecraft {
typedef unsigned int mc_uint32_t;
struct MeshVertex { int x,y,z; mc_uint32_t color; int u,v; };
struct Mesh { MeshVertex* vertices; unsigned int capacity; unsigned int vertexCount; Mesh(MeshVertex* storage, unsigned int cap) : vertices(storage), capacity(cap), vertexCount(0) {} };
int buildChunkMesh(const Chunk& chunk, Mesh& mesh);
}
