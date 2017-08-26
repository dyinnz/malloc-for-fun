
#include <iostream>

#include "basic.h"
#include "pages.h"
#include "chunk.h"
#include "simplelogger.h"

simplelogger::Logger logger(simplelogger::kDebug);

int main() {

  using namespace std;
  using namespace ffmalloc;

  logger.log("chunk size: {}", sizeof(Chunk));

  return 0;
}
