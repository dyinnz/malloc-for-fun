# Malloc for Fun

This is a malloc implementation for learning and practising memory management. So I call it "malloc for fun" or ffmalloc.



### Why I write it

In Jul. and Aug. 2017, I have read parts of jemalloc and tcmalloc source code and learned some basic tactics to manange memory efficicently. Then I decide to write my own malloc library — ffmalloc, which helps me understand the principle hiden in source code.

The ffmalloc use some common tactics which are used by jemalloc or tcmalloc, such as thread cache, slab system, incremental  GC and so on. In fact, ffmalloc have no any cool feature invented by me because I just implement it. Memory Management is an old topic and has been research deeply, so it is hard to invent better algorithm.



### Build

I use googletest for unit testing, please "git clone" it to root directory of project.

I use boost for multi-thread testing, and link the boost_system and boost_thread-mt. ( On MacOS, there is only boost_thread-mt instead of boost_thread, maybe you modify the CMakeLists.txt )

run

```bash
cmake . && make
```



### Usage

ffmalloc implement four standard C memory allocation API: malloc, calloc, realloc and free.