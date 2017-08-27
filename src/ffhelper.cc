//
// Created by Guo Yingzhong on 2017/8/26.
//

#include <sstream>
#include <iomanip>
#include "ffhelper.h"
#include "simplelogger.h"

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

void PrintStat(const ChunkManager::Stat &stat) {
  logger.notice("ChunkManager Stat: request {} hold {} meta {} clean {}",
                stat.request, stat.hold, stat.meta, stat.clean);
}

void PrintStat(const Arena::SmallStatArray &stat_array) {
  std::ostringstream oss;
  oss << "Arena.Small Stat:\n";
  for (size_t i = 0; i < stat_array.size(); ++i) {
    if (0 == stat_array[i].hold) {
      continue;
    }
    oss << "[" << std::setw(2) << i << "]"
        << " cs: " << std::setw(5) << ind_to_cs(i)
        << " slab: " << std::setw(5) << lookup_slab_size(i)
        << " request: " << stat_array[i].request
        << " hold: " << stat_array[i].hold;
    if (0 != stat_array[i].hold) {
      oss << " per: " << std::setprecision(2) << stat_array[i].request / float(stat_array[i].hold);
    }
    oss << "\n";
  }
  oss << "--- END of Arena.Small Stat ---";
  logger.notice("{}", oss.str());
}

void PrintStat(const Arena::LargeStat &stat) {
  logger.notice("Arena.Large Stat: request {}",
                stat.request);
}

void PrintStat(const ThreadAllocator::CacheStatArray &stat_array) {
  std::ostringstream oss;
  oss << "ThreadAllocator.Small Stat:\n";
  for (size_t i = 0; i < stat_array.size(); ++i) {
    if (0 == stat_array[i].num_alloc && 0 == stat_array[i].num_free) {
      continue;
    }
    oss << "[" << std::setw(2) << i << "]"
        << " cs: " << std::setw(5) << ind_to_cs(i)
        << " alloc times: " << stat_array[i].num_alloc
        << " free  times: " << stat_array[i].num_free;
    oss << "\n";
  }
  oss << "--- END of ThreadAllocator.Small Stat ---";
  logger.notice("{}", oss.str());
}

void PrintStat(const ThreadAllocator::LargeStat &stat) {
  logger.notice("ThreadAllocator.Large Stat: alloc {}, free {}", stat.alloc, stat.free);
}

} // end of namespace ffmalloc
