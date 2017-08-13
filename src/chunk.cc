
#include <cassert>

#include "chunk.h"
#include "pages.h"

RadixTree<Chunk> g_radix_tree;

static void RegisterRadix(Chunk *chunk) {
  char *head = static_cast<char*>(chunk->address());
  char *tail = head + chunk->size() - kPage;
  // first page
  g_radix_tree.Insert(head, chunk);

  // last page
  if (chunk->size() > kPage) {
    g_radix_tree.Insert(tail, chunk);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      g_radix_tree.Insert(curr, chunk);
    }
  }
}

static void DeregisterRadix(Chunk *chunk) {
    char *head = static_cast<char*>(chunk->address());
  char *tail = head + chunk->size() - kPage;
  // first page
  g_radix_tree.Delete(head);

  // last page
  if (chunk->size() > kPage) {
    g_radix_tree.Delete(tail);
  }

  // interior page
  if (chunk->is_slab()) {
    for (char *curr = head + kPage;
         curr < tail;
         curr += kPage) {
      g_radix_tree.Delete(curr);
    }
  }
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, bool is_slab) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;
  if (pind < kNumGePageClasses && !avail_bins[pind].empty()) {
    std::cout << "bin pind: " << pind << " pop." << std::endl;

    chunk = avail_bins[pind].pop();

  } else {
    void *chunk_data = ChunkAllocMap(cs);
    chunk = base_alloc_.New<Chunk>(chunk_data, cs);

    std::cout << "map chunk: " << chunk
              << " cs: " << cs
              << " pind: " << pind
              << std::endl;

    if (nullptr == chunk) {
      fprintf(stderr, "%s() alloc from map failed: size %zu\n", __func__, cs);
      return nullptr;
    }
  }

  assert(chunk);

  chunk->set_slab(is_slab);
  RegisterRadix(chunk);

  return chunk;
}

void
ChunkManager::DallocChunk(Chunk *chunk) {
  size_t pind = sz_to_pind(chunk->size());

  DeregisterRadix(chunk);

  if (pind < kNumGePageClasses && avail_bins[pind].size() < kMaxBinSize) {
    std::cout << "bin pind: " << pind << " push: " << chunk << std::endl;

    avail_bins[pind].push(chunk);
  } else {

    std::cout << "unmap chunk: " << chunk
              << " cs: " << chunk->size()
              << " pind: " << pind
              << std::endl;

    ChunkDallocMap(chunk->address(), chunk->size());
    base_alloc_.Delete(chunk);
  }
}
