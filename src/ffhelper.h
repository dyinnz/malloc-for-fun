//
// Created by 郭映中 on 2017/8/26.
//

#pragma once

#include <iostream>
#include "chunk.h"
#include "arena.h"
#include "thread_alloc.h"

namespace ffmalloc {

void AllocatorStatReport(ThreadAllocator &thread_alloc);
void AllocatorStatReport(Arena &arena);
void AllocatorStatReport(ChunkManager &chunk_mgr);

} // end of namespace ffmalloc
