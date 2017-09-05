project(malloc-for-fun)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -g")

# googletest
add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)

# project include
include_directories(src)

add_library(ff_core.o OBJECT
        src/arena.h
        src/base_alloc.h
        src/basic.h
        src/bitmap.h
        src/chunk.h
        src/ffhelper.h
        src/ffmalloc.h
        src/list.h
        src/pages.h
        src/radix_tree.h
        src/size.h
        src/size_classes.h
        src/static.h
        src/thread_alloc.h
        src/simplelogger.h
        src/arena.cc
        src/chunk.cc
        src/size_classes.cc
        src/ffhelper.cc
        src/ffmalloc.cc
        src/pages.cc
        src/static.cc
        src/base_stl_alloc.h)

# executable
add_executable(ff_dev
        $<TARGET_OBJECTS:ff_core.o>
        src/ff_dev.cc
        )

add_library(ffmalloc SHARED
        $<TARGET_OBJECTS:ff_core.o>
        )

add_library(ffmalloc_static STATIC
        $<TARGET_OBJECTS:ff_core.o>
        )

# ------------------------------------------------------------------------ #
# test
add_executable(test_sample
        $<TARGET_OBJECTS:ff_core.o>
        test/test_sample.cc
        )

target_link_libraries(test_sample gtest)

add_executable(test_correct
        $<TARGET_OBJECTS:ff_core.o>
        test/test_correct.cc
        test/test_common.h)

target_link_libraries(test_correct gtest)

add_executable(test_multithread
        $<TARGET_OBJECTS:ff_core.o>
        test/test_multithread.cc
        test/test_common.h)

target_link_libraries(test_multithread
        gtest
        boost_system
        boost_thread-mt
        )
