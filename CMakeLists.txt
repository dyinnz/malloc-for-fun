project(malloc-for-fun)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -g")

# googletest
add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)

# project include
include_directories(include)
include_directories(src)

add_library(ffalloc.o OBJECT
        include/arena.h
        include/base_alloc.h
        include/basic.h
        include/bitmap.h
        include/chunk.h
        include/list.h
        include/pages.h
        include/radix_tree.h
        include/size.h
        include/size_classes.h
        include/static.h
        include/thread_alloc.h
        src/arena.cc
        src/base_alloc.cc
        src/chunk.cc
        src/size_classes.cc
        src/pages.cc
        src/static.cc)

# executable
add_executable(ffalloc_dev
        $<TARGET_OBJECTS:ffalloc.o>
        src/ffalloc_dev.cc
        )

# test
add_executable(test_sample
        $<TARGET_OBJECTS:ffalloc.o>
        test/test_sample.cc
        )

target_link_libraries(test_sample gtest)

add_executable(test_correct
        $<TARGET_OBJECTS:ffalloc.o>
        test/test_correct.cc
        )

target_link_libraries(test_correct gtest)
