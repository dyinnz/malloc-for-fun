
#pragma once

#include "size_classes.h"
#include "arena.h"
#include <array>

namespace ffmalloc {

class CacheBin {
 private:
  static constexpr int kMaxCaches = 200;

 public:
  CacheBin(Arena &arena, size_t capacity, size_t ind)
      : arena_(arena), capacity_(capacity), ind_(ind), cs_(ind_to_cs(ind)), num_(0) {
    memset(caches_, 0, sizeof(void *) * capacity_);
  }

  ~CacheBin() {
    FlushCaches(num_);
  }

  void *PopRegion() {
    if (unlikely(0 == num_)) {
      if (!FillCaches(capacity_ / 2)) {
        return nullptr;
      }
    }
    num_ -= 1;
    return caches_[num_];
  }

  void PushRegion(void *ptr) {
    if (unlikely(capacity_ == num_)) {
      FlushCaches(capacity_ / 2);
    }
    caches_[num_] = ptr;
    num_ += 1;
  }

 private:
  bool FillCaches(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      void *ptr = arena_.SmallAlloc(cs_, ind_);
      if (nullptr == ptr) {
        return false;
      }
      caches_[num_] = ptr;
      num_ += 1;
    }
    return true;
  }

  void FlushCaches(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      void *ptr = caches_[i];
      Chunk *slab = Static::chunk_rtree()->LookUp(ptr);
      slab->arena()->SmallDalloc(ptr, slab);
    }
    memmove(caches_, caches_ + count, num_ - count);
    num_ -= count;
  }

 private:
  void *caches_[kMaxCaches];
  Arena &arena_;
  const size_t capacity_;
  const size_t ind_;
  const size_t cs_;
  size_t num_;
};

class ThreadAllocator {
 public:
  ThreadAllocator(Arena &arena) : arena_(arena) {
    memset(cache_bins_, 0, sizeof(CacheBin *) * kNumSmallClasses);
  }

  void *ThreadAlloc(size_t size) {
    if (size <= kMaxSmallClass) {
      size_t ind = sz_to_ind(size);
      return cache_bin(ind)->PopRegion();

    } else {
      size_t pind = sz_to_pind(size);
      size_t cs = sz_to_cs(size);
      return arena_.LargeAlloc(cs, pind);
    }
  }

  void ThreadDalloc(void *ptr) {
    Chunk *chunk = Static::chunk_rtree()->LookUp(ptr);
    if (chunk->is_slab()) {
      size_t ind = sz_to_ind(chunk->slab_region_size());
      return cache_bin(ind)->PushRegion(ptr);
    } else {
      chunk->arena()->LargeDalloc(chunk);
    }
  }

 private:
  CacheBin *cache_bin(size_t ind) {
    if (unlikely(nullptr == cache_bins_[ind])) {
      // TODO:
      cache_bins_[ind] = arena_.base_alloc().New<CacheBin>(arena_, 200, ind);
    }
    return cache_bins_[ind];
  }

 private:
  Arena &arena_;
  CacheBin *cache_bins_[kNumSmallClasses];
};

} // end of namespace ffmalloc
