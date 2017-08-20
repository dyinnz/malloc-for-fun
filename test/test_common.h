//
// Created by 郭映中 on 2017/8/19.
//

#pragma once

#include <vector>

struct MemoryRecord {
  uint64_t *ptr;
  size_t mem_size;
  uint64_t magic;
};

struct OperationWindow {
  std::vector<MemoryRecord> records;
  std::function<void *(size_t)> malloc;
  std::function<void(void *)> free;
  size_t times;
  size_t left;
  size_t right;
};
