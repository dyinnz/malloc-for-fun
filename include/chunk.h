
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
  enum class State : uint8_t { kActive, kDirty, kClean };

  Chunk(void *address, size_t size, Arena *arena, State state, size_t region_size)
      : address_(address),
        size_(size),
        arena_(arena),
        state_(state),
        slab_region_size_(static_cast<uint16_t>(region_size)),
        slab_bitmap_() {
    if (is_slab()) {
      slab_bitmap().init(size_ / slab_region_size_);
    }
  }

  Chunk() : Chunk(nullptr, 0, nullptr, State::kClean, 0) {
  }

  Arena *arena() {
    return arena_;
  }

  void *address() const {
    return address_;
  }

  void set_address(void *address) {
    address_ = address;
  }

  char *char_addr() const {
    return static_cast<char *>(address_);
  }

  size_t size() const {
    return size_;
  }

  void set_size(size_t size) {
    size_ = size;
  }

  bool is_slab() const {
    return 0 != slab_region_size();
  }

  size_t slab_region_size() const {
    return slab_region_size_;
  }

  void set_slab_region_size(size_t region_size) {
    slab_region_size_ = static_cast<uint16_t>(region_size);
    if (is_slab()) {
      slab_bitmap().init(size_ / slab_region_size_);
    }
  }

  State state() const {
    return state_;
  }

  void set_state(State state) {
    state_ = state;
  }

  SlabBitmap &slab_bitmap() {
    return slab_bitmap_;
  }

 private:
  void *address_{nullptr};
  size_t size_{0};
  Arena *arena_;
  State state_{State::kDirty};
  uint16_t slab_region_size_{0};
  SlabBitmap slab_bitmap_;
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

  Chunk *AllocChunk(size_t cs, size_t pind, size_t slab_region_size);

  void DallocChunk(Chunk *chunk);

 private:

 private:
  Arena &arena_;
  BaseAllocator &base_alloc_;
  ChunkList avail_bins[kNumGePageClasses];
};

} // end of namespace ffmalloc
