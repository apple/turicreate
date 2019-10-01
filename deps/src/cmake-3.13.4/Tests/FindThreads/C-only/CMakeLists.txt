cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(FindThreads_C-only C)

set(CMAKE_THREAD_PREFER_PTHREAD On)
find_package(Threads REQUIRED)

if (NOT WIN32)
  add_executable(thr ${CMAKE_CURRENT_SOURCE_DIR}/../../../Modules/CheckForPthreads.c)
  target_link_libraries(thr Threads::Threads)
endif ()
