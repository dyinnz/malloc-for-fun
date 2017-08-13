project(malloc-for-fun)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -g")

# googletest
add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)

# project include
include_directories(include)
include_directories(src)

add_library(ffalloc.o OBJECT
  src/size_classes.cc
  src/pages.cc
  src/base_alloc.cc
  src/chunk.cc
  src/arena.cc
  )

# executable
add_executable(ffalloc_dev
  $<TARGET_OBJECTS:ffalloc.o>
  src/malloc_dev.cc
  )

# test
add_executable(test_sample
  $<TARGET_OBJECTS:ffalloc.o>
  test/test_sample.cc
  )

target_link_libraries(test_sample gtest)
