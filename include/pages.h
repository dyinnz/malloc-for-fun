
#pragma once

namespace ffmalloc {

void *ChunkAllocMap(size_t size);

void ChunkDallocMap(void *addr, size_t size);

} // end of namespace ffmalloc
