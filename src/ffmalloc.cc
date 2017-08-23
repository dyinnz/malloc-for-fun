//
// Created by 郭映中 on 2017/8/23.
//

#include "ffmalloc.h"
#include "static.h"
#include "thread_alloc.h"

extern "C" {

CACHELINE_ALIGN
void *ff_malloc(unsigned long size) noexcept {
  return ffmalloc::Static::thread_alloc()->ThreadAlloc(size);
}

CACHELINE_ALIGN
void ff_free(void *ptr) noexcept {
  ffmalloc::Static::thread_alloc()->ThreadDalloc(ptr);
}

CACHELINE_ALIGN
void *ff_calloc(unsigned long count, unsigned long size) noexcept {
  unsigned long total_size = count * size;
  void *ptr = ffmalloc::Static::thread_alloc()->ThreadAlloc(total_size);
  if (likely(nullptr != ptr)) {
    memset(ptr, 0, total_size);
  }
  return ptr;
}

CACHELINE_ALIGN
void *ff_realloc(void *ptr, unsigned long size) noexcept {
  return ffmalloc::Static::thread_alloc()->ThreadRealloc(ptr, size);
}

/*
__attribute__ ((constructor))
static void ff_init(){
}
*/

}


