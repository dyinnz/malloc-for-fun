//
// Created by 郭映中 on 2017/8/19.
//

#include <random>

#include "gtest/gtest.h"
#include "size.h"
#include "arena.h"
#include "static.h"
#include "thread_alloc.h"
#include "test_common.h"

using namespace ffmalloc;

void RunWindowTest(OperationWindow window) {
  std::default_random_engine gen;
  std::uniform_int_distribution<size_t> dis(window.left / 8, window.right / 8);

  std::mt19937_64 magic;

  for (size_t t = 0; t < window.times; ++t) {
    // malloc
    for (size_t i = 0; i < window.records.size(); ++i) {
      size_t size = dis(gen) * 8;
      window.records[i].mem_size = size;
      window.records[i].magic = magic();
      window.records[i].ptr = static_cast<uint64_t *>(window.malloc(size));

      for (size_t pos = 0; pos < size / 8; ++pos) {
        window.records[i].ptr[pos] = window.records[i].magic;
      }
    }
    // free
    for (size_t i = 0; i < window.records.size(); ++i) {
      for (size_t pos = 0; pos < window.records[i].mem_size / 8; ++pos) {
        uint64_t store = window.records[i].magic;
        uint64_t curr = window.records[i].ptr[pos];
        ASSERT_EQ(store, curr);
      }
      window.free(window.records[i].ptr);
    }
  }
}

OperationWindow GenSmallClassWindow() {
  OperationWindow window;
  window.records.resize(1000);
  window.left = 8;
  window.right = 13 * 1024;
  window.times = 5;
  return window;
}

OperationWindow GenLargeClassWindow() {
  OperationWindow window;
  window.records.resize(200);
  window.left = 15 * 1024;
  window.right = 128 * 1024;
  window.times = 5;
  return window;
}

OperationWindow GenMixClassWindow() {
  OperationWindow window;
  window.records.resize(500);
  window.left = 8 * 1024;
  window.right = 32 * 1024;
  window.times = 5;
  return window;
}

TEST(ArenaAlloc, ArenaAllocSmall) {
  auto window = GenSmallClassWindow();
  ArenaAllocator allocator;
  window.malloc = [&](size_t size) {
    return allocator.ArenaAlloc(size);
  };
  window.free = [&](void *ptr) {
    allocator.ArenaDalloc(ptr);
  };
  RunWindowTest(window);
}

TEST(ArenaAlloc, ArenaAllocLarge) {
  auto window = GenLargeClassWindow();
  ArenaAllocator allocator;
  window.malloc = [&](size_t size) {
    return allocator.ArenaAlloc(size);
  };
  window.free = [&](void *ptr) {
    allocator.ArenaDalloc(ptr);
  };
  RunWindowTest(window);
}

TEST(ArenaAlloc, ArenaAllocMix) {
  auto window = GenMixClassWindow();
  ArenaAllocator allocator;
  window.malloc = [&](size_t size) {
    return allocator.ArenaAlloc(size);
  };
  window.free = [&](void *ptr) {
    allocator.ArenaDalloc(ptr);
  };
  RunWindowTest(window);
}

TEST(ThreadAlloc, ThreadAllocSmall) {
  auto window = GenSmallClassWindow();
  window.malloc = [](size_t size) {
    return Static::thread_alloc()->ThreadAlloc(size);
  };
  window.free = [](void *ptr) {
    Static::thread_alloc()->ThreadDalloc(ptr);
  };
  RunWindowTest(window);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
