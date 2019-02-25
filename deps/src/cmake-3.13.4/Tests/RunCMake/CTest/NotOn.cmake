set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
include(CTest)
configure_file(CTestTestfile.cmake.in CTestTestfile.cmake)
