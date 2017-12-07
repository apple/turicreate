cmake_minimum_required (VERSION 2.6)
project(SUBDIR)

# Some systems do not seem to support rpath with spaces.
if(CMAKE_SYSTEM_NAME MATCHES "IRIX|QNX")
  set(CMAKE_SKIP_BUILD_RPATH 1)
endif()

# be able to see output from make on dashboards
set(CMAKE_VERBOSE_MAKEFILE 1)
message("${CMAKE_MAKE_PROGRAM}")
set(CMAKE_PAREN TRUE)
if("${CMAKE_MAKE_PROGRAM}" MATCHES "wmake")
  message("wmake does not support () in path")
  set(CMAKE_PAREN FALSE)
elseif("${CMAKE_MAKE_PROGRAM}" MATCHES "make")
  execute_process(COMMAND ${CMAKE_MAKE_PROGRAM} no_such_target --version
    RESULT_VARIABLE res OUTPUT_VARIABLE out ERROR_VARIABLE out)
  if("${out}" MATCHES "GNU Make 3.82")
    # GNU Make 3.82 fails on parens: http://savannah.gnu.org/bugs/?30612
    message(STATUS "GNU Make 3.82 sometimes fails on () in path")
    set(CMAKE_PAREN FALSE)
  endif()
endif()

if(CMAKE_PAREN)
  add_definitions(-DCMAKE_PAREN=1)
  subdirs("Executable Sources" "Some(x86) Sources" EXCLUDE_FROM_ALL "Some Examples")
else()
  subdirs("Executable Sources" EXCLUDE_FROM_ALL "Some Examples")
endif()

write_file(${SUBDIR_BINARY_DIR}/ShouldBeHere "This file should exist.")
#WATCOM WMAKE does not support + in the name of a file!
if(WATCOM)
  set(PLUS_NAME_FILES
    "Another Subdir/pair_int.int.c"
    vcl_algorithm_vcl_pair_double.foo.c)
else()
    set(PLUS_NAME_FILES
    "Another Subdir/pair+int.int.c"
    vcl_algorithm+vcl_pair+double.foo.c)
endif()

add_executable(TestFromSubdir
  "Another Subdir/testfromsubdir.c"
  "Another Subdir/secondone"
  ${PLUS_NAME_FILES}
  )

aux_source_directory(ThirdSubDir SOURCES)
if(WATCOM)
  foreach(f ${SOURCES})
    if("${f}" STREQUAL "ThirdSubDir/pair+int.int1.c")
    else()
      set(SOURCES2 ${f} ${SOURCES2})
    endif()
  endforeach()
  set(SOURCES ${SOURCES2})
  set(SOURCES ${SOURCES}
    vcl_algorithm_vcl_pair_double.foo.c)
else()
  foreach(f ${SOURCES})
    if("${f}" STREQUAL "ThirdSubDir/pair_int.int1.c")
    else()
      set(SOURCES2 ${f} ${SOURCES2})
    endif()
  endforeach()
  set(SOURCES ${SOURCES2})
  set(SOURCES ${SOURCES}
    vcl_algorithm+vcl_pair+double.foo.c)
endif()
add_executable(TestWithAuxSourceDir ${SOURCES})
if(CMAKE_PAREN)
  target_link_libraries(TestWithAuxSourceDir testOddPath)
endif()

