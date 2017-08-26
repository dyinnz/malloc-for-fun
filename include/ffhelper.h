//
// Created by 郭映中 on 2017/8/26.
//

#pragma once

#include <iostream>
#include "chunk.h"
#include "arena.h"
#include "simplelogger.h"

namespace ffmalloc {

extern simplelogger::Logger logger;

std::ostream &operator<<(std::ostream &out, const Chunk &chunk);

void PrintStat(const ChunkManager::Stat &stat);
void PrintStat(const Arena::SmallStatArray &stat_array);
void PrintStat(const Arena::LargeStat &stat);

} // end of namespace ffmalloc
