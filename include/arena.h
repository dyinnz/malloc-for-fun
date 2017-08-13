
#pragma once

#include "size_classes.h"
#include "chunk.h"
#include "list.h"

typedef List<Chunk> SlabBin;

class Arena {
  public:
    Arena()
      : chunk_mgr_(base_alloc_) {
      }

    void *SmallAlloc(size_t class_size, size_t slab_index) {
      return nullptr;
    }

    void *LargeAlloc(size_t class_size, size_t page_index) {
      return nullptr;
    }

  private:
    BaseAllocator base_alloc_;
    ChunkManager chunk_mgr_;
    SlabBin nonfull_slab_[kNumSmallClasses];
    SlabBin full_slab_[kNumSmallClasses];
};
