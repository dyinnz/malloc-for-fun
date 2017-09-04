
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <sys/mman.h>

#include "basic.h"
#include "simplelogger.h"

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
    func_error(logger, "mmap failed: {}, size: {}", strerror(errno), size);
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
    func_error(logger, "munmap failed: {} size: {}", strerror(errno), size);
  }
}

void OSReleasePage(void *addr, size_t size) {
  assert(nullptr != addr);
  assert(size % kPage == 0);

  if (-1 == madvise(addr, size, MADV_FREE)) {
    func_error(logger, "madvise set free failed: {} size: {}", strerror(errno), size);
    abort();
  }
}

void OSCommitPage(void *, size_t) {
  // we do not do anything here
}


} // end of namespace ffmalloc
