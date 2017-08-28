//
// Created by Guo Yingzhong on 2017/8/14.
//

#pragma once

#include <cstring>
#include <iostream>
#include <bitset>

namespace ffmalloc {

template<size_t N>
class Bitmap {
 private:
  static_assert(N > 0, "N of Bitmap must greator 0.");
  static constexpr size_t kLongBits = sizeof(long) * 8;
  static constexpr size_t kArraySize = (N - 1) / kLongBits + 1;

 public:
  Bitmap() = default;
  Bitmap(size_t num) {
    init(num);
  }

  void init(size_t num) {
    num_ = num;
    memset(bits_, 0xFF, sizeof(long) * kArraySize);
    if (0 != num_ % kLongBits) {
      bits_[last_index()] &= (1UL << (num_ % kLongBits)) - 1;
    }
  }

  void set(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits_[index] |= 1UL << offset;
  }

  void reset(size_t pos) {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    bits_[index] &= ~(1UL << offset);
  }

  bool test(size_t pos) const {
    const size_t index = pos / kLongBits;
    const size_t offset = pos % kLongBits;
    return (bits_[index] >> offset) & 1;
  }

  bool any() const {
    for (size_t i = 0; i <= last_index(); ++i) {
      if (bits_[i]) {
        return true;
      }
    }
    return false;
  }

  bool all() const {
    for (size_t i = 0; i < last_index(); ++i) {
      if (~bits_[i]) {
        return false;
      }
    }

    if (0 != num_ % kLongBits) {
      return bits_[last_index()] == (1UL << (num_ % kLongBits)) - 1;
    } else {
      return 0 == ~bits_[last_index()];
    }
  }

  int ffs_and_reset() {
    for (size_t i = 0; i <= last_index(); ++i) {
      if (bits_[i]) {
        int ret = ::ffsl(bits_[i]) - 1;
        bits_[i] &= ~(1UL << ret);
        return ret + i * kLongBits;
      }
    }
    return -1;
  }

  size_t last_index() const {
    return (num_ - 1) / kLongBits;
  }

  size_t last_mask() const {
    return (1UL << (num_ % kLongBits)) - 1;
  }

  // private:
  unsigned long bits_[kArraySize];
  size_t num_;
};

} // end of namespace ffmalloc
