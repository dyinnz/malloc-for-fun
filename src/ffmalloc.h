//
// Created by 郭映中 on 2017/8/23.
//

#pragma once

#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

extern "C" {

void *ff_malloc(unsigned long size) noexcept;
void ff_free(void *ptr) noexcept;
void *ff_calloc(unsigned long count, unsigned long size) noexcept;
void *ff_realloc(void *ptr, unsigned long size) noexcept;

}

namespace ffmalloc {

namespace details {

#ifdef __APPLE__

size_t mz_size(malloc_zone_t *zone, const void *ptr);

inline void *mz_malloc(malloc_zone_t *, size_t size) {
  return ff_malloc(size);
}

inline void *mz_calloc(malloc_zone_t *, size_t num_items, size_t size) {
  return ff_calloc(num_items, size);
}

inline void *mz_valloc(malloc_zone_t *, size_t) {
  return nullptr;
}

inline void mz_free(malloc_zone_t *, void *ptr) {
  return ff_free(ptr);
}

inline void *mz_realloc(malloc_zone_t *, void *ptr, size_t size) {
  return ff_realloc(ptr, size);
}

inline void *mz_memalign(malloc_zone_t *, size_t, size_t) {
  return nullptr;
}

inline void mz_destroy(malloc_zone_t *) {
  // A no-op -- we will not be destroyed!
}

// malloc_introspection callbacks.  I'm not clear on what all of these do.
inline kern_return_t mi_enumerator(task_t, void *, unsigned, vm_address_t, memory_reader_t, vm_range_recorder_t) {
  // Should enumerate all the pointers we have.  Seems like a lot of work.
  return KERN_FAILURE;
}

inline size_t mi_good_size(malloc_zone_t *, size_t size) {
  // I think it's always safe to return size, but we maybe could do better.
  return size;
}

inline boolean_t mi_check(malloc_zone_t *) {
  return 1;
}

inline void mi_print(malloc_zone_t *, boolean_t) {
}

inline void mi_log(malloc_zone_t *, void *) {
  // I don't think we support anything like this
}

inline void mi_force_lock(malloc_zone_t *) {
}

inline void mi_force_unlock(malloc_zone_t *) {
}

inline void mi_statistics(malloc_zone_t *, malloc_statistics_t *) {
}

inline boolean_t mi_zone_locked(malloc_zone_t *) {
  return 0;  // Hopefully unneeded by us!
}

#endif // __APPLE__

void ReplaceSystemAlloc();

} // end of namespace details

} // end of namespace ffmalloc

