//
// Created by 郭映中 on 2017/8/18.
//

#pragma once

#include <atomic>
#include <pthread.h>
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

  static void set_num_arena(size_t num) {
    num_arenas_ = num;
  }

  static size_t num_arena() {
    return num_arenas_;
  }

  static Arena *get_arena(size_t index) {
    return arenas_[index];
  }

  static ChunkRTree *chunk_rtree() {
    return chunk_rtree_;
  }

  static BaseAllocator *root_alloc() {
    return root_alloc_;
  }

  static ThreadAllocator *thread_alloc() {
#if defined(FF_USE_TLS)
    if (unlikely(nullptr == thread_alloc_)) {
      return create_thread_allocator();
    }
    return thread_alloc_;
#else
    auto *alloc = static_cast<ThreadAllocator*>(pthread_getspecific(pthread_tls_key_));
    if (unlikely(nullptr == alloc)) {
      return create_thread_allocator();
    }
    return alloc;
#endif
  }

  static size_t GetAllocatedSize(void *ptr);
  static void InitTLS();

 private:
  static void Init();
  static ThreadAllocator* create_thread_allocator();

 private:
  static Static init_data_;
  static Arena *arenas_[kMaxCPUs];
  static std::atomic_ulong arena_index_;
  static size_t num_arenas_;
  static ChunkRTree *chunk_rtree_;
  static BaseAllocator *root_alloc_;

#if defined(FF_USE_TLS)
  static thread_local ThreadAllocator *thread_alloc_ __attribute__ ((tls_model("initial-exec")));
#else
  static pthread_key_t pthread_tls_key_;
#endif
};

} // end of namespace ffmalloc

