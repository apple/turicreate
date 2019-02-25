cmake_minimum_required(VERSION 2.8)
project(CTestLimitDashJ NONE)

# This file demonstrates https://gitlab.kitware.com/cmake/cmake/issues/12904
# when configured with CMake 2.8.10.2 and earlier, and when running
# "ctest -j 4" in the resulting build tree. This example is hard-coded
# to assume -j 4 just to reproduce the issue easily. Adjust the
# FAIL_REGULAR_EXPRESSION and PROCESSORS values to reproduce this problem
# with a different ctest -j value...

if(EXISTS "${CMAKE_BINARY_DIR}/Testing/Temporary/CTestCostData.txt")
  message(STATUS "Removing CTestCostData.txt to force ordering by COST PROPERTY value rather than prior run data")
  file(REMOVE "${CMAKE_BINARY_DIR}/Testing/Temporary/CTestCostData.txt")
endif()

include(CTest)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CreateSleepDelete.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/CreateSleepDelete.cmake
  @ONLY
  )

foreach(n RANGE 1 100)
  add_test(NAME t${n}
    COMMAND ${CMAKE_CTEST_COMMAND}
      -D basefilename=f${n}
      -S ${CMAKE_CURRENT_BINARY_DIR}/CreateSleepDelete.cmake
    )
  set_property(TEST t${n} PROPERTY FAIL_REGULAR_EXPRESSION "(c='[5-9]'|c='[1-9][0-9]+')")
endforeach()

set_property(TEST t1 PROPERTY RUN_SERIAL 1)
set_property(TEST t1 PROPERTY PROCESSORS 4)

set_property(TEST t51 PROPERTY RUN_SERIAL 1)
set_property(TEST t51 PROPERTY PROCESSORS 4)

foreach(n RANGE 2 50)
  set_property(TEST t${n} PROPERTY DEPENDS t1)
endforeach()
set_property(TEST t1 PROPERTY DEPENDS t51)
set_property(TEST t51 PROPERTY DEPENDS t100)

foreach(n 50)
  set_property(TEST t${n} PROPERTY COST 6)
endforeach()
foreach(n RANGE 52 99)
  set_property(TEST t${n} PROPERTY COST 3)
endforeach()
