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

add_library(ff_core.o OBJECT
        include/arena.h
        include/base_alloc.h
        include/basic.h
        include/bitmap.h
        include/chunk.h
        include/ffhelper.h
        include/ffmalloc.h
        include/list.h
        include/pages.h
        include/radix_tree.h
        include/size.h
        include/size_classes.h
        include/static.h
        include/thread_alloc.h
        include/simplelogger.h
        src/arena.cc
        src/base_alloc.cc
        src/chunk.cc
        src/size_classes.cc
        src/ffhelper.cc
        src/ffmalloc.cc
        src/pages.cc
        src/static.cc
        test/simplelogger.cc)

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
