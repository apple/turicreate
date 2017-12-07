project(gl_numpy)

if (NOT WIN32)

ExternalProject_Add(gl_numpy_numpy
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}
  GIT_REPOSITORY https://github.com/turi-code/numpy
  GIT_TAG patched_v1.9.2
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

ExternalProject_Add_Step(gl_numpy_numpy git_branch
  COMMENT "Switch to branch patched_v1.9.2"
  COMMAND ${GIT_EXECUTABLE} fetch --all
  COMMAND ${GIT_EXECUTABLE} checkout -f patched_v1.9.2
  DEPENDERS update
  DEPENDEES download
  ALWAYS 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/gl_numpy_numpy)


# The little bit of cython code to let us create a numpy array from a raw pointer
# and have numpy own the pointer and delete it when done
if (APPLE)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generated_osx)
else()
  # linux
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generated)
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/gl_numpy_numpy/numpy/core/include)
set_source_files_properties(pointer_to_ndarray.pyx PROPERTIES CYTHON_IS_CXX TRUE)
cython_add_module(pointer_to_ndarray pointer_to_ndarray.pyx)
add_dependencies(pointer_to_ndarray gl_numpy_numpy)


make_library(gl_numpy
  SOURCES
  memory_mapped_sframe.cpp
  alt_malloc.cpp
  numpy_to_sarray.cpp
  pydatamem_intercept.cpp
  REQUIRES
  sframe flexible_type user_pagefault minipsutil_static so_utils)

endif()
