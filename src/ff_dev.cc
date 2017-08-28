
#include <iostream>

#include "basic.h"
#include "pages.h"
#include "chunk.h"
#include "simplelogger.h"
#include "ffhelper.h"
#include "ffmalloc.h"

int main() {

  using namespace std;
  using namespace ffmalloc;

  logger.log("chunk size: {}", sizeof(Chunk));

  void *test_ptr = ff_malloc(128);

  printf("\n");
  AllocatorStatReport(*Static::thread_alloc());
  printf("\n");
  for (size_t i = 0; i < Static::num_arena(); ++i) {
    if (nullptr != Static::get_arena(i)) {
      AllocatorStatReport(*Static::get_arena(i));
      printf("\n");
      AllocatorStatReport(Static::get_arena(i)->chunk_manager());
      printf("\n");
    }
  }

  ff_free(test_ptr);

  ff_free(ff_malloc(32));

  for (size_t i = 0; i < 100; ++i) {
    ff_free(ff_malloc(128));
  }

  printf("\n");
  AllocatorStatReport(*Static::thread_alloc());
  printf("\n");
  for (size_t i = 0; i < Static::num_arena(); ++i) {
    if (nullptr != Static::get_arena(i)) {
      AllocatorStatReport(*Static::get_arena(i));
      printf("\n");
      AllocatorStatReport(Static::get_arena(i)->chunk_manager());
      printf("\n");
    }
  }

  for (size_t i = 0; i < 100; ++i) {
    ff_free(ff_malloc(32));
  }

  printf("\n");
  AllocatorStatReport(*Static::thread_alloc());
  printf("\n");
  for (size_t i = 0; i < Static::num_arena(); ++i) {
    if (nullptr != Static::get_arena(i)) {
      AllocatorStatReport(*Static::get_arena(i));
      printf("\n");
      AllocatorStatReport(Static::get_arena(i)->chunk_manager());
      printf("\n");
    }
  }

  return 0;
}
