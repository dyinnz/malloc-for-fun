//
// Created by Guo Yingzhong on 2017/8/14.
//

#pragma once

#include <cstring>
#include <iostream>

template<size_t N>
class Bitmap {
 private:
  static_assert(N > 0, "N of Bitmap must greator 0.");
  static constexpr size_t kLongBits = sizeof(long) * 8;
  static constexpr size_t kArraySize = (N - 1) / kLongBits + 1;
  static constexpr size_t kLastMask = (1UL << (N % kLongBits)) - 1;

 public:
  Bitmap() {
    memset(bits, 0xFF, sizeof(long) * kArraySize);
    if (0 != N % kLongBits) {
      bits[kArraySize - 1] &= kLastMask;
    }
  }

  void set(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits[index] |= 1UL << offset;
  }

  void reset(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits[index] &= ~(1UL << offset);
  }

  bool test(size_t pos) const {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    return (bits[index] >> offset) & 1;
  }

  bool any() const {
    for (int i = 0; i < kArraySize; ++i) {
      if (bits[i]) {
        return true;
      }
    }
    return false;
  }

  bool all() const {
    for (int i = 0; i < kArraySize - 1; ++i) {
      if (~bits[i]) {
        return false;
      }
    }

    if (0 != N % kLongBits) {
      return bits[kArraySize-1] == kLastMask;
    } else {
      return ~bits[kArraySize-1];
    }
  }

  int ffs_and_reset() {
    for (int i = 0; i < kArraySize; ++i) {
      if (bits[i]) {
        int ret = ::ffsl(bits[i]) - 1;
        bits[i] &= ~(1UL << ret);
        return ret + i * kLongBits;
      }
    }
    return -1;
  }

  // private:
  long bits[kArraySize];
};
