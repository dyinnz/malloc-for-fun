
#include <cassert>

#include "chunk.h"

namespace ffmalloc {

static void
RegisterRtree(Chunk *chunk) {
  char *head = chunk->char_addr();
  // first page
  Static::chunk_rtree()->Insert(head, chunk);
  // last page
  if (chunk->size() > kPage) {
    char *tail = head + chunk->size() - kPage;
    Static::chunk_rtree()->Insert(tail, chunk);
  }
}

static void
DeregisterRtree(Chunk *chunk) {
  char *head = chunk->char_addr();
  // first page
  Static::chunk_rtree()->Delete(head);
  // last page
  if (chunk->size() > kPage) {
    char *tail = head + chunk->size() - kPage;
    Static::chunk_rtree()->Delete(tail);
  }
}

static void
RegisterRtreeInterior(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  for (char *curr = head + kPage; curr < tail; curr += kPage) {
    Static::chunk_rtree()->Insert(curr, chunk);
  }
}

static void
DeregisterRtreeInterior(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  for (char *curr = head + kPage; curr < tail; curr += kPage) {
    Static::chunk_rtree()->Delete(curr);
  }
}

static size_t
get_fit_pind(Chunk *chunk) {
  return sz_to_pind(chunk->size() + 1) - 1;
}

Chunk *
ChunkManager::OSMapChunk(size_t cs, size_t slab_region_size) {
  void *chunk_data = OSAllocMap(cs);
  if (nullptr != chunk_data) {
    Chunk *chunk = base_alloc_.New<Chunk>(chunk_data, cs, &arena_, Chunk::State::kDirty, slab_region_size);
    if (nullptr == chunk) {
      fprintf(stderr, "%s() alloc chunk meta failed\n", __func__);
      return nullptr;

    } else {
      RegisterRtree(chunk);
      return chunk;
    }
  }
  return nullptr;
}

void
ChunkManager::OSUnmapChunk(Chunk *chunk) {
  DeregisterRtree(chunk);
  OSDallocMap(chunk->address(), chunk->size());
}

Chunk *
ChunkManager::SplitChunk(Chunk *curr, size_t head_size) {
  assert(head_size < curr->size());
  DeregisterRtree(curr);

  Chunk *tail = base_alloc_.New<Chunk>(curr->char_addr() + head_size,
                                       curr->size() - head_size,
                                       &arena_,
                                       curr->state(),
                                       kNonSlabAttr);
  curr->set_size(head_size);

  RegisterRtree(curr);
  RegisterRtree(tail);
  return tail;
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, size_t slab_region_size) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;

  /*
  if (pind < kNumGePageClasses && !avail_bins[pind].empty()) {
    chunk = avail_bins[pind].pop();
    */

  if (pind < kNumGePageClasses) {
    // first fit
    for (size_t i = pind; i < kNumGePageClasses; ++i) {
      if (!avail_bins[i].empty()) {
        chunk = avail_bins[i].pop();
        break;
      }
    }
    // mmap if no available chunk
    if (nullptr == chunk) {
      chunk = OSMapChunk(kStandardChunk, 0);
      if (nullptr == chunk) {
        return nullptr;
      }
    }

    // split if gets a larger chunk
    if (chunk->size() > cs) {
      Chunk *tail = SplitChunk(chunk, cs);
      avail_bins[get_fit_pind(tail)].push(tail);
    }

  } else {
    chunk = OSMapChunk(cs, slab_region_size);
    if (nullptr == chunk) {
      return nullptr;
    }
  }

  assert(Chunk::State::kDirty == chunk->state());
  assert(&arena_ == chunk->arena());
  assert(chunk->size() == sz_to_cs(chunk->size()));

  chunk->set_slab_region_size(slab_region_size);
  chunk->set_state(Chunk::State::kActive);
  if (chunk->is_slab()) {
    RegisterRtreeInterior(chunk);
  }

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
  if (chunk->is_slab()) {
    DeregisterRtreeInterior(chunk);
  }

  if (pind < kNumGePageClasses && avail_bins[pind].size() < kMaxBinSize) {
    avail_bins[pind].push(chunk);

  } else {
    OSUnmapChunk(chunk);
  }
}

} // end of namespace ffmalloc
