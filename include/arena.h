
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
    assert(sz_to_ind(cs) == sind);

    if (nonfull_slab_[sind].empty()) {
      const size_t slab_size = g_slab_sizes[sind];
      const size_t pind = sz_to_pind(slab_size);
      Chunk *new_slab = AllocChunkWrapper(slab_size, pind, kSlabAttr);
      new_slab->set_slab_region_size(cs);
      if (nullptr == new_slab) {
        fprintf(stderr, "%s(): alloc slab failed: cs %zu, sind %zu\n",
                __func__, cs, sind);
        return nullptr;
      }
      nonfull_slab_[sind].push(new_slab);
    }

    Chunk *slab = nonfull_slab_[sind].first();
    Chunk::SlabBitmap &bitmap = slab->slab_bitmap();
    int index = bitmap.ffs_and_reset();

    assert(-1 != index);
    if (!bitmap.any()) {
      nonfull_slab_[sind].erase(slab);
      full_slab_[sind].push(slab);
    }

    void *region = static_cast<char *>(slab->address()) + index * cs;
    return region;
  }

  void SmallDalloc(void *region, Chunk *slab) {
    size_t cs = slab->slab_region_size();
    size_t sind = sz_to_ind(cs);
    size_t index = (static_cast<char*>(region)
        - static_cast<char*>(slab->address())) / cs;
    Chunk::SlabBitmap &bitmap = slab->slab_bitmap();
    bool is_full = bitmap.any();

    bitmap.set(index);
    if (is_full) {
      full_slab_[sind].erase(slab);
      nonfull_slab_[sind].push(slab);
    }

    if (bitmap.all()) {
      nonfull_slab_[sind].erase(slab);
      DallocChunkWrapper(slab);
    }
  }

  void *LargeAlloc(size_t cs, size_t pind) {
    assert(cs >= kMinLargeClass);
    Chunk * chunk = AllocChunkWrapper(cs, pind, kNonSlabAttr);
    if (nullptr == chunk) {
      return nullptr;
    } else {
      return chunk->address();
    }
  }

  void LargeDalloc(Chunk *chunk) {
    DallocChunkWrapper(chunk);
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

class ArenaAllocator {
 public:
  void *alloc(size_t size) {
    size_t cs = sz_to_cs(size);
    if (size <= kMaxSmallClass) {
      size_t sind = sz_to_ind(size);
      return arena_.SmallAlloc(cs, sind);

    } else {
      size_t pind = sz_to_pind(cs);
      return arena_.LargeAlloc(cs, pind);
    }
  }

  void free(void *ptr) {
    Chunk *chunk = g_radix_tree.LookUp(ptr);
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

