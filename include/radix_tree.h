
#pragma once

#include <iostream>
#include <cstring>
#include <cassert>
#include "static.h"
#include "basic.h"

namespace ffmalloc {

template<class T>
class RadixTree {
 private:
  static constexpr int kLayerLogSize = 12;
  static constexpr int kLayerSize = 1 << 12;
  static constexpr int kLayerMask = (1 << 12) - 1;

  struct RadixNode {
    RadixNode() {
      memset(children, 0, sizeof(RadixNode *) * kLayerSize);
    }
    union {
      RadixNode *children[kLayerSize];
      T *elements[kLayerSize];
    };
  };

 public:
  void Insert(void *addr, T *element) {
    // std::cout << __func__ << "(): addr: " << addr << " elem: " << element << std::endl;
    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);

    const uintptr_t key3 = (key >> (kLayerLogSize * 3)) & kLayerMask;
    RadixNode **node = &root_.children[key3];

    if (unlikely(nullptr == *node)) {
      RadixNode *temp_node = Static::root_alloc()->NewPermanent<RadixNode>();
      if (!atomic_cas_simple(node, temp_node)) {
        Static::root_alloc()->ReturnPermanentMemory(temp_node, sizeof(RadixNode));
      }
    }
    const uintptr_t key2 = (key >> (kLayerLogSize * 2)) & kLayerMask;
    node = &(*node)->children[key2];

    if (unlikely(nullptr == *node)) {
      RadixNode *temp_node = Static::root_alloc()->NewPermanent<RadixNode>();
      if (!atomic_cas_simple(node, temp_node)) {
        Static::root_alloc()->ReturnPermanentMemory(temp_node, sizeof(RadixNode));
      }
    }
    const uintptr_t key1 = (key >> (kLayerLogSize * 1)) & kLayerMask;
    (*node)->elements[key1] = element;
  }

  void Delete(void *addr) {
    // std::cout << __func__ << "(): " << addr << std::endl;
    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);

    const uintptr_t key3 = (key >> (kLayerLogSize * 3)) & kLayerMask;
    RadixNode *node = root_.children[key3];
    assert(nullptr != node);

    const uintptr_t key2 = (key >> (kLayerLogSize * 2)) & kLayerMask;
    node = node->children[key2];
    assert(nullptr != node);

    const uintptr_t key1 = (key >> (kLayerLogSize * 1)) & kLayerMask;
    assert(nullptr != node->elements[key1]);

    node->elements[key1] = nullptr;
  }

  T *LookUp(void *addr) {
    //std::cout << __func__ << "(): " << addr << std::endl;

    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);

    const uintptr_t key3 = (key >> (kLayerLogSize * 3)) & kLayerMask;
    RadixNode *node = root_.children[key3];
    if (nullptr == node) {
      return nullptr;
    }

    const uintptr_t key2 = (key >> (kLayerLogSize * 2)) & kLayerMask;
    node = node->children[key2];
    if (nullptr == node) {
      return nullptr;
    }

    const uintptr_t key1 = (key >> (kLayerLogSize * 1)) & kLayerMask;

    // std::cout << __func__ << "(): " << addr << " elem: " << node->elements[key1] << std::endl;
    return node->elements[key1];
  }

 private:
  RadixNode root_;
};

} // end of namespace ffmalloc
