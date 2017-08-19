
#include <iostream>

#include "basic.h"
#include "pages.h"
#include "chunk.h"

int main() {

  using namespace std;
  using namespace ffmalloc;

  cout << "hello world" << endl;

  cout << "Chunk size: " << sizeof(Chunk) << endl;
  return 0;
}
