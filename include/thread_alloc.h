
#pragma once

#include "size_classes.h"
#include "arena.h"
#include <array>
#include "array"

namespace ffmalloc {

/*------------------------------------------------------------------*/

class CacheBin {
 private:
  static constexpr int kMaxCaches = 200;

 public:
  CacheBin(Arena &arena, size_t capacity, size_t ind)
      : arena_(arena), capacity_(capacity), ind_(ind), cs_(ind_to_cs(ind)), num_(0) {
    memset(caches_, 0, sizeof(void *) * capacity_);
  }

  CacheBin(const CacheBin &) = delete;
  CacheBin(CacheBin &&) = delete;
  CacheBin &operator=(const CacheBin &) = delete;
  CacheBin &operator=(CacheBin &&) = delete;

  ~CacheBin() {
    FlushCaches(num_);
  }

  void *PopRegion();

  void PushRegion(void *ptr);

  size_t num_cache() const {
    return num_;
  }

 private:
  size_t FillCaches(size_t count);

  void FlushCaches(size_t count);

 private:
  void *caches_[kMaxCaches];
  Arena &arena_;
  const size_t capacity_;
  const size_t ind_;
  const size_t cs_;
  size_t num_;
} CACHELINE_ALIGN;

/*------------------------------------------------------------------*/

class ThreadAllocator {
 public:
  struct CacheStat {
    size_t num_alloc{0};
    size_t num_free{0};
    size_t num_cache{0};
  };

  struct LargeStat {
    size_t alloc{0};
    size_t free{0};
  };

  typedef std::array<CacheStat, kNumSmallClasses> CacheStatArray;

  ThreadAllocator(Arena &arena) : arena_(arena) {
    memset(cache_bins_, 0, sizeof(CacheBin *) * kNumSmallClasses);
  }

  ThreadAllocator(const ThreadAllocator &) = delete;
  ThreadAllocator(ThreadAllocator &&) = delete;
  ThreadAllocator &operator=(const ThreadAllocator &) = delete;
  ThreadAllocator &operator=(ThreadAllocator &&) = delete;

  ~ThreadAllocator();

  void *ThreadAlloc(size_t size);

  void ThreadDalloc(void *ptr);

  void *ThreadRealloc(void *old, size_t size);

  const CacheStatArray &cache_stats();

  const LargeStat &large_stat() {
    return large_stat_;
  }

 private:
  CacheBin *cache_bin(size_t ind) {
    if (unlikely(nullptr == cache_bins_[ind])) {
      // TODO: nullptr check
      cache_bins_[ind] = arena_.base_alloc().New<CacheBin>(arena_, 200, ind);
    }
    return cache_bins_[ind];
  }

 private:
  CacheBin *cache_bins_[kNumSmallClasses];
  CacheStatArray cache_stats_;
  LargeStat large_stat_;
  Arena &arena_;
} CACHELINE_ALIGN;

/*------------------------------------------------------------------*/
// inline functions of CacheBin

inline void *
CacheBin::PopRegion() {
  if (unlikely(0 == num_)) {
    if (0 == FillCaches(capacity_ / 2)) {
      func_error(logger, "fill caches failed");
      return nullptr;
    }
  }
  num_ -= 1;
  return caches_[num_];
}

inline void
CacheBin::PushRegion(void *ptr) {
  if (unlikely(capacity_ == num_)) {
    FlushCaches(capacity_ / 2);
  }
  caches_[num_] = ptr;
  num_ += 1;
}

inline size_t
CacheBin::FillCaches(size_t count) {
  for (size_t i = 0; i < count; ++i) {
    void *ptr = arena_.SmallAlloc(cs_, ind_);
    if (nullptr == ptr) {
      return i;
    }
    caches_[num_] = ptr;
    num_ += 1;
  }
  return count;
}

inline void
CacheBin::FlushCaches(size_t count) {
  for (size_t i = 0; i < count; ++i) {
    void *ptr = caches_[i];
    Chunk *slab = Static::chunk_rtree()->LookUp(ptr);
    slab->arena()->SmallDalloc(ptr, slab);
  }
  memmove(caches_, caches_ + count, (num_ - count) * sizeof(void *));
  num_ -= count;
}

/*------------------------------------------------------------------*/
// inline functions of ThreadAllocator

inline
ThreadAllocator::~ThreadAllocator() {
  for (auto &bin : cache_bins_) {
    if (nullptr != bin) {
      arena_.base_alloc().Delete(bin);
    }
  }
}

inline void *
ThreadAllocator::ThreadAlloc(size_t size) {
  if (unlikely(0 == size)) {
    return nullptr;
  }

  if (size <= kMaxSmallClass) {
    size_t ind = sz_to_ind(size);
    if (g_stat) {
      cache_stats_[ind].num_alloc += 1;
    }
    return cache_bin(ind)->PopRegion();

  } else {
    size_t pind = sz_to_pind(size);
    size_t cs = sz_to_cs(size);
    if (g_stat) {
      large_stat_.alloc += cs;
    }
    return arena_.LargeAlloc(cs, pind);
  }
}

inline void
ThreadAllocator::ThreadDalloc(void *ptr) {
  if (likely(nullptr != ptr)) {
    Chunk *chunk = Static::chunk_rtree()->LookUp(ptr);
    if (chunk->is_slab()) {
      size_t ind = sz_to_ind(chunk->slab_region_size());
      cache_bin(ind)->PushRegion(ptr);
      if (g_stat) {
        cache_stats_[ind].num_free += 1;
      }

    } else {
      chunk->arena()->LargeDalloc(chunk);
      if (g_stat) {
        large_stat_.free -= chunk->size();
      }
    }
  }
}

inline void *
ThreadAllocator::ThreadRealloc(void *old, size_t size) {
  if (unlikely(nullptr == old)) {
    return ThreadAlloc(size);
  }
  if (unlikely(0 == size)) {
    ThreadDalloc(old);
    return nullptr;
  }
  void *addr = ThreadAlloc(size);
  if (unlikely(nullptr == addr)) {
    return nullptr;
  }

  Chunk *chunk = Static::chunk_rtree()->LookUp(old);
  if (chunk->is_slab()) {
    size_t old_size = chunk->slab_region_size();
    memmove(addr, old, std::min(old_size, size));
    size_t ind = sz_to_ind(old_size);
    cache_bin(ind)->PushRegion(old);

    if (g_stat) {
      cache_stats_[ind].num_free -= 1;
    }
  } else {
    size_t old_size = chunk->size();
    memmove(addr, old, std::min(old_size, size));
    chunk->arena()->LargeDalloc(chunk);

    if (g_stat) {
      large_stat_.free -= chunk->size();
    }
  }
  return addr;
}

inline const ThreadAllocator::CacheStatArray &
ThreadAllocator::cache_stats() {
  for (size_t i = 0; i < kNumSmallClasses; ++i) {
    if (nullptr != cache_bins_[i]) {
      cache_stats_[i].num_cache = cache_bins_[i]->num_cache();
    }
  }
  return cache_stats_;
}

} // end of namespace ffmalloc
