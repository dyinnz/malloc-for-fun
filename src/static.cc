//
// Created by 郭映中 on 2017/8/19.
//

#include <thread>
#include "static.h"
#include "chunk.h"
#include "arena.h"
#include "thread_alloc.h"
#include "ffmalloc.h"
#include "simplelogger.h"

static ffmalloc::ChunkRTree g_chunk_rtree;
static ffmalloc::BaseAllocator g_root_alloc;
simplelogger::Logger logger(simplelogger::kDebug);

namespace ffmalloc {

#if defined(FF_USE_TLS)
thread_local ThreadAllocator *Static::thread_alloc_{nullptr};
#else
pthread_key_t Static::pthread_tls_key_;
#endif

Arena *Static::arenas_[kMaxCPUs] = {nullptr};
std::atomic_ulong Static::arena_index_{0};
size_t Static::num_arenas_{1};
ChunkRTree *Static::chunk_rtree_{nullptr};
BaseAllocator *Static::root_alloc_{nullptr};
Static Static::init_data_;

Static::Static() {
#ifndef NDEBUG
  std::cout << "Static Constructor" << std::endl;
#endif
  Init();
  details::ReplaceSystemAlloc();
}

void Static::Init() {
  num_arenas_ = std::thread::hardware_concurrency();
  root_alloc_ = &g_root_alloc;
  chunk_rtree_ = &g_chunk_rtree;
  InitTLS();
}

ThreadAllocator *Static::create_thread_allocator() {
  // Ensure that all the static variables has been set correctly
  // We can call init() twice and the values are the same.
  // if (chunk_rtree_ == nullptr) {
  //   init();
  // }

  auto tmp_alloc = Static::root_alloc()->New<ThreadAllocator>(*next_arena());

#if defined(FF_USE_TLS)
  if (!atomic_cas_simple(&thread_alloc_, tmp_alloc)) {
    Static::root_alloc()->Delete(tmp_alloc);
  }
  return thread_alloc_;
#else
  if (0 != pthread_setspecific(pthread_tls_key_, tmp_alloc)) {
    func_error(logger, "pthread_setspecific failed");
    abort();
  }
  return tmp_alloc;
#endif
}

Arena *Static::next_arena() {
  unsigned long curr = arena_index_.fetch_add(1, std::memory_order_relaxed) % num_arenas_;
  if (nullptr == arenas_[curr]) {
    auto tmp_arena = Static::root_alloc()->New<Arena>(curr);
    if (!atomic_cas_simple(&arenas_[curr], tmp_arena)) {
      Static::root_alloc()->Delete(tmp_arena);
    }
  }
  return arenas_[curr];
}

size_t Static::GetAllocatedSize(void *ptr) {
  if (likely(nullptr != ptr)) {
    Chunk *chunk = Static::chunk_rtree()->LookUp(ptr);
    if (unlikely(0 == chunk)) {
      // we don't own the memory
      return 0;
    }
    if (chunk->is_slab()) {
      return chunk->slab_region_size();
    } else {
      return chunk->size();
    }
  } else {
    return 0;
  }
}

void Static::InitTLS() {
#ifndef FF_USE_TLS
  if (0 != pthread_key_create(&pthread_tls_key_, nullptr)) {
    func_error(logger, "pthread_key_create failed");
    abort();
  }
  func_log(logger, "init pthread key ok");
#endif
}

} // end of namespace ffmalloc

