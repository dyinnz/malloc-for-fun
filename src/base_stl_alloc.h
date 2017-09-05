//
// Created by Guo Yingzhong on 2017/9/5.
//

#pragma once

#include <cstddef>
#include <utility>

#include "static.h"

namespace ffmalloc {

template<class T>
class BaseStlAllocator {
 public:
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef const T *const_pointer;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template<class Type>
  struct rebind {
    typedef BaseStlAllocator<Type> other;
  };

  pointer address(reference x) const noexcept {
    return &x;
  }

  const_pointer address(const_reference x) const noexcept {
    return &x;
  }

  size_type max_size() const noexcept {
    return 1LL << 48;
  }

  pointer allocate(size_type n, const void *hint = 0) {
    return static_cast<pointer>(Static::root_alloc()->Alloc(n * sizeof(value_type)));
  }

  void deallocate(pointer p, size_type) {
    Static::root_alloc()->Dalloc(p);
  }

  template<class U, class... Args>
  void construct(U *p, Args &&... args) {
    new(p) U(std::forward<Args>(args)...);
  }

  template<class U>
  void destroy(U *p) {
    p->~U();
  }
};

} // end of namespace ffmalloc
