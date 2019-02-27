cmake_minimum_required(VERSION 2.8)
project(mfc_driver)

include(CTest)
include(ExternalProject)


set(CMAKE_MFC_FLAG_VALUE "2")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.SharedMfcDll.txt
  @ONLY
  )

ExternalProject_Add(mfcShared
  URL ${CMAKE_CURRENT_SOURCE_DIR}/mfc1
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.SharedMfcDll.txt
    <SOURCE_DIR>/CMakeLists.txt
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )


set(CMAKE_MFC_FLAG_VALUE "1")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.StaticMfcLib.txt
  @ONLY
  )

ExternalProject_Add(mfcStatic
  URL ${CMAKE_CURRENT_SOURCE_DIR}/mfc1
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.StaticMfcLib.txt
    <SOURCE_DIR>/CMakeLists.txt
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )


set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ValidateBuild.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ValidateBuild.cmake
  @ONLY
  )
add_test(
  NAME validate
  COMMAND ${CMAKE_COMMAND}
    -P "${CMAKE_CURRENT_BINARY_DIR}/ValidateBuild.cmake"
  )


add_test(
  NAME environment
  COMMAND ${CMAKE_COMMAND} -E environment
  )


message(STATUS "===== environment =====")
execute_process(COMMAND ${CMAKE_COMMAND} -E environment)
message(STATUS "===== /environment =====")
