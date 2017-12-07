cmake_minimum_required (VERSION 2.6)
project(CTestTestFailure)
include(CTest)

add_executable (NoBuild badCode.cxx)
target_link_libraries (NoBuild ${EXTRA_LIBS})

add_test (TestNoExe NoBuild)
