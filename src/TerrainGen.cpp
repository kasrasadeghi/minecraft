#include "TerrainGen.h"
#include "World.h"
#include "Player.h"
#include "Perlin.h"

// FIXME: is every chunk actually only loaded once?

void TerrainGen::spawn(World& world, Player& player) {
  auto chunk_index = World::toChunk(player.blockPosition());

  for (int i = -3; i <= 3; ++i) {
    for (int k = -3; k <= 3; ++k) {
      glm::ivec2 curr_index = chunk_index + glm::ivec2(i, k);
      world._chunks[curr_index] = {};
      chunk(world, curr_index);
    }
  }
  for (auto& p : world._chunks) {
    p.second.generated = true;
  }
}

void TerrainGen::chunk(World& world, glm::ivec2 chunk_index) {
  assert (not world._chunks.at(chunk_index).generated);

  int bi = chunk_index.x * CHUNK_SIZE;
  int bk = chunk_index.y * CHUNK_SIZE;

  // FIXME: is everything in this function actually in the same chunk?

  for (int i = 0; i < CHUNK_SIZE; ++i)
  for (int k = 0; k < CHUNK_SIZE; ++k) {
    int h = 50 + perlin((bi + i) / 50.f, (bk + k) / 50.f) * 10;
    world(bi + i, h,   bk + k) = 1;
    world(bi + i, h-1, bk + k) = 1;
    world(bi + i, h-2, bk + k) = 1;
  }

  world._chunks[chunk_index].generated = true;
}