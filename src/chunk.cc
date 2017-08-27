
#include <cassert>

#include "chunk.h"
#include "simplelogger.h"

namespace ffmalloc {

static void
RegisterRtree(Chunk *chunk) {
  char *head = chunk->char_addr();
  // first page
  Static::chunk_rtree()->Insert(head, chunk);
  // last page
  if (chunk->size() > kPage) {
    char *tail = head + chunk->size() - kPage;
    Static::chunk_rtree()->Insert(tail, chunk);
  }
}

static void
DeregisterRtree(Chunk *chunk) {
  char *head = chunk->char_addr();
  // first page
  Static::chunk_rtree()->Delete(head);
  // last page
  if (chunk->size() > kPage) {
    char *tail = head + chunk->size() - kPage;
    Static::chunk_rtree()->Delete(tail);
  }
}

static void
RegisterRtreeInterior(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  for (char *curr = head + kPage; curr < tail; curr += kPage) {
    Static::chunk_rtree()->Insert(curr, chunk);
  }
}

static void
DeregisterRtreeInterior(Chunk *chunk) {
  char *head = chunk->char_addr();
  char *tail = head + chunk->size() - kPage;
  for (char *curr = head + kPage; curr < tail; curr += kPage) {
    Static::chunk_rtree()->Delete(curr);
  }
}

static size_t
get_fit_pind(Chunk *chunk) {
  return sz_to_pind(chunk->size() + 1) - 1;
}

ChunkManager::~ChunkManager() {
  for (auto &chunks: avail_bins_) {
    while (!chunks.empty()) {
      OSUnmapChunk(chunks.pop());
    }
  }
}

Chunk *
ChunkManager::OSMapChunk(size_t cs, size_t slab_region_size) {
  void *chunk_data = OSAllocMap(cs);
  if (nullptr == chunk_data) {
    func_error(logger, "ChunkManager alloc chunk data from os failed");
    return nullptr;
  }

  Chunk *chunk = base_alloc_.New<Chunk>(chunk_data, cs, &arena_, Chunk::State::kDirty, slab_region_size);
  if (nullptr == chunk) {
    OSDallocMap(chunk_data, cs);
    func_error(logger, "alloc chunk meta failed");
    return nullptr;

  } else {
    RegisterRtree(chunk);
    stat_.meta += sizeof(Chunk);
    stat_.hold += chunk->size();
    return chunk;
  }
}

void
ChunkManager::OSUnmapChunk(Chunk *chunk) {
  DeregisterRtree(chunk);
  OSDallocMap(chunk->address(), chunk->size());
  stat_.hold -= chunk->size();

  base_alloc_.Delete(chunk);
  stat_.meta -=  sizeof(Chunk);
}

Chunk *
ChunkManager::SplitChunk(Chunk *curr, size_t head_size) {
  assert(head_size < curr->size());
  Chunk *tail = base_alloc_.New<Chunk>(curr->char_addr() + head_size,
                                       curr->size() - head_size,
                                       &arena_,
                                       curr->state(),
                                       kNonSlabAttr);
  if (nullptr == tail) {
    return nullptr;
  }
  stat_.meta += sizeof(Chunk);

  DeregisterRtree(curr);
  curr->set_size(head_size);
  RegisterRtree(curr);
  RegisterRtree(tail);
  return tail;
}

Chunk *
ChunkManager::MergeChunk(Chunk *head, Chunk *tail) {
  assert(head->char_addr() + head->size() == tail->char_addr());
  assert(head->arena() == tail->arena());

  DeregisterRtree(head);
  DeregisterRtree(tail);
  head->set_size(head->size() + tail->size());
  RegisterRtree(head);

  base_alloc_.Delete(tail);
  stat_.meta -= sizeof(Chunk);

  return head;
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, size_t slab_region_size) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;

  /*
  if (pind < kNumGePageClasses && !avail_bins_[pind].empty()) {
    chunk = avail_bins_[pind].pop();
    */

  if (pind < kNumGePageClasses) {
    // first fit
    for (size_t i = pind; i < kNumGePageClasses; ++i) {
      if (!avail_bins_[i].empty()) {
        chunk = avail_bins_[i].pop();
        break;
      }
    }
    // mmap if no available chunk
    if (nullptr == chunk) {
      chunk = OSMapChunk(kStandardChunk, 0);
      if (nullptr == chunk) {
        func_error(logger, "alloc standard chunk for splitting failed");
        return nullptr;
      }
    }

    // split if gets a larger chunk
    if (chunk->size() > cs) {
      Chunk *tail = SplitChunk(chunk, cs);
      avail_bins_[get_fit_pind(tail)].push(tail);
    }

  } else {
    chunk = OSMapChunk(cs, slab_region_size);
    if (nullptr == chunk) {
      func_error(logger, "alloc large chunk failed");
      return nullptr;
    }
  }

  assert(Chunk::State::kDirty == chunk->state());
  assert(&arena_ == chunk->arena());
  assert(chunk->size() == sz_to_cs(chunk->size()));

  chunk->set_slab_region_size(slab_region_size);
  chunk->set_state(Chunk::State::kActive);
  if (chunk->is_slab()) {
    RegisterRtreeInterior(chunk);
  }

  stat_.request += chunk->size();

  return chunk;
}

void
ChunkManager::DallocChunk(Chunk *chunk) {
  assert(nullptr != chunk);
  assert(Chunk::State::kActive == chunk->state());
  assert(&arena_ == chunk->arena());
  assert(chunk->size() == sz_to_cs(chunk->size()));

  stat_.request -= chunk->size();

  size_t pind = sz_to_pind(chunk->size());

  chunk->set_state(Chunk::State::kDirty);
  if (chunk->is_slab()) {
    DeregisterRtreeInterior(chunk);
  }

  /*
  if (pind < kNumGePageClasses && avail_bins_[pind].size() < kMaxBinSize) {
    avail_bins_[pind].push(chunk);
    */

  if (pind < kNumGePageClasses) {
    Chunk *prev = Static::chunk_rtree()->LookUp(chunk->char_addr() - kPage);
    if (nullptr != prev && prev->arena() == chunk->arena() && Chunk::State::kDirty == prev->state()) {
      avail_bins_[get_fit_pind(prev)].erase(prev);
      chunk = MergeChunk(prev, chunk);
    }

    Chunk *next = Static::chunk_rtree()->LookUp(chunk->char_addr() + chunk->size());
    if (nullptr != next && next->arena() == chunk->arena() && Chunk::State::kDirty == next->state()) {
      avail_bins_[get_fit_pind(next)].erase(next);
      chunk = MergeChunk(chunk, next);
    }

    size_t new_pind = get_fit_pind(chunk);
    if (new_pind < kNumGePageClasses) {
      avail_bins_[new_pind].push(chunk);
    } else {
      OSUnmapChunk(chunk);
    }

  } else {
    OSUnmapChunk(chunk);
  }
}

} // end of namespace ffmalloc
