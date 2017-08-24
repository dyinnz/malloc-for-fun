
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
    memmove(caches_, caches_ + count, (num_ - count) * sizeof(void *));
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

  ~ThreadAllocator() {
    for (auto &bin : cache_bins_) {
      if (nullptr != bin) {
        arena_.base_alloc().Delete(bin);
      }
    }
  }

  void *ThreadAlloc(size_t size) {
    if (unlikely(0 == size)) {
      return nullptr;
    }
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
    if (likely(nullptr != ptr)) {
      Chunk *chunk = Static::chunk_rtree()->LookUp(ptr);
      if (chunk->is_slab()) {
        size_t ind = sz_to_ind(chunk->slab_region_size());
        cache_bin(ind)->PushRegion(ptr);
      } else {
        chunk->arena()->LargeDalloc(chunk);
      }
    }
  }

  void *ThreadRealloc(void *old, size_t size) {
    if (unlikely(nullptr == old)) {
      return ThreadAlloc(size);
    }
    if (unlikely(0 == size)) {
      ThreadDalloc(old);
      return nullptr;
    }
    void *addr = ThreadAlloc(size);
    if (nullptr == addr) {
      return nullptr;
    }

    Chunk *chunk = Static::chunk_rtree()->LookUp(old);
    if (chunk->is_slab()) {
      size_t old_size = chunk->slab_region_size();
      memmove(addr, old, std::min(old_size, size));
      size_t ind = sz_to_ind(old_size);
      cache_bin(ind)->PushRegion(old);

    } else {
      size_t old_size = chunk->size();
      memmove(addr, old, std::min(old_size, size));
      chunk->arena()->LargeDalloc(chunk);
    }
    return addr;
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
  CacheBin *cache_bins_[kNumSmallClasses];
  Arena &arena_;
} CACHELINE_ALIGN;

} // end of namespace ffmalloc
