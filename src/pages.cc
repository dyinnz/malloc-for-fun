
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>

#include <sys/mman.h>

#include "basic.h"

namespace ffmalloc {

void *
OSAllocMap(void *addr, size_t size) {
  // TODO: tmp
  // assert(size >= kMinAllocMmap);
  assert(size % kPage == 0);

  void *ret = mmap(addr, size,
                   PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                   0, 0);

  if (MAP_FAILED == ret) {
    return nullptr;
  }

  return ret;
}

void
OSDallocMap(void *addr, size_t size) {
  // assert(size >= kMinAllocMmap);
  assert(nullptr != addr);
  assert(size % kPage == 0);

  if (-1 == munmap(addr, size)) {
    abort();
  }
}

void OSReleasePage(void *addr, size_t size) {
  assert(nullptr != addr);
  assert(size % kPage == 0);

  if (-1 == madvise(addr, size, MADV_FREE)) {
    abort();
  }
}

void OSCommitPage(void *, size_t) {
  // we do not do anything here
}


} // end of namespace ffmalloc
