set(depend_make "${RunCMake_TEST_BINARY_DIR}/CMakeFiles/DepTarget.dir/depend.make")
if(EXISTS "${depend_make}")
  file(READ "${depend_make}" depend_make_content)
  string(REGEX REPLACE "\n+$" "" depend_make_content "${depend_make_content}")
  if(NOT depend_make_content MATCHES "
CMakeFiles/DepTarget.dir/test.c.o: .*/Tests/RunCMake/CommandLine/cmake_depends/test.c
CMakeFiles/DepTarget.dir/test.c.o: .*/Tests/RunCMake/CommandLine/cmake_depends/test.h$")
    string(REPLACE "\n" "\n  " depend_make_content "  ${depend_make_content}")
    set(RunCMake_TEST_FAILED "depend.make does not have expected content:\n${depend_make_content}")
  endif()
else()
  set(RunCMake_TEST_FAILED "depend.make missing:\n ${depend_make}")
endif()
