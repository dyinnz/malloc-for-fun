
#pragma once

#include "basic.h"
#include "size.h"
#include "base_alloc.h"
#include "list.h"
#include "radix_tree.h"
#include "bitmap.h"

namespace ffmalloc {

class Arena;

class Chunk : public ListNode<Chunk> {
 public:
  typedef Bitmap<kMaxSlabRegions> SlabBitmap;
  enum class State : uint8_t { kActive, kDirty, kClean };

  Chunk(void *address, size_t size, Arena *arena, State state, uint8_t epoch, size_t region_size)
      : address_(address),
        size_(size),
        arena_(arena),
        state_(state),
        epoch_(epoch),
        slab_region_size_(static_cast<uint16_t>(region_size)),
        slab_bitmap_() {
    if (is_slab()) {
      slab_bitmap().init(size_ / slab_region_size_);
    }
  }

  Chunk() : Chunk(nullptr, 0, nullptr, State::kClean, 0, 0) {
  }

  Chunk(const Chunk &) = delete;
  Chunk(Chunk &&) = delete;
  Chunk &operator=(const Chunk &) = delete;
  Chunk &operator=(Chunk &&) = delete;

  Arena *arena() {
    return arena_;
  }

  void *address() const {
    return address_;
  }

  void set_address(void *address) {
    address_ = address;
  }

  char *char_addr() const {
    return static_cast<char *>(address_);
  }

  size_t size() const {
    return size_;
  }

  void set_size(size_t size) {
    size_ = size;
  }

  bool is_slab() const {
    return 0 != slab_region_size();
  }

  size_t slab_region_size() const {
    return slab_region_size_;
  }

  void set_slab_region_size(size_t region_size) {
    slab_region_size_ = static_cast<uint16_t>(region_size);
    if (is_slab()) {
      slab_bitmap().init(size_ / slab_region_size_);
    }
  }

  State state() const {
    return state_;
  }

  void set_state(State state) {
    state_ = state;
  }

  SlabBitmap &slab_bitmap() {
    return slab_bitmap_;
  }

  void set_epoch(uint8_t epoch) {
    epoch_ = epoch;
  }

  uint8_t epoch() const {
    return epoch_;
  }

 private:
  void *address_{nullptr};
  size_t size_{0};
  Arena *arena_;
  State state_{State::kDirty};
  uint8_t epoch_ {0};
  uint16_t slab_region_size_{0};
  SlabBitmap slab_bitmap_;
};

typedef List<Chunk> ChunkList;
// typedef RadixTree<Chunk> ChunkRTree;

class ChunkManager {
 private:
// TODO:
  static constexpr int kMaxBinSize = 20;
  static constexpr size_t kMaxEventTick = 4096;

 public:
  struct Stat {
    size_t hold{0};
    size_t request{0};
    size_t meta{0};
    size_t clean{0};
  };

  ChunkManager(Arena &arena, BaseAllocator &base_alloc)
      : arena_(arena), base_alloc_(base_alloc), stat_() {
  }

  ChunkManager(const ChunkManager &) = delete;
  ChunkManager(ChunkManager &&) = delete;
  ChunkManager &operator=(const ChunkManager &) = delete;
  ChunkManager &operator=(ChunkManager &&) = delete;

  ~ChunkManager();

  Chunk *AllocChunk(size_t cs, size_t pind, size_t slab_region_size);

  void DallocChunk(Chunk *chunk);

  const Stat &stat() const {
    return stat_;
  }

  Arena& arena() {
    return arena_;
  }

  void Event();
  void TriggerGC();

 private:
  Chunk *OSNewChunk(size_t cs, size_t slab_region_size);
  void OSDeleteChunk(Chunk *chunk);

  bool OSMapChunk(Chunk *chunk);
  void OSUnmapChunk(Chunk *chunk);

  void PushCleanChunk(size_t pind, Chunk *chunk);

  Chunk *SplitChunk(Chunk *curr, size_t head_size);
  Chunk *MergeChunk(Chunk *head, Chunk *tail);

 private:
  Arena &arena_;
  BaseAllocator &base_alloc_;
  ChunkList dirty_chunk_[kNumGePageClasses];
  ChunkList clean_chunk_[kNumGePageClasses+1];

  Stat stat_;
  size_t event_tick_ {0};
  uint8_t epoch_ {0};
};

std::ostream &operator<<(std::ostream &out, const Chunk &chunk);

/*------------------------------------------------------------------*/

inline void
ChunkManager::Event() {
  event_tick_ += 1;
  if (event_tick_ > kMaxEventTick) {
    event_tick_ = 0;
    TriggerGC();
  }
}

} // end of namespace ffmalloc
