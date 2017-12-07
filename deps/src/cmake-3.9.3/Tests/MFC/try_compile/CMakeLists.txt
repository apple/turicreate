cmake_minimum_required(VERSION 2.8)
project(try_compile_mfc)

set(files
  stdafx.cpp
  stdafx.h
)

set(CMAKE_MFC_FLAG "2")

# VS generators add this automatically based on the CMAKE_MFC_FLAG value,
# but generators matching "Make" require:
add_definitions(-D_AFXDLL)

add_executable(simplest_possible_mfc_exe WIN32 ${files})
