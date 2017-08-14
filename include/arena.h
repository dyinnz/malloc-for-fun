
#pragma once

#include "size_classes.h"
#include "chunk.h"
#include "list.h"

typedef List<Chunk> SlabBin;

class Arena {
 public:
  Arena()
      : chunk_mgr_(*this, base_alloc_) {
  }

  void *SmallAlloc(size_t cs, size_t sind) {
    assert(cs <= kMinLargeClass);
    assert(sz_to_ind(sind) == sind);

    Chunk *slab = nullptr;
    if (nonfull_slab_[sind].empty()) {
      const size_t slab_size = g_slab_sizes[sind];
      const size_t pind = sz_to_pind(slab_size);
      slab = AllocChunkWrapper(slab_size, pind, kSlabAttr);
      nonfull_slab_[sind].push(slab);
    }
    // TODO:

    return nullptr;
  }

  void *LargeAlloc(size_t cs, size_t pind) {
    assert(cs >= kMinLargeClass);
    return AllocChunkWrapper(cs, pind, kSlabAttr);
  }

 private:
  Chunk *AllocChunkWrapper(size_t cs, size_t pind, bool is_slab) {
    assert(cs >= kPage && cs % kPage == 0);
    assert(sz_to_pind(cs) == pind);

    return chunk_mgr_.AllocChunk(cs, pind, is_slab);
  }

  void DallocChunkWrapper(Chunk *chunk) {
    chunk_mgr_.DallocChunk(chunk);
  }

 private:
  BaseAllocator base_alloc_;
  ChunkManager chunk_mgr_;
  SlabBin nonfull_slab_[kNumSmallClasses];
  SlabBin full_slab_[kNumSmallClasses];
};
