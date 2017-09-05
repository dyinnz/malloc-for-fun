
#include <cassert>
#include <iostream>

#include "chunk.h"
#include "simplelogger.h"

namespace {

constexpr char kChunkActive[] = "Active";
constexpr char kChunkDirty[] = "Dirty";
constexpr char kChunkClean[] = "Clean";
constexpr char kChunkUnknown[] = "Unknown";

} // end of namespace

namespace ffmalloc {

std::ostream &operator<<(std::ostream &out, const Chunk &chunk) {
  const char *state = nullptr;
  switch (chunk.state()) {
    case Chunk::State::kActive:state = kChunkActive;
      break;
    case Chunk::State::kDirty:state = kChunkDirty;
      break;
    case Chunk::State::kClean:state = kChunkClean;
      break;
    default:state = kChunkUnknown;
  }

  out << "Chunk[" << &chunk
      << "]{ " "addr: " << chunk.address()
      << " size: " << chunk.size()
      << " region_size: " << chunk.slab_region_size()
      << " epoch: " << static_cast<int>(chunk.epoch())
      << " " << state
      << " }";
  return out;
}

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
  for (auto &chunks: dirty_chunk_) {
    while (!chunks.empty()) {
      OSDeleteChunk(chunks.pop());
    }
  }
  for (auto &chunks: clean_chunk_) {
     while (!chunks.empty()) {
      OSDeleteChunk(chunks.pop());
    }
  }
}

Chunk *
ChunkManager::OSNewChunk(size_t cs, size_t slab_region_size) {
  void *chunk_data = OSAllocMap(nullptr, cs);
  if (nullptr == chunk_data) {
    func_error(logger, "ChunkManager alloc chunk data from os failed");
    return nullptr;
  }

  Chunk *chunk = base_alloc_.New<Chunk>(chunk_data, cs, &arena_, Chunk::State::kDirty, epoch_, slab_region_size);
  if (nullptr == chunk) {
    OSDallocMap(chunk_data, cs);
    func_error(logger, "alloc chunk meta failed");
    return nullptr;

  } else {
    RegisterRtree(chunk);
    stat_.meta += sizeof(Chunk);
    stat_.hold += chunk->size();

    func_debug(logger, "{}", *chunk);
    return chunk;
  }
}

void
ChunkManager::OSDeleteChunk(Chunk *chunk) {
  func_debug(logger, "{}", *chunk);

  DeregisterRtree(chunk);
  OSDallocMap(chunk->address(), chunk->size());
  stat_.hold -= chunk->size();

  base_alloc_.Delete(chunk);
  stat_.meta -= sizeof(Chunk);
}

void
ChunkManager::OSCommitChunk(Chunk *chunk){
  assert(chunk->state() == Chunk::State::kClean);
  OSCommitPage(chunk->address(), chunk->size());
  chunk->set_state(Chunk::State::kDirty);
}

void
ChunkManager::OSReleaseChunk(Chunk *chunk){
  assert(chunk->state() == Chunk::State::kDirty);
  OSReleasePage(chunk->address(), chunk->size());
  chunk->set_state(Chunk::State::kClean);
  stat_.hold -= chunk->size();
}

Chunk *
ChunkManager::SplitChunk(Chunk *curr, size_t head_size) {
  assert(head_size < curr->size());
  Chunk *tail = base_alloc_.New<Chunk>(curr->char_addr() + head_size,
                                       curr->size() - head_size,
                                       &arena_,
                                       curr->state(),
                                       curr->epoch(),
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
  assert(head->state() == tail->state());

  DeregisterRtree(head);
  DeregisterRtree(tail);
  head->set_size(head->size() + tail->size());
  RegisterRtree(head);

  base_alloc_.Delete(tail);
  stat_.meta -= sizeof(Chunk);

  return head;
}

Chunk*
ChunkManager::FindSuitableChunk(ChunkList *bins, size_t bin_size, size_t start_pind) {
  for (size_t i = start_pind; i < bin_size; ++i) {
    if (!bins[i].empty()) {
      Chunk *curr = bins[i].pop();
      if (curr->size() > pind_to_cs(start_pind)) {
        Chunk *tail = SplitChunk(curr, pind_to_cs(start_pind));
        bins[get_fit_pind(tail)].push(tail);
      }
      return curr;
    }
  }
  return nullptr;
}

Chunk *
ChunkManager::AllocChunk(size_t cs, size_t pind, size_t slab_region_size) {
  assert(cs == sz_to_cs(cs));
  assert(pind == sz_to_pind(cs));

  Chunk *chunk = nullptr;

  if (pind < kNumGePageClasses) {
    // first fit search for dirty chunks
    chunk = FindSuitableChunk(dirty_chunk_, kNumGePageClasses, pind);

    // first fit search for clean chunks
    if (nullptr == chunk) {
      chunk = FindSuitableChunk(clean_chunk_, kNumGePageClasses+1, pind);
      if (nullptr != chunk) {
        func_debug(logger, "fetch clean {}", *chunk);
        OSCommitChunk(chunk);
      }
    }

    // mmap if no available chunk
    if (nullptr == chunk) {
      chunk = OSNewChunk(kStandardChunk, 0);
      if (nullptr == chunk) {
        func_error(logger, "alloc standard chunk for splitting failed");
        return nullptr;
      }
      // split original chunk if get a larger one
      if (chunk->size() > cs) {
        Chunk *tail = SplitChunk(chunk, cs);
        dirty_chunk_[get_fit_pind(tail)].push(tail);
      }
    }
    assert(nullptr != chunk);

  } else {
    chunk = OSNewChunk(cs, slab_region_size);
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
  chunk->set_epoch(epoch_);
  if (chunk->is_slab()) {
    RegisterRtreeInterior(chunk);
  }

  stat_.request += chunk->size();

  Event();
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
  chunk->set_epoch(epoch_);
  if (chunk->is_slab()) {
    DeregisterRtreeInterior(chunk);
  }

  if (pind < kNumGePageClasses) {
    // TODO: limit the number of dirty chunks
    dirty_chunk_[pind].push(chunk);

  } else {
    OSDeleteChunk(chunk);
  }

  Event();
}

void
ChunkManager::TriggerGC() {
  func_debug(logger, "epoch: {}", static_cast<int>(epoch_));
  for (size_t i = 0; i < kNumGePageClasses; ++i) {
    Chunk *curr = dirty_chunk_[i].first();
    while (curr != dirty_chunk_[i].end()) {
      if (curr->epoch() < epoch_) {
        Chunk *temp = curr;
        curr = curr->next();

        dirty_chunk_[i].erase(temp);
        OSReleaseChunk(temp);
        PushCleanChunk(i, temp);
      } else {
        curr = curr->next();
      }
    }
  }

  // after gc, we step a epoch
  epoch_ += 1;
  if (UINT8_MAX == epoch_) {
    epoch_ = 0;
  }
}

void
ChunkManager::PushCleanChunk(size_t pind, Chunk *chunk) {
  func_debug(logger, "pind: {}, {}", pind, *chunk);
  assert(chunk->state() == Chunk::State::kClean);

  Chunk *prev = Static::chunk_rtree()->LookUp(chunk->char_addr() - kPage);
  if (nullptr != prev && prev->arena() == chunk->arena() && Chunk::State::kClean == prev->state()) {
    clean_chunk_[get_fit_pind(prev)].erase(prev);
    chunk = MergeChunk(prev, chunk);
  }

  Chunk *next = Static::chunk_rtree()->LookUp(chunk->char_addr() + chunk->size());
  if (nullptr != next && next->arena() == chunk->arena() && Chunk::State::kClean == next->state()) {
    clean_chunk_[get_fit_pind(next)].erase(next);
    chunk = MergeChunk(chunk, next);
  }

  clean_chunk_[get_fit_pind(chunk)].push(chunk);
}

} // end of namespace ffmalloc
