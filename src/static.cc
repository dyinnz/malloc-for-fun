//
// Created by 郭映中 on 2017/8/19.
//

#include "static.h"
#include "chunk.h"

namespace ffmalloc {

Static Static::init_data_;
ChunkRTree *Static::chunk_rtree_ = nullptr;

Static::Static() {
  init();
}

void Static::init() {
  static ChunkRTree s_chunk_rtree;

  chunk_rtree_ = &s_chunk_rtree;
}

} // end of namespace ffmalloc

