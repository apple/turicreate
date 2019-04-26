cmake_minimum_required (VERSION 2.6)
project(SUBDIR)

subdirs(Executable EXCLUDE_FROM_ALL Examples)

set(DEFINED_AFTER_SUBDIRS_COMMAND 42)

write_file(${SUBDIR_BINARY_DIR}/ShouldBeHere "This file should exist.")
#WATCOM WMAKE does not support + in the name of a file!
if(WATCOM)
  set(PLUS_NAME_FILES
    AnotherSubdir/pair_int.int.c
    vcl_algorithm_vcl_pair_double.foo.c)
else()
    set(PLUS_NAME_FILES
    AnotherSubdir/pair+int.int.c
    vcl_algorithm+vcl_pair+double.foo.c)
endif()

add_executable(TestFromSubdir
  AnotherSubdir/testfromsubdir.c
  AnotherSubdir/secondone
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
      message("${f}")
    endif()
  endforeach()
  set(SOURCES ${SOURCES2})
  set(SOURCES ${SOURCES}
    vcl_algorithm+vcl_pair+double.foo.c)
endif()
message("Sources: ${SOURCES}")
add_executable(TestWithAuxSourceDir ${SOURCES})
