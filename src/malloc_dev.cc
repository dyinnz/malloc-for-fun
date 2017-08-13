
#include <iostream>

#include "basic.h"
#include "pages.h"

int main() {

  using namespace std;

  cout << "hello world" << endl;
  cout << ChunkAllocMap(kMinAllocMmap) << endl;
  return 0;
}
