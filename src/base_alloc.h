
#pragma once

#include <iostream>
#include <cstdlib>
#include <new>
#include <mutex>
#include "size.h"
#include "pages.h"
#include "list.h"

namespace ffmalloc {

class RootAllocator {
 private:
  struct Header {
    unsigned long size;
  };
  struct RootNode : ListNode<RootNode> {
  };

  static_assert(sizeof(Header) == 8, "the size of Header of BaseAllocator Node should be 8");

 public:
  void init() {
    for (size_t i = 0; i < kNumSmallClasses; ++i) {
      free_lists_[i].init();
    }
  }

  void *Alloc(size_t size) {
    size_t size_with_meta = size + kCacheLine /* + sizeof(Header) */ ;
    size_t ind = sz_to_ind(size_with_meta);
    size_t cs = ind_to_cs(ind);

    if (cs <= kMaxSmallClass) {
      std::lock_guard<std::mutex> guard(mutex_);
      if (free_lists_[ind].empty()) {
        if (!AllocSmallNodes(ind, cs)) {
          return nullptr;
        }
      }
      RootNode *node = free_lists_[ind].pop();
      return node;

    } else {
      void *addr = OSAllocMap(nullptr, cs);
      if (nullptr == addr) {
        fprintf(stderr, "BaseAllocator::Alloc() map from os failed\n");
        return nullptr;
      }
      assert(reinterpret_cast<uintptr_t>(addr) % kPage == 0);
      char *aligned_addr = reinterpret_cast<char *>(
          cacheline_ceil(reinterpret_cast<size_t>(addr) + sizeof(Header))
      );
      *reinterpret_cast<unsigned long *>(aligned_addr - sizeof(Header)) = cs;
      return aligned_addr;
    }
  }

  void Dalloc(void *ptr) {
    size_t cs = *reinterpret_cast<unsigned long *>(
        static_cast<char *>(ptr) - sizeof(Header)
    );
    if (cs <= kMaxSmallClass) {
      std::lock_guard<std::mutex> guard(mutex_);
      free_lists_[sz_to_ind(cs)].push(reinterpret_cast<RootNode *>(ptr));
    } else {
      void *page_ptr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) & (~(kPage - 1)));
      OSDallocMap(page_ptr, cs);
    }
  }

  void *AllocPermanent(size_t size) {
    assert(size % kPage == 0);
    return OSAllocMap(nullptr, size);
  }

  void ReturnPermanentMemory(void *ptr, size_t size) {
    assert(size % kPage == 0);
    OSDallocMap(ptr, size);
  }

  template<class T, class... Args>
  T *New(Args &&... args) {
    void *mem = Alloc(sizeof(T));
    T *p = new(mem) T(std::forward<Args>(args)...);
    return p;
  }

  template<class T, class... Args>
  T *NewPermanent(Args &&... args) {
    void *mem = AllocPermanent(sizeof(T));
    T *p = new(mem) T(std::forward<Args>(args)...);
    return p;
  }

  template<class T>
  void Delete(T *p) {
    p->~T();
    Dalloc(p);
  }

 private:
  bool AllocSmallNodes(size_t sind, size_t cs) {
    size_t slab_size = lookup_slab_size(sind);
    char *addr = static_cast<char *>(OSAllocMap(nullptr, slab_size));
    if (nullptr == addr) {
      fprintf(stderr, "BaseAllocator::AllocSmallNodes() map from os failed\n");
      return false;
    }

    for (size_t offset = 0; offset < slab_size; offset += cs) {
      char *curr_addr = addr + offset;
      RootNode *list_node = reinterpret_cast<RootNode *>(
          cacheline_ceil(reinterpret_cast<size_t>(curr_addr + sizeof(Header)))
      );
      *reinterpret_cast<unsigned long *>(reinterpret_cast<char *>(list_node) - sizeof(Header)) = cs;
      free_lists_[sind].push(list_node);

    }
    return true;
  }

 private:
  List<RootNode> free_lists_[kNumSmallClasses];
  std::mutex mutex_;
} CACHELINE_ALIGN;

class BaseAllocator : public RootAllocator {
  public:
    BaseAllocator() {
      this->init();
    }
  private:
    using RootAllocator::init;
};


} // end of namespace ffmalloc
