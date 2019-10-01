cmake_minimum_required (VERSION 2.8)
project(FortranOnly Fortran)
message("CTEST_FULL_OUTPUT ")

# create a library with hello and world functions
add_library(FortranOnlylib hello.f world.f)
set_property(TARGET FortranOnlylib PROPERTY Fortran_FORMAT FIXED)
set_property(SOURCE world.f PROPERTY Fortran_FORMAT FREE)

# create an executable that calls hello and world
add_executable(FortranOnly1 testf.f)
set_property(TARGET FortranOnly1 PROPERTY OUTPUT_NAME FortranOnly)
target_link_libraries(FortranOnly1 FortranOnlylib)

# create a custom command that runs FortranOnly1 and puts
# the output into the file testfhello.txt
add_custom_command(OUTPUT ${FortranOnly_BINARY_DIR}/testfhello.txt
  COMMAND FortranOnly1
  > testfhello.txt)
# create a second executable FortranOnly2 that has
# testfhello.txt has an source file so that it will
# run the above custom command.
add_executable(FortranOnly2 testfhello.txt testf.f)
target_link_libraries(FortranOnly2 FortranOnlylib)
# create a custom target to check the content of testfhello.txt
# by running the cmake script checktestf2.cmake
add_custom_target(checktestf2 ALL
  COMMAND ${CMAKE_COMMAND}
  -P ${FortranOnly_SOURCE_DIR}/checktestf2.cmake)

# create a custom target that runs FortranOnly1 executable and creates
# a file out.txt that should have hello world in it.
add_custom_target(sayhello ALL
  COMMAND FortranOnly1 > out.txt
)
# make sure stuff is built in the right order
add_dependencies(checktestf2 FortranOnly2)
add_dependencies(sayhello FortranOnly1)
add_dependencies(FortranOnly2 FortranOnly1)

# add a custom target that checks that out.txt has the correct
# content
add_custom_target(checksayhello ALL
  COMMAND ${CMAKE_COMMAND} -P ${FortranOnly_SOURCE_DIR}/checksayhello.cmake
  )
add_dependencies(checksayhello sayhello)

# Exclude this test on IBM XL for now because the check strangely
# fails with 'ld: 0706-029 Use a number with the -H flag'.
if(NOT CMAKE_Fortran_COMPILER_ID STREQUAL XL)
  set(err_log ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log)
  file(REMOVE "${err_log}")
  include(CheckFortranSourceCompiles)
  unset(HAVE_PRINT CACHE)
  CHECK_Fortran_SOURCE_COMPILES([[
      PROGRAM TEST_HAVE_PRINT
        PRINT *, 'Hello'
      END
]] HAVE_PRINT)
  if(NOT HAVE_PRINT)
    if(EXISTS "${err_log}")
      file(READ "${err_log}" err)
    endif()
    string(REPLACE "\n" "\n  " err "  ${err}")
    message(SEND_ERROR "CHECK_Fortran_SOURCE_COMPILES for HAVE_PRINT failed:\n"
      "${err}")
  endif()

  unset(Fortran_BOGUS_FLAG CACHE)
  include(CheckFortranCompilerFlag)
  CHECK_Fortran_COMPILER_FLAG(-_this_is_not_a_flag_ Fortran_BOGUS_FLAG)
  if (Fortran_BOGUS_FLAG)
    message (SEND_ERROR "CHECK_Fortran_COMPILER_FLAG() succeeded, but should have failed")
  endif ()
endif()

# Test generation of preprocessed sources.
if("${CMAKE_GENERATOR}" MATCHES "Makefile" AND CMAKE_MAKE_PROGRAM)
  if(CMAKE_Fortran_CREATE_PREPROCESSED_SOURCE)
    # Skip running this part of the test on certain platforms
    # until they are fixed.
    set(MAYBE_ALL ALL)
    list(LENGTH CMAKE_OSX_ARCHITECTURES ARCH_COUNT)
    if(ARCH_COUNT GREATER 1)
      # OSX does not support preprocessing more than one architecture.
      set(MAYBE_ALL)
    endif()

    add_executable(preprocess preprocess.F)

    # Custom target to try preprocessing invocation.
    add_custom_target(test_preprocess ${MAYBE_ALL}
      COMMAND ${CMAKE_COMMAND} -E remove CMakeFiles/preprocess.dir/preprocess.F.i
      COMMAND ${CMAKE_MAKE_PROGRAM} preprocess.i
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/test_preprocess.cmake
      # Remove bogus file some compilers leave behind.
      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/preprocess.s
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
  endif()
endif()
