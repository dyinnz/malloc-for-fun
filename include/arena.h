
#pragma once

#include "size_classes.h"
#include "chunk.h"
#include "list.h"
#include "static.h"
#include <iostream>

namespace ffmalloc {

class Arena {
 private:
  typedef List<Chunk> SlabBin;

 public:
  Arena()
      : chunk_mgr_(*this, base_alloc_) {
  }

  void *SmallAlloc(size_t cs, size_t sind);
  void SmallDalloc(void *region, Chunk *slab);

  void *LargeAlloc(size_t cs, size_t pind) {
    assert(cs >= kMinLargeClass);
    Chunk *chunk = AllocChunkWrapper(cs, pind, kNonSlabAttr);
    if (nullptr == chunk) {
      return nullptr;
    } else {
      return chunk->address();
    }
  }

  void LargeDalloc(Chunk *chunk) {
    DallocChunkWrapper(chunk);
  }

  BaseAllocator& base_alloc() {
    return base_alloc_;
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
  SlabBin nonempty_slab[kNumSmallClasses];
  SlabBin empty_slab[kNumSmallClasses];
} __attribute__((aligned (128)));

class ArenaAllocator {
 public:
  void *ArenaAlloc(size_t size) {
    size_t cs = sz_to_cs(size);
    if (size <= kMaxSmallClass) {
      size_t sind = sz_to_ind(size);
      return arena_.SmallAlloc(cs, sind);

    } else {
      size_t pind = sz_to_pind(cs);
      return arena_.LargeAlloc(cs, pind);
    }
  }

  void ArenaDalloc(void *ptr) {
    Chunk *chunk = Static::chunk_rtree()->LookUp(ptr);
    {
      char *addr = static_cast<char *>(chunk->address());
      assert(addr <= ptr && ptr < addr + chunk->size());
    }
    if (chunk->is_slab()) {
      arena_.SmallDalloc(ptr, chunk);
    } else {
      assert(chunk->address() == ptr);
      arena_.LargeDalloc(chunk);
    }
  }

 private:
  Arena arena_;
};

} // end of namespace ffmalloc
