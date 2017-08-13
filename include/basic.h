
#pragma once

#include <cstdint>
#include <cstdlib>

constexpr size_t kCacheLine = 64;
constexpr size_t kPage = 4 * 1024; // 4K
constexpr size_t kMinAllocMmap = 2 * 1024 * 1024; // 2M
constexpr size_t kStandardExtent = kMinAllocMmap;

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

inline size_t standard_extent_ceil(size_t size) {
  return align_ceil_pow2(size, kStandardExtent);
}
