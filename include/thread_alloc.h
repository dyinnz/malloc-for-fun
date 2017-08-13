
#pragma once

#include "size_classes.h"
#include "arena.h"

class CacheBin {
  public:
    void *PopRegion();

    void PushRegion(void *ptr);

  private:
    void FillCaches();

    void FlushCaches();

  private:
    Arena &arena_;
    const size_t capacity_;
    size_t num_;
    void *caches_;
};

class ThreadAllocator {
  public:
    void *ThreadAlloc(size_t size) {
      if (size <= kMaxSmallClass) {

      } else {
      }

      return nullptr;
    }

    void ThreadDalloc(void *ptr) {
      // look up radix tree
    }

  private:
    Arena &arena_;
    CacheBin *cache_bins_[kNumSmallClasses];
};
