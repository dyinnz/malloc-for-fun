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

}

#ifdef __APPLE__

namespace ffmalloc {

namespace details {

size_t mz_size(malloc_zone_t *zone, const void *ptr) {
  return Static::GetAllocatedSize(const_cast<void*>(ptr));
}

static malloc_zone_t* GetDefaultZone() {
  malloc_zone_t **zones = NULL;
  unsigned int num_zones = 0;

  /*
   * On OSX 10.12, malloc_default_zone returns a special zone that is not
   * present in the list of registered zones. That zone uses a "lite zone"
   * if one is present (apparently enabled when malloc stack logging is
   * enabled), or the first registered zone otherwise. In practice this
   * means unless malloc stack logging is enabled, the first registered
   * zone is the default.
   * So get the list of zones to get the first one, instead of relying on
   * malloc_default_zone.
   */
  if (KERN_SUCCESS != malloc_get_all_zones(0, NULL, (vm_address_t**) &zones,
                                           &num_zones)) {
    /* Reset the value in case the failure happened after it was set. */
    num_zones = 0;
  }

  if (num_zones)
    return zones[0];

  return malloc_default_zone();
}

void ReplaceSystemAlloc() {
  static malloc_introspection_t ffmalloc_intro;
  memset(&ffmalloc_intro, 0, sizeof(ffmalloc_intro));
  ffmalloc_intro.enumerator = mi_enumerator;
  ffmalloc_intro.good_size = mi_good_size;
  ffmalloc_intro.check = mi_check;
  ffmalloc_intro.print = mi_print;
  ffmalloc_intro.log = mi_log;
  ffmalloc_intro.force_lock = mi_force_lock;
  ffmalloc_intro.force_unlock = mi_force_unlock;

  static malloc_zone_t ffmalloc_zone;
  memset(&ffmalloc_zone, 0, sizeof(ffmalloc_zone));
  ffmalloc_zone.version = 4;
  ffmalloc_zone.zone_name = "ffmalloc";
  ffmalloc_zone.size = mz_size;
  ffmalloc_zone.malloc = mz_malloc;
  ffmalloc_zone.calloc = mz_calloc;
  ffmalloc_zone.valloc = mz_valloc;
  ffmalloc_zone.free = mz_free;
  ffmalloc_zone.realloc = mz_realloc;
  ffmalloc_zone.destroy = mz_destroy;
  ffmalloc_zone.batch_malloc = nullptr;
  ffmalloc_zone.batch_free = nullptr;
  ffmalloc_zone.introspect = &ffmalloc_intro;

  malloc_zone_register(&ffmalloc_zone);

  malloc_zone_t *default_zone = GetDefaultZone();
  std::cout << default_zone << " " << default_zone->zone_name << std::endl;
  malloc_zone_unregister(default_zone);
  malloc_zone_register(default_zone);
}

} // end of namespace details

} // end of namespace ffmalloc

#endif // __APPLE__
