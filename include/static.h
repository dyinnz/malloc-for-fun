//
// Created by 郭映中 on 2017/8/18.
//

#pragma once

namespace ffmalloc {

class Arena;
class ThreadAllocator;
class Chunk;
template<class T>
class RadixTree;
typedef RadixTree<Chunk> ChunkRTree;
class BaseAllocator;

class Static {
 public:
  Static();

  static Arena *next_arena();
  static ThreadAllocator *thread_allocator();

  static ChunkRTree *chunk_rtree() {
    return chunk_rtree_;
  }

  static BaseAllocator &root_allocator();

 private:
  static void init();

 private:
  static Static init_data_;
  static ChunkRTree *chunk_rtree_;
};

} // end of namespace ffmalloc

