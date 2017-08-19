
#include "arena.h"

namespace ffmalloc {

void *Arena::SmallAlloc(size_t cs, size_t sind) {
  assert(cs <= kMinLargeClass);
  assert(sz_to_ind(cs) == sind);

  if (nonempty_slab[sind].empty()) {
    const size_t slab_size = lookup_slab_size(sind);
    const size_t pind = sz_to_pind(slab_size);
    Chunk *new_slab = AllocChunkWrapper(slab_size, pind, kSlabAttr);
    new_slab->set_slab_region_size(cs);
    if (nullptr == new_slab) {
      fprintf(stderr, "%s(): ArenaAlloc slab failed: cs %zu, sind %zu\n",
              __func__, cs, sind);
      return nullptr;
    }
    nonempty_slab[sind].push(new_slab);
  }

  Chunk *slab = nonempty_slab[sind].first();
  Chunk::SlabBitmap &bitmap = slab->slab_bitmap();
  int index = bitmap.ffs_and_reset();
  assert(-1 != index);
  assert(index < lookup_slab_size(sind) / cs);

  if (!bitmap.any()) {
    nonempty_slab[sind].erase(slab);
    empty_slab[sind].push(slab);
  }

  void *region = static_cast<char *>(slab->address()) + index * cs;

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
      - static_cast<char *>(slab->address())) / cs;
  Chunk::SlabBitmap &bitmap = slab->slab_bitmap();
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
    empty_slab[sind].erase(slab);
    nonempty_slab[sind].push(slab);
  }

  if (bitmap.all()) {
    nonempty_slab[sind].erase(slab);
    DallocChunkWrapper(slab);
  }
}

} // end of namespace ffmalloc
