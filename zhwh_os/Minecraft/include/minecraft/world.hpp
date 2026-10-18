#pragma once

typedef unsigned char mc_uint8_t;
typedef unsigned int mc_size_t;

namespace minecraft {

enum class Block : mc_uint8_t { Air = 0, Grass, Dirt, Stone };

inline constexpr int kChunkSize = 16;
inline constexpr int kChunkHeight = 64;

class Chunk {
 public:
  Chunk();
  Block get(int x, int y, int z) const;
  void set(int x, int y, int z, Block block);
  void generateFlat(int groundHeight = 8);
  mc_size_t solidBlockCount() const;

 private:
  Block blocks_[kChunkSize * kChunkHeight * kChunkSize]{};
  static mc_size_t index(int x, int y, int z);
};

class World {
 public:
  World();
  void generateStarterArea();
  const Chunk& starterChunk() const { return starter_; }

 private:
  Chunk starter_;
};

}  // namespace minecraft
