
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <sys/mman.h>

#include "basic.h"
#include "simplelogger.h"

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
    func_error(logger, "mmap failed: {}, size: {}", strerror(errno), size);
    return nullptr;
  }

  return ret;
}

void
OSDallocMap(void *addr, size_t size) {
  // assert(size >= kMinAllocMmap);
  assert(nullptr != addr);

  if (-1 == munmap(addr, size)) {
    func_error(logger, "munmap failed: {} size: {}", strerror(errno), size);
  }
}

} // end of namespace ffmalloc
