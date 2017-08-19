
#pragma once

#include "basic.h"
#include "size.h"
#include "base_alloc.h"
#include "list.h"
#include "radix_tree.h"
#include "bitmap.h"

namespace ffmalloc {

class Arena;

class Chunk : public ListNode<Chunk> {
 public:
  typedef Bitmap<kMaxSlabRegions> SlabBitmap;

  Chunk(void *address, size_t size) :
      address_(address), size_(size) {
  }

  Chunk() : Chunk(nullptr, 0) {
  }

  Arena *arena() {
    return arena_;
  }

  void set_arena(Arena *arena) {
    arena_ = arena;
  }

  void *address() const {
    return address_;
  }

  void set_address(void *address) {
    address_ = address;
  }

  size_t size() const {
    return size_;
  }

  void set_size(size_t size) {
    size_ = size;
  }

  bool is_slab() const {
    return is_slab_;
  }

  void set_slab(bool is_slab) {
    is_slab_ = is_slab;
  }

  size_t slab_region_size() const {
    return slab_region_size_;
  }

  void set_slab_region_size(size_t region_size) {
    slab_region_size_ = region_size;
    slab_bitmap().init(size_ / slab_region_size_);
  }

  bool has_data() const {
    return address_ != nullptr;
  }

  void reset() {
    memset(this, 0, sizeof(*this));
  }

  SlabBitmap &slab_bitmap() {
    return slab_bitmap_;
  }

 private:
  void *address_{nullptr};
  size_t size_{0};
  Arena *arena_{nullptr};
  SlabBitmap slab_bitmap_;
  bool is_slab_{kNonSlabAttr};
  size_t slab_region_size_{0};
};

typedef List<Chunk> ChunkList;
// typedef RadixTree<Chunk> ChunkRTree;

class ChunkManager {
 private:
  // TODO:
  static constexpr int kMaxBinSize = 2;

 public:
  explicit ChunkManager(Arena &arena, BaseAllocator &base_alloc)
      : arena_(arena), base_alloc_(base_alloc) {
  }

  Chunk *AllocChunk(size_t cs, size_t pind, bool is_slab);

  void DallocChunk(Chunk *chunk);

 private:

 private:
  Arena &arena_;
  BaseAllocator &base_alloc_;
  ChunkList avail_bins[kNumGePageClasses];
};

} // end of namespace ffmalloc
