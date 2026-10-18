#include "minecraft/world.hpp"

namespace minecraft {
mc_size_t Chunk::index(int x, int y, int z) {
  if (x < 0 || x >= kChunkSize || y < 0 || y >= kChunkHeight || z < 0 || z >= kChunkSize) return 0;
  return (mc_size_t)((y * kChunkSize + z) * kChunkSize + x);
}

Chunk::Chunk() { for (mc_size_t i=0; i<kChunkSize*kChunkHeight*kChunkSize; ++i) blocks_[i]=Block::Air; }
Block Chunk::get(int x, int y, int z) const { return blocks_[index(x, y, z)]; }
void Chunk::set(int x, int y, int z, Block block) { blocks_[index(x, y, z)] = block; }

void Chunk::generateFlat(int groundHeight) {
  for (int y = 0; y < groundHeight && y < kChunkHeight; ++y)
    for (int z = 0; z < kChunkSize; ++z)
      for (int x = 0; x < kChunkSize; ++x)
        set(x, y, z, y == groundHeight - 1 ? Block::Grass : (y < 3 ? Block::Stone : Block::Dirt));
}

mc_size_t Chunk::solidBlockCount() const {
  mc_size_t count = 0;
  for (Block block : blocks_) if (block != Block::Air) ++count;
  return count;
}

World::World() { generateStarterArea(); }
void World::generateStarterArea() { starter_.generateFlat(); }
}  // namespace minecraft
