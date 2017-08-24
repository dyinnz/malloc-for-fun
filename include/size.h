
#pragma once

#include <iostream>
#include <cstdint>
#include <cassert>

#include "basic.h"
#include "size_classes.h"

namespace ffmalloc {

inline size_t ind_to_cs(size_t index) {
  assert(index < kNumSizeClasses);
  return details::g_size_classes[index];
}

namespace details {

inline size_t sz_to_ind_look_up(size_t size) {
  return g_index_lookup[(size - 1) / 8];
}

inline size_t sz_to_cs_look_up(size_t size) {
  return ind_to_cs(sz_to_ind_look_up(size));
}

inline size_t sz_to_cs_compute(size_t size) {
  const int msb = 63 - __builtin_clzl(2 * size - 1) - 1;
  const int lg_space = msb - 2;
  // ceiling to space
  const size_t class_size = (((size - 1) >> lg_space) + 1) << lg_space;
  return class_size;
}

inline size_t sz_to_ind_compute(size_t size) {
  // some annoying magic code
  const int msb = 63 - __builtin_clzl(2 * size - 1) - 1;
  const int lg_space = msb - 2;
  const int group_offset = ((size - 1) >> lg_space) - 4;
  const int class_index = msb * 4 - 19 + group_offset;
  return class_index;
}

} // end of namespace details

inline size_t sz_to_cs(size_t size) {
  assert(0 != size);
  if (size <= details::kMaxLookupSize) {
    return details::sz_to_cs_look_up(size);
  } else {
    return details::sz_to_cs_compute(size);
  }
}

inline size_t sz_to_ind(size_t size) {
  assert(0 != size);
  if (size <= details::kMaxLookupSize) {
    return details::sz_to_ind_look_up(size);
  } else {
    return details::sz_to_ind_compute(size);
  }
}

inline size_t sz_to_pind(size_t size) {
  assert(size >= kPage);

  if (size >= kMinAllocMmap) {
    return kNumGePageClasses + 1;

  } else {
    const int msb = 63 - __builtin_clzl(2 * size - 1) - 1;
    const int lg_space = msb - 2;
    const int group_offset = ((size - 1) >> lg_space) - 4;
    const int class_index = msb * 4 - 19 + group_offset;
    const int page_index =
        class_index - (kNumSizeClasses - kNumGePageClasses);
    return page_index;
  }
}

inline size_t ind_to_pind(size_t ind) {
  return ind - (kNumSizeClasses - kNumGePageClasses);
}

inline size_t pind_to_ind(size_t pind) {
  return pind + (kNumSizeClasses - kNumGePageClasses);
}

inline size_t pind_to_cs(size_t pind) {
  return ind_to_cs(pind_to_ind(pind));
}

inline size_t lookup_slab_size(size_t sind) {
  return details::g_slab_sizes[sind];
}

} // end of namespace ffmalloc
