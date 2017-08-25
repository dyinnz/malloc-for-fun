
#pragma once

#include "size_classes.h"
#include "chunk.h"
#include "list.h"
#include "static.h"
#include <iostream>
#include <mutex>

namespace ffmalloc {

class Arena {
 private:
  typedef List<Chunk> SlabBin;

 public:
  Arena()
      : chunk_mgr_(*this, base_alloc_) {
  }

  ~Arena() {
    for (auto &slabs : nonempty_slab_) {
      while (!slabs.empty()) {
        // TODO: should warning here
        DallocChunkWrapper(slabs.pop());
      }
    }
    for (auto &slabs : empty_slab_) {
      while (!slabs.empty()) {
        DallocChunkWrapper(slabs.pop());
      }
    }
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
  Chunk *AllocChunkWrapper(size_t cs, size_t pind, size_t region_size) {
    assert(cs >= kPage && cs % kPage == 0);
    assert(sz_to_pind(cs) == pind);

    std::lock_guard<std::mutex> guard(mutex_);
    return chunk_mgr_.AllocChunk(cs, pind, region_size);
  }

  void DallocChunkWrapper(Chunk *chunk) {
    std::lock_guard<std::mutex> guard(mutex_);
    chunk_mgr_.DallocChunk(chunk);
  }

 private:
  SlabBin nonempty_slab_[kNumSmallClasses];
  SlabBin empty_slab_[kNumSmallClasses];
  std::mutex slab_mutex_[kNumSmallClasses];
  BaseAllocator base_alloc_;
  ChunkManager chunk_mgr_;
  std::mutex mutex_;
} CACHELINE_ALIGN;

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
      char *addr = chunk->char_addr();
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
