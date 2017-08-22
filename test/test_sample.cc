
#include <bitset>
#include <random>
#include <atomic>

#include "gtest/gtest.h"

#include "basic.h"
#include "size.h"
#include "pages.h"
#include "chunk.h"
#include "arena.h"
#include "list.h"
#include "radix_tree.h"
#include "bitmap.h"
#include "static.h"
#include "thread_alloc.h"

using namespace ffmalloc;

TEST(BasicInfo, BasicInfo) {
  using std::cout;
  using std::endl;

  cout << "atomic " << sizeof(std::atomic<void*>) << endl;

  cout << "Allocated by BaseAllocator:" << endl;
  cout << "CacheBin " << sizeof(CacheBin) << endl;
  cout << "ThreadAllocator " << sizeof(ThreadAllocator) << endl;
  cout << "Arena " << sizeof(Arena) << endl;
  cout << "RadixTree " << sizeof(RadixTree<Chunk>) << endl;
  cout << "Chunk " << sizeof(Chunk) << endl;
}

TEST(Alignment, Alignment) {
  for (int i = 1; i <= kCacheLine; ++i) {
    EXPECT_EQ(kCacheLine, cacheline_ceil(i));
  }
  for (int i = kCacheLine + 1; i <= kCacheLine * 2; ++i) {
    EXPECT_EQ(kCacheLine * 2, cacheline_ceil(i));
  }
}

void RangeRWTest(void *address, size_t size) {
  int *p = static_cast<int *>(address);
  for (int i = 0; i < size / sizeof(int); ++i) {
    p[i] = 0x12345678;
  }
  for (int i = 0; i < size / sizeof(int); ++i) {
    EXPECT_EQ(p[i], 0x12345678);
  }
}

TEST(OSMap, OSMap) {
  constexpr size_t size = 4 * 1024 * 1024;

  void *address = OSAllocMap(size);

  RangeRWTest(address, size);

  OSDallocMap(address, size);
}

TEST(ChunkManager, ChunkManager) {
  BaseAllocator base_alloc;
  Arena arena;
  ChunkManager mgr(arena, base_alloc);
  Chunk *chunk = nullptr;

  constexpr size_t huge_size = 4 * 1024 * 1024;

  chunk = mgr.AllocChunk(huge_size, sz_to_pind(huge_size), false);
  RangeRWTest(chunk->address(), huge_size);
  mgr.DallocChunk(chunk);

  constexpr size_t large_size = 4 * 1024;
  chunk = mgr.AllocChunk(large_size, sz_to_pind(large_size), false);
  RangeRWTest(chunk->address(), large_size);
  mgr.DallocChunk(chunk);
}

TEST(List, ChunkList) {
  constexpr size_t count = 3;
  ChunkList chunk_list;

  BaseAllocator base_alloc;

  for (int i = 0; i < count; ++i) {
    Chunk *chunk = base_alloc.New<Chunk>(nullptr, i);
    chunk_list.push(chunk);
  }

  int num = count - 1;
  while (!chunk_list.empty()) {
    Chunk *chunk = chunk_list.pop();

    EXPECT_EQ(num, chunk->size());

    base_alloc.Delete(chunk);
    num -= 1;
  }

  Chunk *x = base_alloc.New<Chunk>(nullptr, 111);
  Chunk *y = base_alloc.New<Chunk>(nullptr, 222);
  Chunk *z = base_alloc.New<Chunk>(nullptr, 333);
  chunk_list.push(x);
  chunk_list.push(y);
  chunk_list.push(z);

  chunk_list.erase(y);

  EXPECT_EQ(chunk_list.pop()->size(), 333);
  EXPECT_EQ(chunk_list.pop()->size(), 111);
  EXPECT_EQ(chunk_list.empty(), true);
}

TEST(RadixTree, RadixTree) {
  RadixTree<Chunk> radix_tree;
  Chunk chunk_data;
  Chunk *chunk = &chunk_data;
  void *k1 = reinterpret_cast<void *>(
      (0x111L << 36) + (0x222L << 24) + (0x333L << 12) + 0x444L
  );
  radix_tree.Insert(k1, chunk);
  EXPECT_EQ(chunk, radix_tree.LookUp(k1));

  radix_tree.Delete(k1);
  ASSERT_DEATH({ radix_tree.LookUp(k1); }, "");

}

TEST(RadixTree, ChunkRegister) {
  BaseAllocator base_alloc;
  Arena arena;
  ChunkManager mgr(arena, base_alloc);

  constexpr size_t large_size = 4 * kPage;
  Chunk *chunk = mgr.AllocChunk(large_size, sz_to_pind(large_size), false);

  char *ptr = static_cast<char *>(chunk->address());
  EXPECT_EQ(chunk, Static::chunk_rtree()->LookUp(ptr));
  ASSERT_DEATH({ Static::chunk_rtree()->LookUp(ptr + kPage); }, "");
  ASSERT_DEATH({ Static::chunk_rtree()->LookUp(ptr + 2 * kPage); }, "");
  EXPECT_EQ(chunk, Static::chunk_rtree()->LookUp(ptr + 3 * kPage));

  mgr.DallocChunk(chunk);
}

void TestClassIndex(size_t size) {
  const size_t index = sz_to_ind(size);
  EXPECT_EQ(sz_to_cs(size), ind_to_cs(index));
}

TEST(Size, Size) {
  EXPECT_EQ(sz_to_cs(7), 8);
  EXPECT_EQ(sz_to_cs(8), 8);
  EXPECT_EQ(sz_to_cs(9), 16);
  EXPECT_EQ(sz_to_cs(4096), 4096);
  EXPECT_EQ(sz_to_cs(4097), 5 * 1024);

  EXPECT_EQ(sz_to_cs(16 * 1024 + 1), 20 * 1024);
  EXPECT_EQ(sz_to_cs(20 * 1024), 20 * 1024);
  EXPECT_EQ(sz_to_cs(32 * 1024), 32 * 1024);

  TestClassIndex(16 * 1024 + 1);
  TestClassIndex(20 * 1024);
  TestClassIndex(32 * 1024);
}

TEST(Bitmap, Bitmap) {
  Bitmap<512> bitmap;
  bitmap.init(64);
  for (int i = 0; i < 64; ++i) {
    int get = bitmap.ffs_and_reset();
    ASSERT_EQ(get, i);
  }

  Bitmap<512> bitmap127;
  bitmap127.init(127);
  EXPECT_TRUE(bitmap127.all());
  for (int i = 0; i < 64; ++i) {
    int get = bitmap127.ffs_and_reset();
    ASSERT_EQ(get, i);
  }
  EXPECT_FALSE(bitmap127.all());
  for (int i = 64; i < 127; ++i) {
    int get = bitmap127.ffs_and_reset();
    ASSERT_EQ(get, i);
  }
  EXPECT_FALSE(bitmap127.any());

  bitmap127.set(100);
  EXPECT_TRUE(bitmap127.any());
  EXPECT_EQ(bitmap127.ffs_and_reset(), 100);
  EXPECT_FALSE(bitmap127.any());
}

TEST(Arena14k, Arena14k) {
  ArenaAllocator aa;

  for (int i = 0; i < 10; ++i) {
    aa.ArenaDalloc(aa.ArenaAlloc(14 * 1024));
  }
}

TEST(ArenaAllocator, ArenaAllocator) {
  ArenaAllocator aa;
  std::default_random_engine gen;
  std::uniform_int_distribution<size_t> dis(1, 128 * 1024);
  int times = 100;

  for (int i = 0; i < times; ++i) {
    void *ptr = aa.ArenaAlloc(dis(gen));
    ASSERT_NE(nullptr, ptr);
    aa.ArenaDalloc(ptr);
  }
}

TEST(ThreadAllocator, Exist) {
  EXPECT_NE(nullptr, Static::thread_alloc());
}

TEST(ThreadAllocator, Basic) {
  std::default_random_engine gen;
  std::uniform_int_distribution<size_t> dis(1, 128 * 1024);
  int times = 100;

  for (int i = 0; i < times; ++i) {
    void *ptr = Static::thread_alloc()->ThreadAlloc(dis(gen));
    ASSERT_NE(nullptr, ptr);
    Static::thread_alloc()->ThreadDalloc(ptr);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
