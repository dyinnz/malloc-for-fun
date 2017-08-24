
#pragma once

#include <cstdint>
#include <cstdlib>

namespace ffmalloc {

constexpr size_t kCacheLine = 64;
constexpr size_t kPage = 4 * 1024; // 4K
constexpr size_t kMinAllocMmap = 2 * 1024 * 1024; // 2M
constexpr size_t kStandardChunk = kMinAllocMmap;
constexpr uint16_t kNonSlabAttr = 0;

/*
inline size_t alignment_ceil(size_t size, size_t alignment) {
  return (size + alignment - 1) / alignment * alignment;
}
*/

inline size_t align_ceil_pow2(size_t size, size_t alignment) {
  return (size + alignment - 1) & ((~alignment) + 1);
}

inline size_t cacheline_ceil(size_t size) {
  return align_ceil_pow2(size, kCacheLine);
}

inline size_t page_ceil(size_t size) {
  return align_ceil_pow2(size, kPage);
}

inline size_t standard_chunk_ceil(size_t size) {
  return align_ceil_pow2(size, kStandardChunk);
}

#define NAME(a) (#a)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define atomic_cas(ptr, expected, desired) \
  __atomic_compare_exchange_n(ptr, expected, desired, \
                              false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

template<class T>
bool atomic_cas_simple(T *ptr, T desired) {
  T null_data = nullptr;
  return atomic_cas(ptr, &null_data, desired);
}

#define CACHELINE_ALIGN __attribute__((aligned (64)))

} // end of namespace ffmalloc
