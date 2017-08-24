
#include <cassert>

#include "chunk.h"

namespace ffmalloc {

static void RegisterRadix(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  // first page
  Static::chunk_rtree()->Insert(head, chunk);

  // last page
  if (chunk->size() > kPage) {
    Static::chunk_rtree()->Insert(tail, chunk);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      Static::chunk_rtree()->Insert(curr, chunk);
    }
  }
}

static void DeregisterRadix(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  // first page
  Static::chunk_rtree()->Delete(head);

  // last page
  if (chunk->size() > kPage) {
    Static::chunk_rtree()->Delete(tail);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      Static::chunk_rtree()->Delete(curr);
    }
  }
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, size_t slab_region_size) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;
  if (pind < kNumGePageClasses && !avail_bins[pind].empty()) {

    chunk = avail_bins[pind].pop();

  } else {
    void *chunk_data = OSAllocMap(cs);
    chunk = base_alloc_.New<Chunk>(chunk_data, cs, &arena_, Chunk::State::kDirty, slab_region_size);

    if (nullptr == chunk) {
      fprintf(stderr, "%s() ArenaAlloc from map failed: size %zu\n", __func__, cs);
      return nullptr;
    }
  }

  assert(nullptr != chunk);
  assert(Chunk::State::kDirty == chunk->state());
  assert(&arena_ == chunk->arena());
  assert(chunk->size() == sz_to_cs(chunk->size()));

  chunk->set_slab_region_size(slab_region_size);
  chunk->set_state(Chunk::State::kActive);
  RegisterRadix(chunk);

  return chunk;
}

void
ChunkManager::DallocChunk(Chunk *chunk) {
  assert(nullptr != chunk);
  assert(Chunk::State::kActive == chunk->state());
  assert(&arena_ == chunk->arena());
  assert(chunk->size() == sz_to_cs(chunk->size()));

  size_t pind = sz_to_pind(chunk->size());

  chunk->set_state(Chunk::State::kDirty);
  DeregisterRadix(chunk);

  if (pind < kNumGePageClasses && avail_bins[pind].size() < kMaxBinSize) {
    avail_bins[pind].push(chunk);

  } else {

    OSDallocMap(chunk->address(), chunk->size());
    base_alloc_.Delete(chunk);
  }
}

} // end of namespace ffmalloc
