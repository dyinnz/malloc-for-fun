
#pragma once

#include "size_classes.h"
#include "chunk.h"
#include "list.h"
#include "static.h"
#include <iostream>
#include <mutex>
#include <array>

namespace ffmalloc {

class Arena {
 private:
  typedef List<Chunk> SlabBin;

 public:
  struct SmallStat {
    size_t hold {0};
    size_t request {0};
  };

  typedef std::array<SmallStat, kNumSmallClasses> SmallStatArray;

  struct LargeStat {
    size_t request {0};
  };

  Arena()
      : chunk_mgr_(*this, base_alloc_) {
  }

  Arena(const Arena&) = delete;
  Arena(Arena&&) = delete;
  Arena& operator=(const Arena&) = delete;
  Arena& operator=(Arena&&) = delete;

  ~Arena();

  void *SmallAlloc(size_t cs, size_t sind);
  void SmallDalloc(void *region, Chunk *slab);

  void *LargeAlloc(size_t cs, size_t pind) {
    assert(cs >= kMinLargeClass);
    Chunk *chunk = AllocChunkWrapper(cs, pind, kNonSlabAttr);
    if (nullptr == chunk) {
      func_error(logger, "alloc large chunk failed");
      return nullptr;
    } else {
      return chunk->address();
    }
  }

  void LargeDalloc(Chunk *chunk) {
    DallocChunkWrapper(chunk);
  }

  BaseAllocator &base_alloc() {
    return base_alloc_;
  }

  ChunkManager &chunk_manager() {
    return chunk_mgr_;
  }

  const SmallStatArray& small_stats() const {
    return small_stats_;
  }

  const LargeStat& large_stat() const {
    return large_stat_;
  }

 private:
  Chunk *AllocChunkWrapper(size_t cs, size_t pind, size_t region_size) {
    assert(cs >= kPage && cs % kPage == 0);
    assert(sz_to_pind(cs) == pind);

    std::lock_guard<std::mutex> guard(mutex_);
    Chunk *chunk = chunk_mgr_.AllocChunk(cs, pind, region_size);
    if (nullptr != chunk && 0 == region_size) {
      large_stat_.request += cs;
    }
    return chunk;
  }

  void DallocChunkWrapper(Chunk *chunk) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (!chunk->is_slab()) {
      large_stat_.request -= chunk->size();
    }
    chunk_mgr_.DallocChunk(chunk);
  }

 private:
  SlabBin nonempty_slab_[kNumSmallClasses];
  SlabBin empty_slab_[kNumSmallClasses];
  std::mutex slab_mutex_[kNumSmallClasses];
  SmallStatArray small_stats_;
  LargeStat large_stat_;
  BaseAllocator base_alloc_;
  ChunkManager chunk_mgr_;
  std::mutex mutex_;
} CACHELINE_ALIGN;

class ArenaAllocator {
 public:
  ArenaAllocator() = default;
  ArenaAllocator(const ArenaAllocator&) = delete;
  ArenaAllocator(ArenaAllocator&&) = delete;
  ArenaAllocator& operator=(const ArenaAllocator&) = delete;
  ArenaAllocator& operator=(ArenaAllocator&&) = delete;

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
    assert(chunk->char_addr() <= ptr && ptr < chunk->char_addr() + chunk->size());
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
