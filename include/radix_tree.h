
#pragma once

#include <iostream>
#include <cstring>
#include "static.h"

namespace ffmalloc {

template<class T>
class RadixTree {
 private:
  static constexpr int kRadixLayer = 3;
  static constexpr int kRadixLayerLogSize = 12;
  static constexpr int kRadixLayerSize = 1 << 12;
  static constexpr int kRadixLayerMask = (1 << 12) - 1;

  struct RadixNode {
    RadixNode() {
      memset(children, 0, sizeof(RadixNode *) * kRadixLayerSize);
    }
    union {
      RadixNode *children[kRadixLayerSize];
      T *elements[kRadixLayerSize];
    };
  };

 public:
  void Insert(void *addr, T *element) {
    // std::cout << __func__ << "(): addr: " << addr << " elem: " << element << std::endl;
    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);
    RadixNode **node = &root_;

    for (int i = kRadixLayer * kRadixLayerLogSize;
         i > 0; i -= kRadixLayerLogSize) {
      if (nullptr == *node) {
        *node = Static::root_alloc()->New<RadixNode>();
      }

      const uintptr_t sub_key = (key >> i) & kRadixLayerMask;
      // std::cout << std::hex << sub_key << " " << *node << " " << (*node)->children[sub_key] << std::endl;
      node = &(*node)->children[sub_key];
    }

    *reinterpret_cast<T **>(node) = element;
  }

  void Delete(void *addr) {
    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);
    RadixNode *node = root_;
    assert(nullptr != node);

    const uintptr_t key3 = (key >> (kRadixLayerLogSize * 3)) & kRadixLayerMask;
    node = node->children[key3];
    assert(nullptr != node);

    const uintptr_t key2 = (key >> (kRadixLayerLogSize * 2)) & kRadixLayerMask;
    node = node->children[key2];
    assert(nullptr != node);

    const uintptr_t key1 = (key >> (kRadixLayerLogSize * 1)) & kRadixLayerMask;
    assert(nullptr != node->elements[key1]);

    // std::cout << __func__ << "(): " << addr << " elem: " << node->elements[key1] << std::endl;

    node->elements[key1] = nullptr;
  }

  T *LookUp(void *addr) {
    const uintptr_t key = reinterpret_cast<uintptr_t>(addr);
    RadixNode *node = root_;
    assert(nullptr != node);

    const uintptr_t key3 = (key >> (kRadixLayerLogSize * 3)) & kRadixLayerMask;
    // std::cout << std::hex << key3 << " " << node << " " << node->children[key3] << std::endl;
    node = node->children[key3];
    assert(nullptr != node);

    const uintptr_t key2 = (key >> (kRadixLayerLogSize * 2)) & kRadixLayerMask;
    // std::cout << std::hex << key2 << " " << node << " " << node->children[key2] << std::endl;
    node = node->children[key2];
    assert(nullptr != node);

    const uintptr_t key1 = (key >> (kRadixLayerLogSize * 1)) & kRadixLayerMask;
    // std::cout << std::hex << key1 << " " << node << " " << node->children[key1] << std::endl;
    assert(nullptr != node->children[key1]);

    // std::cout << __func__ << "(): " << addr << " elem: " << node->elements[key1] << std::endl;
    return node->elements[key1];

  }

 private:
  RadixNode *root_{nullptr};
};

} // end of namespace ffmalloc
