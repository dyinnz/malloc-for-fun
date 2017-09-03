
#pragma once

namespace ffmalloc {

void *OSAllocMap(void *addr, size_t size);

void OSDallocMap(void *addr, size_t size);

} // end of namespace ffmalloc
