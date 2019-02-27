include(RunCMake)

# Detect the compiler in use in the current environment.
run_cmake(FindCompiler)
include(${RunCMake_BINARY_DIR}/FindCompiler-build/cc.cmake)
if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "FindCompiler provided no compiler!")
endif()
if(NOT IS_ABSOLUTE "${CMAKE_C_COMPILER}")
  message(FATAL_ERROR "FindCompiler provided non-absolute path \"${CMAKE_C_COMPILER}\"!")
endif()
if(NOT EXISTS "${CMAKE_C_COMPILER}")
  message(FATAL_ERROR "FindCompiler provided non-existing path \"${CMAKE_C_COMPILER}\"!")
endif()

# Now that we have the full compiler path, hide CC.
unset(ENV{CC})

# Wrap around the real compiler so we can change the compiler
# path without changing the underlying compiler.
set(ccIn ${RunCMake_SOURCE_DIR}/cc.sh.in)
set(cc1 ${RunCMake_BINARY_DIR}/cc1.sh)
set(cc2 ${RunCMake_BINARY_DIR}/cc2.sh)
set(cc3 CMAKE_C_COMPILER-NOTFOUND)
configure_file(${ccIn} ${cc1} @ONLY)
configure_file(${ccIn} ${cc2} @ONLY)

# Use a single build tree for remaining tests without cleaning.
set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/ChangeCompiler-build)
set(RunCMake_TEST_NO_CLEAN 1)
file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")

# Check build with compiler wrapper 1.
set(RunCMake_TEST_OPTIONS -DCMAKE_C_COMPILER=${cc1})
set(ENV{RunCMake_TEST} "FirstCompiler")
run_cmake(FirstCompiler)
include(${RunCMake_TEST_BINARY_DIR}/cc.cmake)
if(NOT "${CMAKE_C_COMPILER}" STREQUAL "${cc1}")
  message(FATAL_ERROR "FirstCompiler built with compiler:\n  ${CMAKE_C_COMPILER}\nand not with:\n  ${cc1}")
endif()

# Check rebuild with compiler wrapper 2.
set(RunCMake_TEST_OPTIONS -DCMAKE_C_COMPILER=${cc2})
set(ENV{RunCMake_TEST} "SecondCompiler")
run_cmake(SecondCompiler)
include(${RunCMake_TEST_BINARY_DIR}/cc.cmake)
if(NOT "${CMAKE_C_COMPILER}" STREQUAL "${cc2}")
  message(FATAL_ERROR "SecondCompiler built with compiler:\n  ${CMAKE_C_COMPILER}\nand not with:\n  ${cc2}")
endif()

# Check failure with an empty compiler string.
set(RunCMake_TEST_OPTIONS -DCMAKE_C_COMPILER=)
set(ENV{RunCMake_TEST} "EmptyCompiler")
run_cmake(EmptyCompiler)
include(${RunCMake_TEST_BINARY_DIR}/cc.cmake)
if(NOT "${CMAKE_C_COMPILER}" STREQUAL "${cc3}")
  message(FATAL_ERROR "Empty built with compiler:\n  ${CMAKE_C_COMPILER}\nand not with:\n  ${cc3}")
endif()
