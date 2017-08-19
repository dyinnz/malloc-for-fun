
#pragma once

#include <new>
#include <cstdlib>

namespace ffmalloc {

class BaseAllocator {
 public:
  void *Alloc(size_t size) {
    return malloc(size);
  }

  void Dalloc(void *p) {
    free(p);
  }

  template<class T, class... Args>
  T *
  New(Args &&... args) {
    void *mem = Alloc(sizeof(T));
    T *p = new(mem) T(args...);
    return p;
  }

  template<class T>
  void
  Delete(T *p) {
    p->~T();
    Dalloc(p);
  }
};

} // end of namespace ffmalloc
