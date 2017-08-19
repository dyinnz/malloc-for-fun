
#include <cassert>

#include "chunk.h"
#include "pages.h"
#include "static.h"

namespace ffmalloc {

static void RegisterRadix(Chunk *chunk) {
  char *head = static_cast<char *>(chunk->address());
  char *tail = head + chunk->size() - kPage;
  // first page
  Static::chunk_rtree()->Insert(head, chunk);

  // last page
  if (chunk->size() > kPage) {
    Static::chunk_rtree()->Insert(tail, chunk);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      Static::chunk_rtree()->Insert(curr, chunk);
    }
  }
}

static void DeregisterRadix(Chunk *chunk) {
  char *head = static_cast<char *>(chunk->address());
  char *tail = head + chunk->size() - kPage;
  // first page
  Static::chunk_rtree()->Delete(head);

  // last page
  if (chunk->size() > kPage) {
    Static::chunk_rtree()->Delete(tail);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      Static::chunk_rtree()->Delete(curr);
    }
  }
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, bool is_slab) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;
  if (pind < kNumGePageClasses && !avail_bins[pind].empty()) {
    // std::cout << "bin pind: " << pind << " pop." << std::endl;

    chunk = avail_bins[pind].pop();

  } else {
    void *chunk_data = ChunkAllocMap(cs);
    chunk = base_alloc_.New<Chunk>(chunk_data, cs);

    /*
    std::cout << "map chunk: " << chunk
              << " cs: " << cs
              << " pind: " << pind
              << std::endl;
              */

    if (nullptr == chunk) {
      fprintf(stderr, "%s() ArenaAlloc from map failed: size %zu\n", __func__, cs);
      return nullptr;
    }
  }

  assert(chunk);

  chunk->set_slab(is_slab);
  chunk->set_arena(&arena_);
  RegisterRadix(chunk);

  return chunk;
}

void
ChunkManager::DallocChunk(Chunk *chunk) {
  size_t pind = sz_to_pind(chunk->size());

  DeregisterRadix(chunk);

  if (pind < kNumGePageClasses && avail_bins[pind].size() < kMaxBinSize) {
    // std::cout << "bin pind: " << pind << " push: " << chunk << std::endl;

    avail_bins[pind].push(chunk);
  } else {

    /*
    std::cout << "unmap chunk: " << chunk
              << " addr: " << chunk->address()
              << " cs: " << chunk->size()
              << " pind: " << pind
              << std::endl;
              */

    ChunkDallocMap(chunk->address(), chunk->size());
    base_alloc_.Delete(chunk);
  }
}

} // end of namespace ffmalloc
