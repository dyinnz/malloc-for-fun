
#include "arena.h"

namespace ffmalloc {

void *Arena::SmallAlloc(size_t cs, size_t sind) {
  assert(cs <= kMinLargeClass);
  assert(sz_to_ind(cs) == sind);

  slab_mutex_[sind].lock();
  if (nonempty_slab_[sind].empty()) {
    const size_t slab_size = lookup_slab_size(sind);
    const size_t pind = sz_to_pind(slab_size);
    Chunk *new_slab = AllocChunkWrapper(slab_size, pind, kSlabAttr);
    new_slab->set_slab_region_size(cs);
    if (nullptr == new_slab) {
      fprintf(stderr, "%s(): ArenaAlloc slab failed: cs %zu, sind %zu\n",
              __func__, cs, sind);
      return nullptr;
    }
    nonempty_slab_[sind].push(new_slab);
  }

  Chunk *slab = nonempty_slab_[sind].first();
  Chunk::SlabBitmap &bitmap = slab->slab_bitmap();
  int index = bitmap.ffs_and_reset();
  assert(-1 != index);
  assert(index < lookup_slab_size(sind) / cs);

  if (!bitmap.any()) {
    nonempty_slab_[sind].erase(slab);
    empty_slab_[sind].push(slab);
  }
  slab_mutex_[sind].unlock();

  void *region = slab->char_addr() + index * cs;

  /*
  std::cout << __func__ << "():"
            << " cs " << cs
            << " sind " << sind
          << " region " << region
            << std::endl;
            */

  return region;
}

void Arena::SmallDalloc(void *region, Chunk *slab) {
  size_t cs = slab->slab_region_size();
  size_t sind = sz_to_ind(cs);
  size_t index = (static_cast<char *>(region)
      - slab->char_addr()) / cs;
  Chunk::SlabBitmap &bitmap = slab->slab_bitmap();

  slab_mutex_[sind].lock();
  bool is_empty = !bitmap.any();

  /*
  std::cout << __func__ << "():"
            << " cs " << cs
            << " sind " << sind
            << " ptr " << region
            << std::endl;
            */

  bitmap.set(index);
  if (is_empty) {
    empty_slab_[sind].erase(slab);
    nonempty_slab_[sind].push(slab);
  }

  if (bitmap.all()) {
    nonempty_slab_[sind].erase(slab);
    slab_mutex_[sind].unlock();

    DallocChunkWrapper(slab);
  } else {
    slab_mutex_[sind].unlock();
  }
}

} // end of namespace ffmalloc
