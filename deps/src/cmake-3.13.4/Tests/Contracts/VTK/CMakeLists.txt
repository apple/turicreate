# The VTK external project for CMake
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)
project(VTK)
include(ExternalProject)

# find "HOME".  VTK will be downloaded & built within a subdirectory.
include(${CMAKE_CURRENT_SOURCE_DIR}/../Home.cmake)

set(base_dir "${HOME}/.cmake/Contracts/VTK")

if(NOT DEFINED SITE)
  site_name(SITE)
endif()

# configure our dashboard script
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Dashboard.cmake.in
  ${base_dir}/Dashboard.cmake
  @ONLY)

# build & test VTK's release branch
ExternalProject_Add(${PROJECT_NAME}
  GIT_REPOSITORY "git://vtk.org/VTK.git"
  GIT_TAG "release"
  PREFIX ${base_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${CMAKE_CTEST_COMMAND} -S "${base_dir}/Dashboard.cmake"
  INSTALL_COMMAND ""
)
