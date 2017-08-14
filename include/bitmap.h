//
// Created by Guo Yingzhong on 2017/8/14.
//

#pragma once

#include <cstring>

template <size_t N>
class Bitmap {
 private:
  static_assert(N > 0, "N of Bitmap must greator 0.");
  static constexpr size_t kLongBits = sizeof(long) * 8;
  static constexpr size_t kArraySize = (N - 1) / kLongBits + 1;

 public:
  Bitmap() {
    memset(bits, 0, sizeof(long) * kArraySize);
  }

  void set(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits[index] |= 1 << offset;
  }

  void reset(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits[index] &= ~(1 << offset);
  }

  bool test(size_t pos) const {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    return (bits[index] >> offset) & 1;
  }

  int ffs() {
    for (int i = 0; i < kArraySize; ++i) {
      if (bits[i]) {
        return ::ffsl(bits[i]) + i * kLongBits - 1;
      }
    }
    return -1;
  }

 // private:
  long bits[kArraySize];
};
