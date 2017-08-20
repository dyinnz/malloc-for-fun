//
// Created by 郭映中 on 2017/8/19.
//

#include <cstdio>
#include <thread>
#include <random>
#include <boost/thread/barrier.hpp>
#include <sstream>
#include "gtest/gtest.h"
#include "static.h"
#include "thread_alloc.h"
#include "test_common.h"

using namespace ffmalloc;
using std::thread;
using std::vector;
using boost::barrier;

TEST(Hello, Hello) {
  using std::cout;
  using std::endl;

  cout << "cores: " << std::thread::hardware_concurrency() << endl;
}

OperationWindow GenMTSmallWindow() {
  OperationWindow window;
  window.left = 32;
  window.right = 128;
  window.times = 10;

  size_t length = 10000;
  window.records.reserve(length);
  std::default_random_engine gen;
  std::uniform_int_distribution<size_t> dis(window.left / 8, window.right / 8);
  for (int i = 0; i < length; ++i) {
    window.records.push_back({nullptr, dis(gen) * 8, 0});
  }

  return window;
};

void ThreadTest(barrier *barr) {
  auto window = GenMTSmallWindow();

  std::ostringstream oss;
  oss << std::this_thread::get_id() << " complete generating window\n";
  std::cout << oss.str() << std::flush;

  barr->count_down_and_wait();

  oss.str("");
  oss << std::this_thread::get_id() << " begin test\n";
  std::cout << oss.str() << std::flush;

  for (size_t t = 0; t < window.times; ++t) {
    for (int i = 0; i < window.records.size(); ++i) {
      window.records[i].ptr = static_cast<uint64_t*>(
          Static::thread_alloc()->ThreadAlloc(window.records[i].mem_size)
      );
      // std::cout << "alloc: " << window.records[i].ptr << std::endl;
      ASSERT_NE(nullptr, window.records[i].ptr);
    }
    for (int i = 0; i < window.records.size(); ++i) {
      // std::cout << "free : " << window.records[i].ptr << std::endl;
      void *ptr = window.records[i].ptr;
      Static::thread_alloc()->ThreadDalloc(ptr);
    }
  }
}

TEST(MultiThread, C1T1) {
  unsigned int num_threads = 1;
  Static::set_num_arena(1);
  barrier barr(num_threads);

  std::vector<thread> threads;
  for (unsigned int i = 0; i < num_threads; ++i) {
    threads.push_back(thread(ThreadTest, &barr));
  }
  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
}

TEST(MultiThread, C8T8) {
  // TODO
  unsigned int num_threads = 8;
  Static::set_num_arena(8);
  barrier barr(num_threads);

  std::vector<thread> threads;
  for (unsigned int i = 0; i < num_threads; ++i) {
    threads.push_back(thread(ThreadTest, &barr));
  }
  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
