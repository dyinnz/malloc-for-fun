
#pragma once

namespace ffmalloc {

void *OSAllocMap(size_t size);

void OSDallocMap(void *addr, size_t size);

} // end of namespace ffmalloc
