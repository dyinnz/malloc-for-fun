
#include <cstdio>
#include <cassert>
#include <cstring>

#include <sys/mman.h>
#include <errno.h>

#include "basic.h"

namespace ffmalloc {

void *
OSAllocMap(size_t size) {
  // TODO: tmp
  // assert(size >= kMinAllocMmap);
  assert(size % kPage == 0);

  void *ret = mmap(nullptr, size,
                   PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                   0, 0);

  if (MAP_FAILED == ret) {
    fprintf(stderr, "mmap() failed: %s, size: %zu\n", strerror(errno), size);
    return nullptr;
  }

  return ret;
}

void
OSDallocMap(void *addr, size_t size) {
  // assert(size >= kMinAllocMmap);
  assert(nullptr != addr);

  if (-1 == munmap(addr, size)) {
    fprintf(stderr, "munmap() failed: %s size: %zu\n", strerror(errno), size);
  }
}

} // end of namespace ffmalloc
