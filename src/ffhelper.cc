//
// Created by Guo Yingzhong on 2017/8/26.
//

#include <sstream>
#include <iomanip>
#include "ffhelper.h"

namespace ffmalloc {

namespace details {

constexpr char kChunkActive[] = "Active";
constexpr char kChunkDirty[] = "Dirty";
constexpr char kChunkClean[] = "Clean";
constexpr char kChunkUnknown[] = "Unknown";

} // end of namespace details

using namespace simplelogger;

std::ostream &operator<<(std::ostream &out, const Chunk &chunk) {
  const char *state = nullptr;
  switch (chunk.state()) {
    case Chunk::State::kActive:state = details::kChunkActive;
      break;
    case Chunk::State::kDirty:state = details::kChunkDirty;
      break;
    case Chunk::State::kClean:state = details::kChunkClean;
      break;
    default:state = details::kChunkUnknown;
  }

  out << "Chunk[" << &chunk
      << "]{ " "addr: " << chunk.address()
      << " size: " << chunk.size()
      << " region_size: " << chunk.slab_region_size()
      << " " << state
      << " }";
  return out;
}

void AllocatorStatReport(ThreadAllocator &thread_alloc) {
  std::ostringstream oss;
  auto &large_stat = thread_alloc.large_stat();
  auto &cache_stats = thread_alloc.cache_stats();

  oss << "ThreadAllocator[" << thread_alloc.thread_id() << "] of Arena["
      << thread_alloc.arena().index() << "]\n";
  oss << "Large Stat:\nalloc " << large_stat.alloc << ", free " << large_stat.free << "\n";
  oss << "Cache Stat:\n";
  for (size_t i = 0; i < cache_stats.size(); ++i) {
    if (0 == cache_stats[i].num_alloc && 0 == cache_stats[i].num_free) {
      continue;
    }
    oss << "[" << std::setw(2) << i << "]"
        << " cs: " << std::setw(5) << ind_to_cs(i)
        << " alloc times: " << cache_stats[i].num_alloc
        << " free times: " << cache_stats[i].num_free
        << " current caches: " << cache_stats[i].num_cache;
    oss << "\n";
  }
  oss << "--- END of ThreadAllocator Stat ---";
  logger.notice("{}", oss.str());
}

void AllocatorStatReport(Arena &arena) {
  std::ostringstream oss;
  auto &large_stat = arena.large_stat();
  auto &small_stats = arena.small_stats();

  oss << "Arena[" << arena.index() << "]\n";
  oss << "Large Stat:\nrequest " << large_stat.request << "\n";
  oss << "Arena.Small Stat:\n";
  for (size_t i = 0; i < small_stats.size(); ++i) {
    if (0 == small_stats[i].hold) {
      continue;
    }
    oss << "[" << std::setw(2) << i << "]"
        << " cs: " << std::setw(5) << ind_to_cs(i)
        << " slab: " << std::setw(5) << lookup_slab_size(i)
        << " request: " << small_stats[i].request
        << " hold: " << small_stats[i].hold;
    if (0 != small_stats[i].hold) {
      oss << " per: " << std::setprecision(2) << small_stats[i].request / float(small_stats[i].hold);
    }
    oss << "\n";
  }
  oss << "--- END of Arena.Small Stat ---";
  logger.notice("{}", oss.str());
}

void AllocatorStatReport(ChunkManager &chunk_mgr) {
  auto &stat = chunk_mgr.stat();
  logger.notice("ChunkManager of Arena[{}] Stat:request {} hold {} meta {} clean {}",
                chunk_mgr.arena().index(), stat.request, stat.hold, stat.meta, stat.clean);
}

} // end of namespace ffmalloc
