cmake_minimum_required (VERSION 2.8.0)
project (OSF15E2)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

# Ahh why does this not work...?
#include(ProcessorCount)
#ProcessorCount(N)
#if(NOT N EQUAL 0)
#  set(CTEST_BUILD_FLAGS -j${N})
#  set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
#endif()

add_subdirectory(bs)
