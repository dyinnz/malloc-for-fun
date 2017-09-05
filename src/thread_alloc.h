
#pragma once

#include <array>
#include <algorithm>
#include <thread>
#include <cmath>
#include "size_classes.h"
#include "arena.h"
#include "simplelogger.h"

namespace ffmalloc {

/*------------------------------------------------------------------*/

constexpr static size_t kInitFillCount = 8;

class CacheBin {
 private:
  constexpr static size_t kMaxCaches = 200;

 public:
  CacheBin(Arena &arena, size_t capacity, size_t ind)
      : arena_(arena), capacity_(capacity), ind_(ind), cs_(ind_to_cs(ind)) {
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

  void GarbageCollect();

 private:
  size_t FillCaches(size_t count);

  void FlushCaches(size_t count);

 private:
  void *caches_[kMaxCaches];
  Arena &arena_;
  const size_t capacity_;
  const size_t ind_;
  const size_t cs_;
  size_t num_ {0};
  size_t fill_count_ {kInitFillCount};
  int low_mark_ {0};
} CACHELINE_ALIGN;

/*------------------------------------------------------------------*/

class ThreadAllocator {
 public:
  static constexpr size_t kMaxEventTick = 4096;

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

 public:
  ThreadAllocator(Arena &arena) : arena_(arena), thread_id_(std::this_thread::get_id()) {
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

  std::thread::id thread_id() const {
    return thread_id_;
  }

  Arena &arena() {
    return arena_;
  }

 private:
  CacheBin *cache_bin(size_t ind) {
    if (unlikely(nullptr == cache_bins_[ind])) {
      // TODO: nullptr check
      cache_bins_[ind] = arena_.base_alloc().New<CacheBin>(arena_, 200, ind);
    }
    return cache_bins_[ind];
  }

  void Event();
  void TriggerGC();

 private:
  CacheBin *cache_bins_[kNumSmallClasses];
  CacheStatArray cache_stats_;
  LargeStat large_stat_;
  Arena &arena_;
  const std::thread::id thread_id_;
  size_t event_tick_{0};
  size_t gc_index_{0};
} CACHELINE_ALIGN;

/*------------------------------------------------------------------*/
// inline functions of CacheBin

inline void *
CacheBin::PopRegion() {
  if (unlikely(0 == num_)) {
    if (unlikely(0 == FillCaches(fill_count_))) {
      func_error(logger, "fill caches failed");
      return nullptr;
    }

    // user intends to use more memory
    fill_count_ = std::min(fill_count_ * 2, capacity_ * 3 / 4);
  }
  num_ -= 1;
  if (unlikely(static_cast<int>(num_) < low_mark_)) {
    low_mark_ = static_cast<int>(num_);
  }
  return caches_[num_];
}

inline void
CacheBin::PushRegion(void *ptr) {
  if (unlikely(capacity_ == num_)) {
    // user has freed too much memory and the bin is full now.
    // we always flush 3/4 caches under this circumstance
    FlushCaches(capacity_ * 3 /4);

    // user intends to use less memory
    fill_count_ = std::max(fill_count_ / 2, kInitFillCount);
  }
  caches_[num_] = ptr;
  num_ += 1;
}

inline size_t
CacheBin::FillCaches(size_t count) {
  assert(num_ + count < capacity_);
  for (size_t i = 0; i < count; ++i) {
    void *ptr = arena_.SmallAlloc(cs_, ind_);
    if (unlikely(nullptr == ptr)) {
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

  // reset low mark to current number of caches
  low_mark_ = static_cast<int>(num_);
}

inline void
CacheBin::GarbageCollect() {
  FlushCaches(static_cast<size_t>(low_mark_ * 3 + 3) / 4);
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
    void *ptr = cache_bin(ind)->PopRegion();
    Event();
    return ptr;

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
      Event();
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

inline void
ThreadAllocator::Event() {
  event_tick_ += 1;
  if (unlikely(event_tick_ > 4096)) {
    event_tick_ = 0;
    TriggerGC();
  }
}

inline void
ThreadAllocator::TriggerGC() {
  gc_index_ += 1;
  while (nullptr == cache_bins_[gc_index_]) {
    gc_index_ += 1;
    if (gc_index_ >= kNumSmallClasses) {
      gc_index_ = 0;
    }
  }
  func_debug(logger, "bin[{}] gc", gc_index_);
  cache_bins_[gc_index_]->GarbageCollect();
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
