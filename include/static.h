//
// Created by 郭映中 on 2017/8/18.
//

#pragma once

#include <atomic>
#include "basic.h"

namespace ffmalloc {

class Arena;
class ThreadAllocator;
class Chunk;
template<class T>
class RadixTree;
typedef RadixTree<Chunk> ChunkRTree;
class BaseAllocator;

class Static {
 private:
  static constexpr size_t kMaxCPUs = 128;

 public:
  Static();

  static Arena *next_arena();

  static ChunkRTree *chunk_rtree() {
    return chunk_rtree_;
  }

  static BaseAllocator *root_alloc() {
    return root_alloc_;
  }

  static ThreadAllocator *thread_alloc() {
    if (unlikely(nullptr == thread_alloc_)) {
      create_thread_allocator();
    }
    return thread_alloc_;
  }

 private:
  static void init();
  static void create_thread_allocator();

 private:
  static Static init_data_;
  static Arena *arenas_[kMaxCPUs];
  static std::atomic_ulong arena_index_;
  static ChunkRTree *chunk_rtree_;
  static BaseAllocator *root_alloc_;
  static thread_local ThreadAllocator *thread_alloc_;
};

} // end of namespace ffmalloc

