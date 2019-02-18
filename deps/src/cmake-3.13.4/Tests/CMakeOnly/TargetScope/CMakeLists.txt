cmake_minimum_required (VERSION 2.8)
project(TargetScope NONE)

add_subdirectory(Sub)

if(TARGET SubLibLocal)
  message(FATAL_ERROR "SubLibLocal visible in top directory")
endif()
if(NOT TARGET SubLibGlobal)
  message(FATAL_ERROR "SubLibGlobal not visible in top directory")
endif()

add_subdirectory(Sib)
