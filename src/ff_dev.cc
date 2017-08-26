
#include <iostream>

#include "basic.h"
#include "pages.h"
#include "chunk.h"
#include "simplelogger.h"
#include "ffhelper.h"
#include "ffmalloc.h"

int main() {

  using namespace std;
  using namespace ffmalloc;

  logger.log("chunk size: {}", sizeof(Chunk));

  void *test_ptr = ff_malloc(128);

  PrintStat(Static::get_arena(0)->small_stats());
  PrintStat(Static::get_arena(0)->large_stat());
  PrintStat(Static::get_arena(0)->chunk_manager().stat());

  ff_free(test_ptr);

  return 0;
}
