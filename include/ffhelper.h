//
// Created by 郭映中 on 2017/8/26.
//

#pragma once

#include <iostream>
#include "chunk.h"
#include "arena.h"
#include "thread_alloc.h"

namespace ffmalloc {

std::ostream &operator<<(std::ostream &out, const Chunk &chunk);

void PrintStat(const ChunkManager::Stat &stat);
void PrintStat(const Arena::SmallStatArray &stat_array);
void PrintStat(const Arena::LargeStat &stat);
void PrintStat(const ThreadAllocator::CacheStatArray &stat_array);
void PrintStat(const ThreadAllocator::LargeStat &stat);

} // end of namespace ffmalloc
