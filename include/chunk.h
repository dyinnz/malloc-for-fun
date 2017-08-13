
#pragma once

#include "basic.h"
#include "size.h"
#include "base_alloc.h"
#include "list.h"
#include "radix_tree.h"

class Chunk {
 public:
  Chunk(void *address, size_t size)
      : prev_(nullptr), next_(nullptr),
        address_(address), size_(size), is_slab_(false) {
  }

  Chunk() : Chunk(nullptr, 0) {
  }

  void *address() const {
    return address_;
  }

  size_t size() const {
    return size_;
  }

  bool is_slab() const {
    return is_slab_;
  }

  void set_slab(bool is_slab) {
    is_slab_ = is_slab;
  }

  Chunk *prev() const {
    return prev_;
  }

  Chunk *next() const {
    return next_;
  }

  void set_prev(Chunk *node) {
    prev_ = node;
  }

  void set_next(Chunk *node) {
    next_ = node;
  }

 private:
  Chunk *prev_;
  Chunk *next_;
  void *address_;
  size_t size_;
  bool is_slab_;
};

typedef List<Chunk> ChunkList;

class ChunkManager {
 private:
  // TODO:
  static constexpr int kMaxBinSize = 2;

 public:
  explicit ChunkManager(BaseAllocator &base_alloc)
      : base_alloc_(base_alloc) {
  }

  Chunk *AllocChunk(size_t cs, size_t pind, bool is_slab);

  void DallocChunk(Chunk *chunk);

 private:

 private:
  BaseAllocator &base_alloc_;
  ChunkList avail_bins[kNumGePageClasses];
};

extern RadixTree<Chunk> g_radix_tree;
