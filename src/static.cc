//
// Created by 郭映中 on 2017/8/19.
//

#include <thread>
#include "static.h"
#include "chunk.h"
#include "arena.h"
#include "thread_alloc.h"

namespace ffmalloc {

Static Static::init_data_;
Arena *Static::arenas_[kMaxCPUs] = {nullptr};
std::atomic_ulong Static::arena_index_{0};
 size_t Static::num_arenas_ {1};
ChunkRTree *Static::chunk_rtree_{nullptr};
BaseAllocator *Static::root_alloc_{nullptr};
thread_local ThreadAllocator *Static::thread_alloc_ {nullptr};

Static::Static() {
  init();
}

void Static::init() {
  static ChunkRTree s_chunk_rtree;
  static BaseAllocator s_root_alloc;

  chunk_rtree_ = &s_chunk_rtree;
  root_alloc_ = &s_root_alloc;
  num_arenas_ = std::thread::hardware_concurrency();

  std::cout << "static init" << std::endl;
}

void Static::create_thread_allocator() {
  auto tmp_alloc = Static::root_alloc()->New<ThreadAllocator>(*next_arena());
  ThreadAllocator *null_data = nullptr;
  if (!__atomic_compare_exchange_n(&thread_alloc_, &null_data, tmp_alloc,
                                   false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    Static::root_alloc()->Delete(tmp_alloc);
  }
}

Arena *Static::next_arena() {
  unsigned long curr = arena_index_.fetch_add(1, std::memory_order_seq_cst) % num_arenas_;
  std::cout << __func__ << "(): " << curr << " " << num_arenas_ << std::endl;
  if (nullptr == arenas_[curr]) {
    auto tmp_arena = Static::root_alloc()->New<Arena>();
    Arena *null_data = nullptr;
    if (!__atomic_compare_exchange_n(&arenas_[curr], &null_data, tmp_arena,
                                     false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
      Static::root_alloc()->Delete(tmp_arena);
    }
  }
  return arenas_[curr];
}

} // end of namespace ffmalloc

