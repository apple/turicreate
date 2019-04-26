if(NOT EXISTS "${RunCMake_TEST_BINARY_DIR}/CMakeCache.txt")
  message(FATAL_ERROR "missing test prerequisite CMakeCache.txt")
endif()

set(CMAKE_INSTALL_CMAKE "${RunCMake_TEST_BINARY_DIR}/cmake_install.cmake")

if(NOT EXISTS ${CMAKE_INSTALL_CMAKE})
  message(FATAL_ERROR "${CMAKE_INSTALL_CMAKE} should exist")
endif()
