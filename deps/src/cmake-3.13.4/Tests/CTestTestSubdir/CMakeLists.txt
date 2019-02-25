cmake_minimum_required (VERSION 2.6)
project(CTestTestSubdir)
include(CTest)

add_subdirectory(subdir)
subdirs(subdir2)
subdirs("${CTestTestSubdir_SOURCE_DIR}/subdir3")
