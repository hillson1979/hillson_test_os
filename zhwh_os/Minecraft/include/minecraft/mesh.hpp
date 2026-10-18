#pragma once
#include "minecraft/world.hpp"
#include <cstdint>
#include <vector>
namespace minecraft {
struct MeshVertex { int x,y,z; uint32_t color; int u,v; };
struct Mesh { std::vector<MeshVertex> vertices; };
Mesh buildChunkMesh(const Chunk& chunk);
}
